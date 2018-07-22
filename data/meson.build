desktopconf = configuration_data()
desktopconf.set('bindir', bindir)
desktopconf.set('libexecdir', libexecdir)

desktop_files = [
  'sm.puri.Phosh.desktop',
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
