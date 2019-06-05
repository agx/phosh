# Phosh
[![Pipeline status](https://source.puri.sm/Librem5/phosh/badges/master/build.svg)](https://source.puri.sm/Librem5/phosh/commits/master)

a trivial wayland shell for prototyping things.

## License

phosh is licensed under the GPLv3+.

## Getting the source

```sh
    git clone https://source.puri.sm/Librem5/phosh
    cd phosh
```

The master branch has the current development version.

## Dependencies
On a Debian based system run

```sh
    sudo apt-get -y install build-essential
    sudo apt-get -y build-dep .
```

For an explicit list of dependencies check the `Build-Depends` entry in the
[debian/control][] file.

Until distros ship [libhandy](https://source.puri.sm/Librem5/libhandy) you need
to build that from source. More details are in the [gitlab-ci.yml][] file.

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

There are some thorough tests not run during CI which can e.g. be run via

    gtester -m thorough  _build/tests/test-idle-manager

## Running
### Running from the source tree
When running from the source tree start the compositor *rootston*. Then start
*phosh* using:

    _build/run -U

or in one command:

    ../wlroots/_build/rootston/rootston -E '_build/run -U' -C ./data/rootston.ini

This will make sure the needed gsettings schema is found. The '-U' option makes
sure the shell is not locked on startup so you can test with arbitrary
passwords.

If you're not running natively on hardware but nested e.g. for development
under GNOME make sure rootston uses the X11 backend so the window scales
correctly to the Librem5's size:

    WLR_BACKENDS=x11 ../wlroots/_build/rootston/rootston -E '_build/run -U' -C ./data/rootston.ini

For that it doesn't matter if you're using GNOME's Wayland or X11 session.

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

# Translations
Please use zanata for translations at https://translate.zanata.org/project/view/phosh?dswid=-3784

# Getting in Touch
* Issue tracker: https://source.puri.sm/Librem5/phosh
* Mailing list: https://lists.community.puri.sm/listinfo/librem-5-dev
* Matrix: https://im.puri.sm/#/room/#phosh:talk.puri.sm
* XMPP: phosh@conference.sigxcpu.org

For details see the [developer documentation](https://developer.puri.sm/Contact.html).

[gitlab-ci.yml]: https://source.puri.sm/Librem5/phosh/blob/master/.gitlab-ci.yml
[debian/control]: https://source.puri.sm/Librem5/phosh/blob/master/debian/control
