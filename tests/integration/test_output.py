#!/usr/bin/python3
#
# (C) 2025 The Phosh Developers
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Author: Guido Günther <agx@sigxcpu.org>

import re
import time
import os
import subprocess
import sys
import pytest

from . import Phosh


@pytest.fixture(scope="class")
def run_phosh():
    topsrcdir = os.environ["TOPSRCDIR"]
    topbuilddir = os.environ["TOPBUILDDIR"]

    success = False
    phosh = Phosh(topsrcdir, topbuilddir).spawn_nested()
    yield phosh
    success = phosh.teardown_nested()
    if not success:
        raise Exception("Phosh teardown failed")


class TestSingleOutput:
    """
    Test monitor/output related bits
    """

    def get_output(self):
        out = subprocess.check_output(["wlr-randr"], env=self.env)
        output = out.decode("utf-8").split("\n")[0].split()[0]
        assert output in ["WL-1", "X11-1", "HEADLESS-1"]
        return output

    def check_mode(self, scale):
        output = self.get_output()
        assert (
            subprocess.check_output(
                ["wlr-randr", "--output", output, "--scale", f"{scale}"], env=self.env
            )
            == b""  # noqa: W503
        )
        time.sleep(1)
        # TODO: take screenshot
        out = subprocess.check_output(["wlr-randr"], env=self.env)
        lines = out.decode("utf-8").split("\n")
        new_scale = 0.0
        scale_re = re.compile(r".*Scale: (?P<scale>[0-9.]+)")
        for line in lines:
            m = scale_re.match(line)
            if m:
                print(m, file=sys.stderr)
                new_scale = float(m.group("scale"))
        assert scale != 0.0, f"No scale found in output {lines})"
        assert scale == new_scale

    @pytest.mark.parametrize("scale", [1, 2, 2.75, 1.25, 1])
    def test_scale(self, run_phosh, scale):
        self.env = os.environ.copy()
        self.env["WAYLAND_DISPLAY"] = run_phosh.wl_display
        self.check_mode(scale)
