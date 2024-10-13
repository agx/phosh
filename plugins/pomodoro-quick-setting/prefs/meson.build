pomodoro_quick_setting_prefs_resources = gnome.compile_resources(
  'phosh-plugin-prefs-pomodoro-quick-setting-resources',
  'phosh-plugin-prefs-pomodoro-quick-setting.gresources.xml',
  c_name: 'phosh_plugin_prefs_pomodoro_quick_setting',
)

pomodoro_quick_setting_plugin_prefs_sources = files(
  'phosh-plugin-prefs-pomodoro-quick-setting.c',
  'pomodoro-quick-setting-prefs.c',
  'pomodoro-quick-setting-prefs.h',
)

phosh_pomodoro_quick_setting_plugin_prefs = shared_module(
  'phosh-plugin-prefs-pomodoro-quick-setting',
  pomodoro_quick_setting_plugin_prefs_sources,
  pomodoro_quick_setting_prefs_resources,
  c_args : ['-DG_LOG_DOMAIN="phosh-plugin-prefs-pomodoro-quick-setting"',
            '-DPLUGIN_PREFS_NAME="@0@-prefs"'.format(name),
            '-DADW_VERSION_MIN_REQUIRED=@0@'.format(adw_ver_str),
           ],
  dependencies: plugin_prefs_dep,
  install: true,
  install_dir: plugin_prefs_dir,
)
