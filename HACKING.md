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

[1]: https://source.puri.sm/Librem5/libhandy/blob/master/HACKING.md#coding-style
