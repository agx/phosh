configure_file(
  input: 'run_tool.in',
  output: 'run_tool',
  configuration: run_data)

osk_stub_deps = [
  gio_dep,
  gtk_dep,
  gtk_wayland_dep,
  libhandy_dep,
  wayland_client_dep,
  cc.find_library('m', required: false),
]

phosh_osk_stub_resources = gnome.compile_resources(
  'phosh-osk-stub-resources',
  'phosh-osk-stub.gresources.xml',

  c_name: 'pos',
)

executable('phosh-osk-stub', ['phosh-osk-stub.c',
			      'pos-input-surface.h',
			      'pos-input-surface.c',
			     ] +
			     phosh_osk_stub_resources +
			     phosh_layer_surface_sources +
			     wl_proto_sources,
           include_directories: [root_inc, phosh_inc],
           install: get_option('osk-stub'),
           dependencies: osk_stub_deps)

if get_option('tools')

  executable('app-scroll', ['app-scroll.c'] + test_stub_deps,
             dependencies: phosh_tool_dep)

  executable('app-grid-standalone', ['app-grid-standalone.c'] + test_stub_deps,
             dependencies: phosh_tool_dep)

  executable('image-notify', ['image-notify.c'] + test_stub_deps,
             dependencies: phosh_tool_dep)

  executable('app-buttons', ['app-buttons.c'] + test_stub_deps,
             dependencies: phosh_tool_dep)

  executable('notify-blocks', ['notify-blocks.c'],
             dependencies: phosh_tool_dep)

  executable('notify-server-standalone', ['notify-server-standalone.c'] + test_stub_deps,
             dependencies: phosh_tool_dep)

  executable('dump-app-list', ['dump-app-list.c'],
             dependencies: phosh_tool_dep)

endif
