# Phosh

All development now happens on the `main` branch. To update your local checkout
to use that branch:
```sh
git checkout master
git branch -m master main
git fetch
git branch --unset-upstream
git branch -u origin/main
git symbolic-ref refs/remotes/origin/HEAD refs/remotes/origin/main
```

This branch is kept around to avoid breaking build tooling that requires a
pinned commit hash to be on the `master` branch unless otherwise specified. See
https://github.com/ostreedev/ostree/issues/2360 for details.

