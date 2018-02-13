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


shell_proto_header = gen_scanner_client_header.process('../protocol/phosh-mobile-shell.xml')
shell_proto_code = gen_scanner_client_code.process('../protocol/phosh-mobile-shell.xml')

phosh_sources = [
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
  shell_proto_header,
  shell_proto_code,
  phosh_resources,
]

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
