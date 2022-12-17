if get_option('lockscreen-plugins')

subdir('calendar')
subdir('upcoming-events')
subdir('ticket-box')

endif

pkgconfig.generate(
  variables : ['lockscreen_plugins_dir=@0@'.format(plugins_dir),
              ],
  subdirs : 'phosh',
  version : meson.project_version(),
  install_dir : libdir / 'pkgconfig',
  filebase : 'phosh-plugins',
  name : 'phosh-plugins',
  description : 'Phosh plugin configuration data',
)
