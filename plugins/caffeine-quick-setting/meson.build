name='caffeine-quick-setting'

caffeine_quick_setting_resources = gnome.compile_resources(
  'phosh-plugin-caffeine-quick-setting-resources',
  'phosh-plugin-caffeine-quick-setting.gresources.xml',
  c_name: 'phosh_plugin_caffeine_quick_setting',
)

caffeine_quick_setting_plugin_sources = files(
  'phosh-plugin-caffeine-quick-setting.c',
  'caffeine-quick-setting.c',
)

phosh_caffeine_quick_setting_plugin = shared_module(
  'phosh-plugin-caffeine-quick-setting',
  caffeine_quick_setting_plugin_sources,
  caffeine_quick_setting_resources,
  c_args : ['-DG_LOG_DOMAIN="phosh-plugin-@0@"'.format(name),
            '-DPLUGIN_NAME="@0@"'.format(name),
           ],
  dependencies: plugin_dep,
  include_directories: phosh_inc,
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
  po_dir:  join_paths(meson.project_source_root(), 'po'),
  install: true,
  install_dir: plugins_dir,
  type: 'desktop'
)
