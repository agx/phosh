upcoming_events_prefs_resources = gnome.compile_resources(
  'phosh-plugin-prefs-upcoming-events-resources',
  'phosh-plugin-prefs-upcoming-events.gresources.xml',
  c_name: 'phosh_plugin_prefs_upcoming_events',
)

upcoming_events_plugin_prefs_sources = files(
  'phosh-plugin-prefs-upcoming-events.c',
  'upcoming-events-prefs.c',
  'upcoming-events-prefs.h',
)

phosh_upcoming_events_plugin_prefs = shared_module(
  'phosh-plugin-prefs-upcoming-events',
  upcoming_events_plugin_prefs_sources,
  upcoming_events_prefs_resources,
  c_args : ['-DG_LOG_DOMAIN="phosh-plugin-prefs-upcoming-events"',
            '-DPLUGIN_PREFS_NAME="@0@-prefs"'.format(name),
            '-DADW_VERSION_MIN_REQUIRED=@0@'.format(adw_ver_str),
           ],
  dependencies: plugin_prefs_dep,
  install: true,
  install_dir: plugin_prefs_dir,
)
