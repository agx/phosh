name='calendar'

calendar_plugin_deps = [
  dependency('gobject-2.0'),
  dependency('gtk+-3.0'),
]

calendar_resources = gnome.compile_resources(
  'phosh-plugin-calendar-resources',
  'phosh-plugin-calendar.gresources.xml',
  c_name: 'phosh_plugin_calendar',
)

calendar_plugin_sources = files(
  'phosh-plugin-calendar.c',
  'calendar.c',
)

phosh_calendar_plugin = shared_module(
  'phosh-plugin-calendar',
  calendar_plugin_sources,
  calendar_resources,
  dependencies: calendar_plugin_deps,
  include_directories: [phosh_inc, plugin_inc],
  install: true,
  install_dir: plugins_dir,
)

pluginconf = configuration_data()
pluginconf.set('name', name)
pluginconf.set('plugins_dir', plugins_dir)

i18n.merge_file(
  input: configure_file(
    input: name + '.desktop.in.in',
    output: name + '.desktop.in',
    configuration: pluginconf,
  ),
  output: name + '.plugin',
  po_dir:  join_paths(meson.source_root(), 'po'),
  install: true,
  install_dir: plugins_dir,
  type: 'desktop'
)
