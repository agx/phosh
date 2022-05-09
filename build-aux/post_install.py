#!/usr/bin/python3

import os
import subprocess
import sys

if 'DESTDIR' not in os.environ:
    datadir = sys.argv[1]

    print('Compiling gsettings schemas...')
    subprocess.call(['glib-compile-schemas', os.path.join(datadir, 'glib-2.0', 'schemas')])
