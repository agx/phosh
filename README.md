# Phosh

a trivial wayland shell for prototyping things.

## License

phosh is licensed under the GPLv3+.

## Dependencies

    sudo apt-get install libgtk-3-dev wayland-protocols

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

This will make sure the gsettings schema is found, there's enough of a GNOME
session running an the some of the mutter DBus API is stubbed so
gnome-settings-manager can work.
