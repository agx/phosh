# Phosh

a trivial wayland shell for prototyping things.

## License

phosh is licensed under the GPLv3+.

## Dependencies

    sudo apt-get install libgtk3.0-dev wayland-protocols

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

If you want to test interaction with gnome-settings-daemon e.g. for brightness
start a session like:

    gnome-session --session=gnome-dummy --disable-acceleration-check &

before running phosh.
