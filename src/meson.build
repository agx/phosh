subdir('wwan')
subdir('monitor')

wayland_protos = dependency('wayland-protocols', version: '>=1.12')
wl_protocol_dir = wayland_protos.get_pkgconfig_variable('pkgdatadir')

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
  '/'.join([wl_protocol_dir, 'stable/xdg-shell/xdg-shell.xml']),
  '../protocol/idle.xml',
  '../protocol/phosh-private.xml',
  '../protocol/wlr-input-inhibitor-unstable-v1.xml',
  '../protocol/wlr-layer-shell-unstable-v1.xml',
]
wl_proto_sources = []
foreach proto: wl_protos
  wl_proto_sources += gen_scanner_client_header.process(proto)
  wl_proto_sources += gen_scanner_client_code.process(proto)
endforeach

phosh_sources = [
  'auth.c',
  'auth.h',
  'background.c',
  'background.h',
  'batteryinfo.c',
  'batteryinfo.h',
  'favorites.c',
  'favorites.h',
  'layersurface.c',
  'layersurface.h',
  'lockscreen.c',
  'lockscreen.h',
  'lockshield.c',
  'lockshield.h',
  'monitor-manager.c',
  'monitor-manager.h',
  'panel.c',
  'panel.h',
  'phosh.c',
  'settings.c',
  'settings.h',
  'settings/brightness.c',
  'wwaninfo.c',
  'wwaninfo.h',
  phosh_resources,
  phosh_wwan_sources,
  phosh_monitor_sources,
  wl_proto_sources,
]

phosh_deps = [
  dependency('gnome-desktop-3.0', version: '>=3.26'),
  dependency('gtk+-3.0', version: '>=3.22'),
  dependency('gtk+-wayland-3.0', version: '>=3.22'),
  dependency('wayland-client', version: '>=1.14'),
  dependency('libhandy-0.0', version: '>=0.0.1'),
  dependency('upower-glib', version: '>=0.99.1'),
  cc.find_library('pam', required: true),
  cc.find_library('m', required: false),
  cc.find_library('rt', required: false),
]

phosh = executable('phosh', phosh_sources,
  dependencies: phosh_deps,
  install: true,
  install_dir: get_option('libexecdir'),
)

