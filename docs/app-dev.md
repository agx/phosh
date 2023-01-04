Title: Notes for Application Developers
Slug: appdev

# Notes for application development

Phosh relies on freedesktop.org and Wayland protocols where possible. Sometimes
we have phosh specific extensions though until changes made it into the spec.

## Desktop Entry Specification
Phosh aims to parse app entries according to the [Desktop Entry Specification][]. For some
mobile needs phosh honors the following additional entries:

* `X-Purism-FormFactor`: Allows app to specify that they are suitable for use on phones.
  Example: `X-Purism-FormFactor=Workstation;Mobile;`. Note that this will be phased
  out in favour of suppying this information [via appstream metadata][].
* `X-KDE-FormFactor`: Similar to the above (used by some KDE applications)
* `X-Geoclue-Reason`: Used to indicate the reason why an application wants to access
  location data.
* `X-Phosh-Lockscreen-Actions`: Used to allow actions to be used on the lock screen.
  Example: `X-Phosh-Lockscreen-Actions=app.stop-alarm::;app.snooze-alarm::;`
* `X-Phosh-UsesFeedback`: Used to indicate that application submit events to feedbackd
  to trigger audio, haptic or led feedback. This is useful for setting applications
  so they can show entries for individual apps. Example: `X-Phosh-UsesFeedback=true`

[Desktop Entry Specification]: https://specifications.freedesktop.org/desktop-entry-spec/latest/
[via appstream metadata]: https://gitlab.gnome.org/World/Phosh/phosh/-/merge_requests/950
