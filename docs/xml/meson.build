url = 'https://gitlab.gnome.org/World/Phosh/phosh'

ent_conf = configuration_data()
ent_conf.set('PACKAGE', 'Phosh')
ent_conf.set('PACKAGE_BUGREPORT', url + '/issues')
ent_conf.set('PACKAGE_NAME', 'Phosh')
ent_conf.set('PACKAGE_STRING', 'Phosh')
ent_conf.set('PACKAGE_TARNAME', 'phosh-' + meson.project_version())
ent_conf.set('PACKAGE_URL', url)
ent_conf.set('PACKAGE_VERSION', meson.project_version())
configure_file(input: 'gtkdocentities.ent.in', output: 'gtkdocentities.ent', configuration: ent_conf)

