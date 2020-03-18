iface = 'phosh-wwan-mm-dbus'
generated_wwan_sources = gnome.gdbus_codegen(iface,
					     iface + '.xml',
					     namespace: 'PhoshMMDBus',
					     interface_prefix: 'org.freedesktop.ModemManager1',
					     object_manager: true)

phosh_wwan_sources = [
  'wwan/phosh-wwan-mm.c',
  'wwan/phosh-wwan-mm.h',
  'wwan/phosh-wwan-iface.c',
  'wwan/phosh-wwan-iface.h',
  generated_wwan_sources,
]
