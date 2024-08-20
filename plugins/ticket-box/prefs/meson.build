ticket_box_prefs_resources = gnome.compile_resources(
  'phosh-plugin-prefs-ticket-box-resources',
  'phosh-plugin-prefs-ticket-box.gresources.xml',
  c_name: 'phosh_plugin_prefs_ticket_box',
)

ticket_box_plugin_prefs_sources = files(
  'phosh-plugin-prefs-ticket-box.c',
  'ticket-box-prefs.c',
  'ticket-box-prefs.h',
)

phosh_ticket_box_plugin_prefs = shared_module(
  'phosh-plugin-prefs-ticket-box',
  ticket_box_plugin_prefs_sources,
  ticket_box_prefs_resources,
  c_args : ['-DG_LOG_DOMAIN="phosh-plugin-prefs-ticket-box"',
            '-DPLUGIN_PREFS_NAME="@0@-prefs"'.format(name),
            '-DADW_VERSION_MIN_REQUIRED=@0@'.format(adw_ver_str),
           ],
  dependencies: plugin_prefs_dep,
  install: true,
  install_dir: plugin_prefs_dir,
)
