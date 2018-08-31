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

# Testing

To run the tests run

    ninja -C _build test

For details see the *.gitlab-ci.yml* file.

## Running
### Running from the source tree
When running from the source tree start the compositor *rootston*. Then start
*phosh* using:

    _build/run -U

or in one command:

    ../wlroots/_build/rootston/rootston -E '_build/run -U' -C ./rootston.ini

This will make sure the needed gsettings schema is found. The '-U' option makes
sure the shell is not locked on startup so you can test with arbitrary
passwords.

### Running from the Debian packages
If installed via the Debian packages you can also run phosh via gnome-session.
It ships a file in /usr/share/gnome-session/sessions so you can bring up a
session using

    gnome-session --disable-acceleration-check --session=phosh

This assumes you have the compositor already running. If you want to start
phosh at system boot there's a systemd unit file in */lib/systemd/system/phosh*
which is disabled by default:

    systemctl enable phosh
    systemctl start phosh

This runs *phosh* as user *purism* (which needs to exist). If you don't have a
user *purism* and don't want to create on you can make systemd run *phosh* as
any user by using an override file:

    $ cat /etc/systemd/system/phosh.service.d/override.conf
    [Service]
    User=<your_user>
    WorkingDirectory=<your_home_directory>

