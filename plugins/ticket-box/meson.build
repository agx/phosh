name='ticket-box'

ticket_box_plugin_deps = [
  dependency('gobject-2.0'),
  dependency('gtk+-3.0'),
  dependency('libhandy-1'),
  dependency('evince-document-3.0'),
  dependency('evince-view-3.0'),
]

ticket_box_resources = gnome.compile_resources(
  'phosh-plugin-ticket-box-resources',
  'phosh-plugin-ticket-box.gresources.xml',
  c_name: 'phosh_plugin_ticket_box',
)

ticket_box_plugin_sources = files(
  'phosh-plugin-ticket-box.c',
  'ticket.c',
  'ticket.h',
  'ticket-box.c',
  'ticket-box.h',
  'ticket-row.c',
  'ticket-row.h',
)

phosh_ticket_box_plugin = shared_module(
  'phosh-plugin-ticket-box',
  ticket_box_plugin_sources,
  ticket_box_resources,
  c_args : ['-DG_LOG_DOMAIN="phosh-plugin-ticket-box"'],
  dependencies: ticket_box_plugin_deps,
  include_directories: [root_inc, phosh_gtk_list_models_inc],
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
