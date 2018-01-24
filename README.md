# Phosh

a trivial desktop shell devices. This is just so that we can launch a terminal
and try out stuff.

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

