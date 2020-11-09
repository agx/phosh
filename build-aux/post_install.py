#!/usr/bin/python3

import os
import subprocess
import sys

destdir = os.environ.get('DESTDIR', '')

if not destdir and len(sys.argv) > 1:
    datadir = sys.argv[1]

    print('Compiling gsettings schemas...')
    subprocess.call(['glib-compile-schemas', os.path.join(datadir, 'glib-2.0', 'schemas')])
