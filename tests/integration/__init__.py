#!/usr/bin/python3
#
# (C) 2025 The Phosh Developers
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Author: Guido Günther <agx@sigxcpu.org>


import re
import time
import fcntl
import os
import subprocess
import sys


class Phosh:
    """
    Wrap phosh startup and teardown
    """

    wl_display = None
    process = None

    def __init__(self, topsrcdir, topbuilddir):
        self.topsrcdir = topsrcdir
        self.topbuilddir = topbuilddir

        if not os.getenv("XDG_RUNTIME_DIR"):
            print(f"'XDG_RUNTIME_DUR' unset, setting to {topbuilddir}")
            os.environ["XDG_RUNTIME_DIR"] = topbuilddir

    def teardown_nested(self):
        self.process.send_signal(15)
        out = self.process.communicate()
        if self.process.returncode != -15:
            print(f"Exited with {self.process.returncode}", file=sys.stderr)
            print(f"stdout: {out[0].decode('utf-8')}", file=sys.stderr)
            print(f"stderr: {out[1].decode('utf-8')}", file=sys.stderr)
            return False
        return True

    def find_wlr_backend(self):
        if os.getenv("WLR_BACKENDS"):
            return os.getenv("WLR_BACKENDS")
        elif os.getenv("WAYLAND_DISPLAY"):
            return "wayland"
        elif os.getenv("DISPLAY"):
            return "x11"
        else:
            return "headless"

    def spawn_nested(self):
        phoc_ini = os.path.join(self.topsrcdir, "data", "phoc.ini")
        runscript = os.path.join(self.topbuilddir, "run")

        env = os.environ.copy()
        env["GSETTINGS_BACKEND"] = "memory"
        env["WLR_BACKENDS"] = self.find_wlr_backend()

        # Spawn phoc -E .../run
        cmd = ["phoc", "-C", phoc_ini, "-E", runscript]
        self.process = subprocess.Popen(
            cmd, env=env, stderr=subprocess.PIPE, stdout=subprocess.PIPE
        )

        for fd in [self.process.stdout.fileno(), self.process.stderr.fileno()]:
            fl = fcntl.fcntl(fd, fcntl.F_GETFL)
            fcntl.fcntl(fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)

        stderr_output = ""
        stdout_output = ""
        timedout = False
        timeout = 5
        # Wait for phosh to report it is up:
        while not timedout:
            assert self.process.poll() is None
            out = self.process.stdout.read()
            if out:
                stdout_output += out.decode("utf-8")

            out = self.process.stderr.read()
            if out:
                stderr_output += out.decode("utf-8")
            if "Phosh ready after" in stderr_output:
                print("Phosh ready")
                break
            time.sleep(1)
            if timeout == 0:
                break
                timedout = True
            timeout -= 1

        assert timedout is False
        self.process.stdout.close()
        self.process.stderr.close()

        display_re = re.compile(
            r"Running compositor on wayland display '(?P<display>wayland-([0-9]+))'"
        )
        for line in stdout_output.split("\n"):
            m = display_re.match(line)
            if m:
                self.wl_display = m.group("display")
                break
        if not self.wl_display:
            self.teardown_nested()
            raise Exception("Failed to find Wayland display")

        # TODO: Check availability on DBus
        time.sleep(2)
        return self
