iface = 'phosh-osk0-dbus'
generated_osk_sources = gnome.gdbus_codegen(iface,
					     iface + '.xml',
					     namespace: 'PhoshOsk0',
					     object_manager: true)

phosh_osk_sources = files(
  'osk-button.c',
) + [
  generated_osk_sources,
]
