calendar_plugin_deps = [
  dependency('gobject-2.0'),
  dependency('gtk+-3.0'),
]

calendar_plugin_sources = files(
  'phosh-plugin-calendar.c',
)

phosh_calendar_plugin = shared_module(
  'phosh-plugin-calendar',
  calendar_plugin_sources,
  dependencies: calendar_plugin_deps,
  include_directories: phosh_inc,
  install: true,
  install_dir: plugins_dir,
)
