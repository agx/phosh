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
runconf.set('wlrootsdir', join_paths(libexecdir, 'wlroots'))
configure_file(
  input: 'phosh.in',
  output: 'phosh',
  install_dir: bindir,
  configuration: runconf,
  install: true
)

schemas = ['sm.puri.phosh.gschema.xml']
compiled = gnome.compile_schemas(
  build_by_default: true
)

install_data(
  schemas,
  install_dir: 'share/glib-2.0/schemas'
)

install_data('phoc.ini', install_dir : pkgdatadir)
install_data('wayland-sessions/phosh.desktop',
	     install_dir : 'share/wayland-sessions')

