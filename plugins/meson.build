phosh_plugin_header_subdir = 'phosh'

phosh_plugin_config = configuration_data()
phosh_plugin_config.set_quoted('PHOSH_PLUGIN_EXTENSION_POINT_LOCKSCREEN_WIDGET', 'phosh-lockscreen-widget')
phosh_plugin_h = configure_file(
  input: 'phosh-plugin.h.in',
  output: 'phosh-plugin.h',
  configuration: phosh_plugin_config,
)
install_headers([phosh_plugin_h], subdir: phosh_plugin_header_subdir)

if get_option('lockscreen-plugins')

plugin_inc = include_directories('.')

subdir('calendar')
subdir('upcoming-events')
subdir('ticket-box')
subdir('emergency-info')

endif

pkgconfig.generate(
  variables : ['lockscreen_plugins_dir=@0@'.format(plugins_dir),
              ],
  subdirs : phosh_plugin_header_subdir,
  version : meson.project_version(),
  install_dir : libdir / 'pkgconfig',
  filebase : 'phosh-plugins',
  name : 'phosh-plugins',
  description : 'Phosh plugin configuration data',
)
