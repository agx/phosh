emergency_info_plugin_prefs_deps = [
  dependency('gtk4'),
  dependency('libadwaita-1'),
]

emergency_info_prefs_resources = gnome.compile_resources(
  'phosh-plugin-prefs-emergency-info-resources',
  'phosh-plugin-prefs-emergency-info.gresources.xml',
  c_name: 'phosh_plugin_prefs_emergency_info',
)

emergency_info_plugin_prefs_sources = files(
  'phosh-plugin-prefs-emergency-info.c',
  'emergency-info-prefs.c',
  'emergency-info-prefs.h',
  'emergency-info-prefs-row.c',
  'emergency-info-prefs-row.h',
)

phosh_emergency_info_plugin_prefs = shared_module(
  'phosh-plugin-prefs-emergency-info',
  emergency_info_plugin_prefs_sources,
  emergency_info_prefs_resources,
  include_directories: [emergency_info_inc, plugin_prefs_inc],
  c_args : ['-DG_LOG_DOMAIN="phosh-plugin-prefs-emergency-info-prefs"',
            '-DPLUGIN_PREFS_NAME="@0@-prefs"'.format(name),
           ],
  dependencies: emergency_info_plugin_prefs_deps,
  install: true,
  install_dir: plugin_prefs_dir,
)
