phosh_plugin_header_subdir = 'phosh'

phosh_plugin_config = configuration_data()
phosh_plugin_config.set_quoted('PHOSH_PLUGIN_EXTENSION_POINT_LOCKSCREEN_WIDGET',
                               'phosh-lockscreen-widget')
phosh_plugin_config.set_quoted('PHOSH_PLUGIN_EXTENSION_POINT_LOCKSCREEN_WIDGET_PREFS',
                               'phosh-lockscreen-widget-prefs')
phosh_plugin_h = configure_file(
  input: 'phosh-plugin.h.in',
  output: 'phosh-plugin.h',
  configuration: phosh_plugin_config,
)
install_headers([phosh_plugin_h], subdir: phosh_plugin_header_subdir)

pkgconfig.generate(
  variables : ['lockscreen_plugins_dir=@0@'.format(plugins_dir),
               'lockscreen_prefs_dir=@0@'.format(plugin_prefs_dir),
              ],
  subdirs : phosh_plugin_header_subdir,
  version : meson.project_version(),
  install_dir : libdir / 'pkgconfig',
  filebase : 'phosh-plugins',
  name : 'phosh-plugins',
  description : 'Phosh plugin configuration data',
)

lockscreen_plugins = [
  'calendar',
  'emergency-info',
  'ticket-box',
  'upcoming-events',
]

if get_option('lockscreen-plugins')

  plugin_inc = include_directories('.')

  plugin_dep = declare_dependency(
    sources: phosh_plugin_h,
    include_directories: plugin_inc,
    dependencies: [
      dependency('gobject-2.0'),
      dependency('gtk+-3.0'),
      dependency('libhandy-1'),
    ],
  )

  plugin_prefs_dep = declare_dependency(
    sources: phosh_plugin_h,
    include_directories: plugin_inc,
    dependencies: [
      dependency('gtk4'),
      dependency('libadwaita-1'),
    ],
  )

  foreach plugin : lockscreen_plugins
    subdir(plugin)
  endforeach

endif
