desktopconf = configuration_data()
desktopconf.set('bindir', bindir)
desktopconf.set('libexecdir', libexecdir)

desktop_files = [
  'sm.puri.Phosh.desktop',
  'sm.puri.OSK0.desktop',
]
foreach desktop_file : desktop_files
  i18n.merge_file('desktop',
    input: configure_file(
      input: desktop_file + '.in.in',
      output: desktop_file + '.in',
      configuration: desktopconf
    ),
    output: desktop_file,
    po_dir: '../po',
    install: true,
    install_dir: desktopdir,
    type: 'desktop'
  )
endforeach

sessions = [
  'phosh.session',
]
foreach session: sessions
  desktop = session + '.desktop'
  i18n.merge_file(
    session,
    input: configure_file(
      input: desktop + '.in.in',
      output: desktop + '.in',
      configuration: desktopconf
    ),
    output: session,
    po_dir: '../po',
    install: true,
    install_dir: join_paths(sessiondir, 'sessions'),
    type: 'desktop',
  )
endforeach

runconf = configuration_data()
runconf.set('bindir', bindir)
runconf.set('libexecdir', libexecdir)
runconf.set('pkgdatadir', pkgdatadir)
runconf.set('version', meson.project_version())
runconf.set('wlrootsdir', join_paths(libexecdir, 'wlroots'))
configure_file(
  input: 'phosh.in',
  output: 'phosh',
  install_dir: bindir,
  configuration: runconf,
  install: true
)

schemasdir = 'share/glib-2.0/schemas'

#generate XML enum definitions for GSettings schema
schema_enum_headers = files(
  '../src/wwan/phosh-wwan-backend.h'
)
generate_enums_schema = gnome.mkenums('sm.puri.phosh.enums.xml',
  sources: schema_enum_headers,
  comments: '<!-- @comment@ -->',
  fhead: '<schemalist>',
  vhead: '  <@type@ id="sm.puri.phosh.@EnumName@">',
  vprod: '    <value nick="@valuenick@" value="@valuenum@"/>',
  vtail: '  </@type@>',
  ftail: '</schemalist>',
  install_header: true,
  install_dir: schemasdir
)

#workaround due to https://github.com/mesonbuild/meson/issues/1687
copy_schema = custom_target('copy-gschema-to-builddir',
  input: 'sm.puri.phosh.gschema.xml',
  output: 'sm.puri.phosh.gschema.xml',
  command: ['cp', '@INPUT@', '@OUTPUT@']
)
schemas = ['sm.puri.phosh.gschema.xml']
compile_schemas = custom_target('glib-compile-schemas',
  build_by_default: true,
  output: 'gschemas.compiled',
  install: false,
  command: [find_program('glib-compile-schemas'),
    meson.current_build_dir()
  ],
  depends: [generate_enums_schema, copy_schema]
)

install_data(
  schemas,
  install_dir: schemasdir
)

install_data('phoc.ini', install_dir : pkgdatadir)
install_data('wayland-sessions/phosh.desktop',
	     install_dir : 'share/wayland-sessions')

