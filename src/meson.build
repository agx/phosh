wl_scanner = find_program('wayland-scanner')
gen_scanner_client_header = generator(wl_scanner,
    output: '@BASENAME@-client-protocol.h',
    arguments: ['client-header', '@INPUT@', '@OUTPUT@'])
gen_scanner_client_code = generator(wl_scanner,
    output: '@BASENAME@-protocol.c',
    arguments: ['code', '@INPUT@', '@OUTPUT@'])

phosh_resources = gnome.compile_resources(
   'phosh-resources',
   'phosh.gresources.xml',

   c_name: 'phosh',
)


wl_protos = [
  'phosh-mobile-shell.xml',
]
wl_proto_deps = []
foreach proto: wl_protos
  wl_proto_deps += gen_scanner_client_header.process('../protocol/' + proto)
  wl_proto_deps += gen_scanner_client_code.process('../protocol/' + proto)
endforeach

phosh_sources = [
  'background.c',
  'background.h',
  'favorites.c',
  'favorites.h',
  'lockscreen.c',
  'lockscreen.h',
  'menu.c',
  'menu.h',
  'panel.c',
  'panel.h',
  'phosh.c',
  'settings.c',
  'settings.h',
  'settings/brightness.c',
  phosh_resources,
] + wl_proto_deps

phosh_deps = [
  dependency('gnome-desktop-3.0'),
  dependency('gtk+-3.0'),
  dependency('gtk+-wayland-3.0'),
  dependency('wayland-client'),
  cc.find_library('m', required: false),
  cc.find_library('rt', required: false),
]

phosh = executable('phosh', phosh_sources,
  dependencies: phosh_deps,
  install: true,
  install_dir: get_option('libexecdir'),
)
