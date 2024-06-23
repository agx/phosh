fs = import('fs')

proto_inc = include_directories('.')

wl_protocol_dir = wayland_protos_dep.get_variable(pkgconfig: 'pkgdatadir')

wayland_scanner = find_program('wayland-scanner')

wl_protos = [
  [wl_protocol_dir, 'stable/xdg-shell/xdg-shell.xml'],
  [wl_protocol_dir, 'staging/ext-idle-notify/ext-idle-notify-v1.xml'],
  [wl_protocol_dir, 'unstable/xdg-output/xdg-output-unstable-v1.xml'],
  ['input-method-unstable-v2.xml'],
  ['phoc-device-state-unstable-v1.xml'],
  ['phoc-layer-shell-effects-unstable-v1.xml'],
  ['phosh-private.xml'],
  ['virtual-keyboard-unstable-v1.xml'],
  ['wlr-foreign-toplevel-management-unstable-v1.xml'],
  ['wlr-gamma-control-unstable-v1.xml'],
  ['wlr-layer-shell-unstable-v1.xml'],
  ['wlr-output-management-unstable-v1.xml'],
  ['wlr-output-power-management-unstable-v1.xml'],
  ['wlr-screencopy-unstable-v1.xml'],
]

wl_proto_headers = []
wl_proto_sources = []

foreach p : wl_protos
  xml = join_paths(p)

  base = fs.name(xml)
  proto = fs.stem(base)

  wl_proto_headers += custom_target('@0@ client header'.format(proto),
				    input: xml,
				    output: '@0@-client-protocol.h'.format(proto),
				    command: [wayland_scanner,
					      'client-header',
					      '@INPUT@',
					      '@OUTPUT@'],
                                    install_dir: lib_inc_dir,
                                    install: bindings_lib,
				   )
  wl_proto_sources += custom_target('@0@ source'.format(proto),
				    input: xml,
				    output: '@0@-protocol.c'.format(proto),
				    command: [wayland_scanner,
					      'private-code',
					      '@INPUT@',
					      '@OUTPUT@']
				   )
endforeach
