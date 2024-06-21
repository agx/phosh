#!/usr/bin/python3
#
# (C) 2024 The Phosh Developers
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Author: Guido Günther <agx@sigxcpu.org>

"""
Spawn ModemManager and NetworkManager mocks on the system bus
"""

import os
import subprocess
import sys

from dbusmock.testcase import SpawnedMock


def cleanup(servers):
    print("Cleaning up")

    for server in servers:
        os.kill(server.process.pid, 15)


def main():
    mm_server = None
    nm_server = None
    nm_parameters = {"Version": "1.46.0"}

    try:
        mm_server = SpawnedMock.spawn_with_template("modemmanager")
        nm_server = SpawnedMock.spawn_with_template("networkmanager", nm_parameters)

        # Add a modem
        mm_server.obj.AddSimpleModem()

        # Add gsm connection
        subprocess.check_output(["nmcli", "c", "add", "connection.type", "gsm"])

        input("Hit <Return> to quit")
        ret = 0
    except Exception as e:
        print(e)
        ret = 1

    cleanup((mm_server, nm_server))
    return ret


if __name__ == "__main__":
    sys.exit(main())
