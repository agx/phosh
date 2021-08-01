Building
========
For build instructions see the README.md

Pull requests
=============
Before filing a pull request run the tests:

```sh
ninja -C _build test
```

Use descriptive commit messages, see

   https://wiki.gnome.org/Git/CommitMessages

and check

   https://wiki.openstack.org/wiki/GitCommitMessages

for good examples.

Coding Style
============
We're mostly using [libhandy's Coding Style][1].

These are the differences:

- We're not picky about GTK+ style function argument indentation, that is
  having multiple arguments on one line is also o.k.
- For callbacks we additionally allow for the `on_<action>` pattern e.g.
  `on_nm_signal_strength_changed()` since this helps to keep the namespace
  clean.
- Since we're not a library we usually use `G_DEFINE_TYPE` instead of
  `G_DEFINE_TYPE_WITH_PRIVATE` (except when we need a deriveable
  type) since it makes the rest of the code more compact.

Source file layout
------------------
We use one file per GObject. It should be named like the GObject without
the phosh prefix, lowercase and '_' replaced by '-'. So a hypothetical
`PhoshThing` would go to `src/thing.c`. If there are likely name
clashes add the `phosh-` prefix (see e.g. `phosh-wayland.c`). The
individual C files should be structured as (top to bottom of file):

  - License boilerplate
    ```c
    /*
     * Copyright (C) year copyright holder
     *
     * SPDX-License-Identifier: GPL-3-or-later
     * Author: you <youremail@example.com>
     */
    ```
  - A log domain
    ```C
    #define G_LOG_DOMAIN "phosh-thing"
    ```
    Usually the filename with `phosh-` prefix.
  - `#include`s:
    Phosh ones go first, then glib/gtk, then generic C headers. These blocks
    are separated by newline and each sorted alphabetically:

    ```
    #define G_LOG_DOMAIN "phosh-settings"

    #include "settings.h"
    #include "shell.h"

    #include <gio/gdesktopappinfo.h>
    #include <glib/glib.h>

    #include <math.h>
    ```

    This helps to detect missing headers in includes.
  - docstring
  - property enum
    ```c
    enum {
      PROP_0,
      PROP_FOO,
      PROP_BAR,,
      LAST_PROP
    };
    static GParamSpec *props[LAST_PROP];
    ```
  - signal enum
    ```c
    enum {
      FOO_HAPPENED,
      BAR_TRIGGERED,
      N_SIGNALS
    };
    static guint signals[N_SIGNALS] = { 0 };
    ```
  - type definitions
    ```c
    typedef struct _PhoshThing {
      GObject parent;

      ...
    } PhoshThing;

    G_DEFINE_TYPE (PhoshThing, phosh_thing, G_TYPE_OBJECT)
    ```
  - private methods and callbacks (these can also go at convenient
    places above `phosh_thing_constructed ()`
  - `phosh_thing_set_property ()`
  - `phosh_thing_get_property ()`
  - `phosh_thing_constructed ()`
  - `phosh_thing_dispose ()`
  - `phosh_thing_finalize ()`
  - `phosh_thing_class_init ()`
  - `phosh_thing_init ()`
  - `phosh_thing_new ()`
  - Public methods, all starting with the object name(i.e. `phosh_thing_`)

  The reason public methods go at the bottom is that they have declarations in
  the header file and can thus be referenced from anywhere else in the source
  file.

CSS Theming
===========
For custom widget always set the css name using `gtk_widget_class_set_css_name ()`.
There's no need set an (additional) style class in the ui file.

*Good*:

```c
static void
phosh_lockscreen_class_init (PhoshLockscreenClass *klass)
{
  …
  gtk_widget_class_set_css_name (widget_class, "phosh-lockscreen");
  …
}
```

*Bad*:

```xml
  <template class="PhoshLockscreen" parent="…">
  …
     <style>
         <class name="phosh-lockscreen"/>
      </style>
  …
  </template>
```

[1]: https://gitlab.gnome.org/GNOME/libhandy/blob/master/HACKING.md#coding-style
