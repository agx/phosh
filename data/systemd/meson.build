gsd_wants = ''
foreach component : gsd_required_components
  gsd_wants += 'Wants=' + component + '.target\n'
endforeach

session_dropins = [
  'gnome-session@phosh.target.d',
]

sessionconf = configuration_data()
sessionconf.set('gsd_wants', gsd_wants)

foreach session_dropin : session_dropins
  configure_file(
    input: 'phosh.session.conf.in',
    output: 'session.conf',
    install_dir: join_paths(systemduserdir, session_dropin),
    configuration: sessionconf,
    install: true
  )
endforeach

serviceconf = configuration_data()
serviceconf.set('libexecdir', libexecdir)
configure_file(
  input: 'sm.puri.Phosh.service.in',
  output: 'sm.puri.Phosh.service',
  install_dir: systemduserdir,
  configuration: serviceconf,
  install: true
)

install_data('sm.puri.Phosh.target', install_dir: systemduserdir)

