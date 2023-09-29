Title: Getting started with Phosh development
Slug: gettingstarted

# Getting started with Phosh development

## Overview
Phosh is a graphical shell for
[Wayland](https://wayland.freedesktop.org/). It's target at mobile
devices with small screens like Purism's Librem 5 running adaptive
[GNOME](https://gitlab.gnome.org/) applications.

It's purpose is to provide a graphical user interface to launch
applications, display status information (time, battery status, ...),
to provide a lock screen and to make often used functionality quickly
accessible.  It's also meant to provide a proper interface when either
connecting a keyboard and monitor to a phone or (but to a lesser
extend) when running on a laptop.

Since it acts as a Wayland client it needs a compositor to function
that provides the necessary protocols (most notably
wlr-layer-shell). It's usually used with
[phoc](https://gitlab.gnome.org/World/Phosh/phoc) (the PHOne Compositor).

On the GNOME side it interfaces with the usual components
(e.g. [gnome-settings-daemon](https://gitlab.gnome.org/GNOME/gnome-settings-daemon),
[upower](https://gitlab.freedesktop.org/upower/upower),
[iio-sensor-proxy](https://gitlab.freedesktop.org/hadess/iio-sensor-proxy/))
via DBus. For haptic feedback it uses
[feedbackd](https://source.puri.sm/Librem5/feedbackd/).

It uses [GTK](https://www.gtk.org/) as it's GUI toolkit and
[libhandy](https://gitlab.gnome.org/GNOME/libhandy) for adaptive
widgets.

Although targeted at touch devices Phosh does not implement a
on screen keyboard (OSK) but leaves this to
[squeekboard](https://gitlab.gnome.org/World/Phosh/squeekboard).

The above combination of software is also often (a bit imprecisely)
named Phosh. For a high level overview see [Phosh Overview](https://honk.sigxcpu.org/con/phosh_overview.html).

### Wayland protocols
Since Phoc (in contrast to some other solutions) aims to be a minimal
Wayland compositor that manages rendering, handle physical and virtual
input and display devices but not much more it needs to provide some
more protocols to enable graphical shells like Phosh. These are usually
from [wlr-protocols](https://github.com/swaywm/wlr-protocols) and
Phoc uses the [wlroots](https://github.com/swaywm/wlroots) library for
implementing them.

These are the most prominent ones use by Phosh:

- wlr-layer-shell: Usually Wayland clients have little influence on where
  the compositor places them. This protocol gives Phosh enough room
  to build the top bar via #PhoshTopPanel, the home bar #PhoshHome at
  the bottom, system modal dialogs e.g. #PhoshSystemPrompt and
  lock screens via #PhoshLockscreen.
- wlr-foreign-toplevel-management: This allows the management of
  toplevels (windows). Phosh uses this to build an application switcher
  called #PhoshOverview.
- wlr-output-management: This allows to manage a monitors power
  state (to e.g.turn it off when unused).

Besides those Phosh uses a number of "regular" Wayland client
protocols like `xdg_output`, `wl_output` or `wl_seat` (see
#PhoshWayland for the full list).

### Session startup

Since Phosh is in many aspects a regular GTK application it's started
as part GNOME session so the start sequence looks like

```
phoc (compositor) -> gnome-session -> phosh (and other session components)
```

## Hints
This is a unsorted list of hints when developing for Phosh

### Running phosh
For development purposes you can run phosh nested on your desktop. See
[this blog post](https://phosh.mobi/posts/phosh-dev-part-0/) for
details.

### Manager Objects

Phosh uses several manager objects e.g. #PhoshBackgroundManager,
#PhoshMonitorManager, #PhoshLockscreenManager to keep track
of certain objects (monitors, lock screens, backgrounds) and to
trigger events on those when needed.  They're usually created and
disposed by #PhoshShell. Some of them like
#PhoshWayland are singletons so you can access them from basically
anywhere in the codebase.

### Status Information Widgets

Several widgets listen to changes on DBus objects in order to e.g. display the
current connectivity - see #PhoshConnectivityInfo for an example that monitors
network connectivity.

Sometimes it is no longer useful to show the widget (since
e.g. the corresponding DBus service went away). In that case the
widget should flip a boolean property so the parent container can
hide the object via #g_object_bind_property().

### Screen locking and blanking

For details see [class@ScreenSaverManager].

### Debugging

Since phosh is a GTK application you can use
[GtkInspector](https://wiki.gnome.org/Projects/GTK/Inspector).
You can use the `GTK_INSPECTOR_DISPLAY` environment variable to use a different
Wayland display for the inspector window. This can be useful to have the
inspector windows outside of a nested Wayland session.
