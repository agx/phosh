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
