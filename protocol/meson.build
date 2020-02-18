wayland_protos = dependency('wayland-protocols', version: '>=1.12')
wl_protocol_dir = wayland_protos.get_pkgconfig_variable('pkgdatadir')

wl_scanner = find_program('wayland-scanner')
gen_scanner_client_header = generator(wl_scanner,
    output: '@BASENAME@-client-protocol.h',
    arguments: ['client-header', '@INPUT@', '@OUTPUT@'])
gen_scanner_client_code = generator(wl_scanner,
    output: '@BASENAME@-protocol.c',
    arguments: ['private-code', '@INPUT@', '@OUTPUT@'])

wl_protos = [
  '/'.join([wl_protocol_dir, 'stable/xdg-shell/xdg-shell.xml']),
  '/'.join([wl_protocol_dir, 'unstable/xdg-output/xdg-output-unstable-v1.xml']),
  'gamma-control.xml',
  'idle.xml',
  'wlr-screencopy-unstable-v1.xml',
  'phosh-private.xml',
  'wlr-foreign-toplevel-management-unstable-v1.xml',
  'wlr-input-inhibitor-unstable-v1.xml',
  'wlr-layer-shell-unstable-v1.xml',
  'wlr-output-management-unstable-v1.xml',
  'wlr-output-power-management-unstable-v1.xml',
]
wl_proto_sources = []
foreach proto: wl_protos
  wl_proto_sources += gen_scanner_client_header.process(proto)
  wl_proto_sources += gen_scanner_client_code.process(proto)
endforeach
