# Phosh

a trivial wayland shell for prototyping things.

## License

phosh is licensed under the GPLv3+.

## Dependencies

    sudo apt-get install libgnome-desktop-3-dev libgtk-3-dev libpam0g-dev libupower-glib-dev libwayland-dev meson

Until distros ship [libhandy](https://source.puri.sm/Librem5/libhandy) you
need to build that from source as well.

## Building

We use the meson (and thereby Ninja) build system for phosh.  The quickest
way to get going is to do the following:

    meson . _build
    ninja -C _build
    ninja -C _build install


## Running
When running from the source tree start *rootston*. Then start *phosh*
using:

    _build/run

or in one command:`

    ../wlroots/_build/rootston/rootston -E _build/run -C ./rootston.ini

This will make sure the gsettings schema is found, there's enough of a GNOME
session running an the some of the mutter DBus API is stubbed so
gnome-settings-manager can work.
