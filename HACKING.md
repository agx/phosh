# Contributing to Phosh

Below are some basic guidelines on coding style, source file layout and merge
requests that hopefully makes it easier for you to land your code.

## Building

For build instructions see the [README.md](./README.md)

## Development Documentation

For internal API documentation as well as notes for application developers see [here](https://world.pages.gitlab.gnome.org/Phosh/phosh/).

## Merge requests

Before filing a pull request run the tests:

```sh
meson test -C _build --print-errorlogs
```

Use descriptive commit messages, see

   <https://wiki.gnome.org/Git/CommitMessages>

and check

   <https://wiki.openstack.org/wiki/GitCommitMessages>

for good examples. The commits in a merge request should have "recipe"
style history rather than being a work log. See
[here](https://www.bitsnbites.eu/git-history-work-log-vs-recipe/) for
an explanation of the difference. The advantage is that the code stays
bisectable and individual bits can be cherry-picked or reverted.

### Checklist

When submitting a merge request consider checking these first:

- [ ] Does the code use the below coding patterns?
- [ ] Is the commit history in recipe style (see above)?
- [ ] Do the commit messages reference the bugs they fix. If so,
      - Use `Helps:` if the commit partially addresses a bug or contributes
        to a solution.
      - Use `Closes:` if the commit fully resolves the bug. This allows the
        release script to detect & mention it in `NEWS` file.
- [ ] Does the code crash or introduce new `CRITICAL` or `WARNING`
      messages in the log or when run form the console. If so, fix
      these first?
- [ ] Is the new code covered by any tests? This could be a unit test,
      an added [screenshot test](./tests/test-take-screenshots.c),
      a tool to exercise new DBus API (see e.g.
      [tools/check-mount-operation](./tools/check-mount-operation).
- [ ] Are property assignments to default values removed from UI files? (See
      `gtk-builder-tool simplify file.ui`)

If any of the above criteria aren't met yet it's still fine (and
encouraged) to open a merge request marked as draft. Please indicate
why you consider it draft in this case. As Phosh is used on a wide
range of devices and distributions please indicate in what scenarios
you tested your code.

## Coding Patterns

### Coding Style

We're mostly using [libhandy's Coding Style][1].

These are the differences:

- We're not picky about GTK style function argument indentation, that is
  having multiple arguments on one line is also o.k.
- For signal handler callbacks we prefer for the `on_<signalname>` naming
  pattern e.g. `on_clicked` since this helps to keep the namespace clean. See
  below.
- Since we're not a library we usually use `G_DEFINE_TYPE` instead of
  `G_DEFINE_TYPE_WITH_PRIVATE` (except when we need a deriveable
  type) since it makes the rest of the code more compact.

### Source file layout

We use one file per GObject. It should be named like the GObject without
the phosh prefix, lowercase and '\_' replaced by '-'. So a hypothetical
`PhoshThing` would go to `src/thing.c`. If there are likely name
clashes add the `phosh-` prefix (e.g. `phosh-wayland.c`). The
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

- A log domain, usually the filename with `phosh-` prefix

  ```c
  #define G_LOG_DOMAIN "phosh-thing"
  ```

- `#include`s:
  Phosh ones go first, then glib/gtk, then generic C headers. These blocks
  are separated by newline and each sorted alphabetically:

  ```
  #define G_LOG_DOMAIN "phosh-settings"

  #include "phosh-config.h"

  #include "settings.h"
  #include "shell.h"

  #include <gio/gdesktopappinfo.h>
  #include <glib/glib.h>

  #include <math.h>
  ```

  This helps to detect missing headers in includes.

- docstring:
  If you have trouble to describe the class concisely, then it might be an indication
  that it should be split into multiple classes.

  ```c
  /**
   * PhoshYourThing:
   *
   * Short, single line, summary
   *
   * A longer description with details that can be
   * multiline.
   */
  ```

- property enum

  ```c
  enum {
    PROP_0,
    PROP_FOO,
    PROP_BAR,
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
  static guint signals[N_SIGNALS];
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
  places above `phosh_thing_constructed ()`)
- `phosh_thing_set_property ()`. Set properties.
  If setting a property requires more than a single line prefer adding a setter method,
  e.g. for the `foo` property, the setter method would be
  `phosh_thing_set_foo ()`. This is almost always the case as we prefer
  `G_PARAM_EXPLICIT_NOTIFY` (see below).

  ```c
  static void
  phosh_thing_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
  {
     PhoshThing *self = PHOSH_TING (object);

     switch (property_id) {
     case PROP_FOO:
       phosh_thing_set_foo (self, g_value_get_string (value));
       break;
     …
     default:
       G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
       break;
     }
  }
  ```

- `phosh_thing_get_property ()`
- `phosh_thing_constructed ()`: Finish object constructions. Usually only
  needed if you need the values of multiple properties passed at object
  construction time.
- `phosh_thing_dispose ()`: Usually only needed when you need to break
  reference cycles. Otherwise prefer `finalize`. As `dispose` can be run
  multiple times use `g_clear_*` to avoid freeing resources multiple times:

  ```c
  static void
  phosh_thing_dispose (GObject *object)
  {
    PhoshThing *self = PHOSH_THING (object);

    g_cancellable_cancel (self->cancel);
    g_clear_object (&self->cancel);
    g_clear_object (&self->bar);
    g_clear_pointer (&self->foo, g_free);

    G_OBJECT_CLASS (phosh_thing_parent_class)->dispose (object);
  }
  ```

- `phosh_thing_finalize ()`: Free allocated resources.
- `phosh_thing_class_init ()`: Define properties and signals. For widget
  templates bind child widgets and signal handlers.
- `phosh_thing_init ()`: Initialize defaults for member variables here.
- `phosh_thing_new ()`: A convenience wrapper around `g_object_new ()`. Don't
  do further object initialization here but rather do that in
  `phosh_thing_init ()`, `phosh_thing_constructed ()` or individual property
  setters. This ensures that objects can be constructed either via this
  constructor or `g_object_new ()`.
- Public methods, all starting with the object name (i.e. `phosh_thing_`).

The reason public methods go at the bottom is that they have
declarations in the header file and can thus be referenced from
anywhere else in the source file.

### CSS Theming

For custom widgets set the css name using `gtk_widget_class_set_css_name ()`.
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

### Properties

#### Signal emission on changed properties

Except for `G_CONSTRUCT_ONLY` properties use `G_PARAM_EXPLICIT_NOTIFY` and notify
about property changes only when the underlying variable changes value:

```c
static void
on_present_changed (PhoshDockedInfo *self)
{
  …

  if (self->enabled == enabled)
    return;

  self->enabled = enabled;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ENABLED]);
}

static void
phosh_docked_info_class_init (PhoshDockedInfoClass *klass)
{
  …

  /**
   * PhoshDockedInfo:enabled:
   *
   * Whether docked mode is enabled
   */
  props[PROP_PRESENT] =
    g_param_spec_boolean ("present", "", "",
                          G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);

  …
}
```

This makes sure we minimize the notifications on changed property values.

#### Property documentation

Prefer a docstring over filling in the properties' `nick` and `blurb`. See
example above.

#### Property bindings

If the state of a property depends on the state of another one prefer
`g_object_bind_property ()` to keep these in sync:

*Good*:

```c
  g_object_bind_property (self, "bar",
                          self->avatar, "loadable-icon",
                          G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);
```

This has the upside that the binding goes away automatically when
either of the two objects gets disposed and also avoids bugs in manual
syncing code.

For widgets you can construct the binding via the UI XML:

```xml
  <object class="PhoshQuickSetting" id="foo_quick_setting">
    <property name="sensitive" bind-source="wwaninfo" bind-property="present" bind-flags="sync-create"/>
    …
  </object>
```

#### Callbacks

There's callbacks for signals, async functions, and actions. We
usually have them all start with `on_` to make it easy to spot
that these aren't just methods (and hence have restrictions regarding
their function arguments and return values).

- Signal handlers are named `on_<signalname>`. E.g. the handler for a
  `GListModel`s `items-changed` signal would be named
  `on_items_changed ()`. To avoid ambiguity one can add the emitter's
  name (`on_devices_list_box_items_changed ()`).

- For `notify::` signal handler callbacks acting on property changes
  we use the `on_<property>_changed ()` naming. E.g.
  `on_nm_signal_strength_changed ()`.

- For `GAsyncReadyCallback`s we use `on_<async-function>_ready()`, e.g.
  the callback for `nm_client_new_async` that invokes `nm_client_new_finish`
  would be named `on_nm_client_new_ready ()`.

- For actions we use `on_<action>_activated`, e.g. for a `go-back` action
  we'd use `on_go_back_activated ()`.

- In cases where the signal handler name should express what it does
  rather than what signal it connects to, we use a `_cb` suffix. This
  is often the case when we want to use the same signal handler to
  handle multiple signals. E.g. a callback that updates a
  `GtkStackPage` when a signal happens would be named
  `update_stack_page_cb ()`.

### API contracts

Public (non static) functions must check the input arguments at the
top of the function. This makes it easy to reuse them in other parts
and makes API misuse easy to debug via `G_DEBUG=fatal-criticals`. You
usually want to check argument types and if the arguments fulfill the
requirements (e.g. if they need to be non-NULL).

*Good*:

```c
void
phosh_foo_set_name (PhoshFoo *self, const char *name)
{
  GtkWidget *somewidget;

  g_return_if_fail (PHOSH_IS_FOO (self));
  g_return_if_fail (!name);

  …
}
```

[1]: https://gitlab.gnome.org/GNOME/libhandy/blob/master/HACKING.md#coding-style
