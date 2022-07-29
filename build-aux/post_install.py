#!/usr/bin/python3

import os
import subprocess
import sys

if 'DESTDIR' not in os.environ:
    datadir = sys.argv[1]
    phosh_plugins_dir = sys.argv[2]

    print('Compiling gsettings schemas...')
    subprocess.call(['glib-compile-schemas', os.path.join(datadir, 'glib-2.0', 'schemas')])

    print('Updating module cache for print backends...')
    os.makedirs(phosh_plugins_dir, exist_ok=True)
    gio_querymodules = subprocess.check_output(['pkg-config',
                                                '--variable=gio_querymodules',
                                                'gio-2.0']).strip()
    if not os.path.exists(gio_querymodules):
        # pkg-config variables only available since GLib 2.62.0.
        gio_querymodules = 'gio-querymodules'
    subprocess.call([gio_querymodules, phosh_plugins_dir])
