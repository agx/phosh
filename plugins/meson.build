phosh_plugin_header_subdir = 'phosh'

# Pubilc header that can also be used by out of tree plugins
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
  'launcher-box',
  'ticket-box',
  'upcoming-events',
]

lockscreen_plugin_schema_dirs = []
foreach plugin : lockscreen_plugins
  lockscreen_plugin_schema_dirs += meson.project_build_root() / 'plugins' / plugin
endforeach
lockscreen_plugin_schema_path = ':'.join(lockscreen_plugin_schema_dirs)

if get_option('lockscreen-plugins')
  plugin_inc = include_directories('.')
  plugin_prefs_inc = include_directories('.')

  # Like phosh-config.h but for preference plugins
  plugin_prefs_config = configuration_data()
  plugin_prefs_config.set_quoted('GETTEXT_PACKAGE', 'phosh')
  plugin_prefs_config.set_quoted('LOCALEDIR', localedir)
  plugin_prefs_config_h = configure_file(
    output: 'phosh-plugin-prefs-config.h',
    configuration: plugin_prefs_config,
  )

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
    sources: [phosh_plugin_h, plugin_prefs_config_h],
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
