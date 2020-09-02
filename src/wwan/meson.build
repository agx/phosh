mm_iface = 'phosh-wwan-mm-dbus'
ofono_iface = 'phosh-wwan-ofono-dbus'
generated_wwan_sources = [
  gnome.gdbus_codegen(
    mm_iface,
    mm_iface + '.xml',
    namespace: 'Phosh_MM_DBus',
    interface_prefix: 'org.freedesktop.ModemManager1',
    object_manager: true
  ),
  gnome.gdbus_codegen(
    ofono_iface,
    ofono_iface + '.xml',
    namespace: 'PhoshOfonoDBus',
    interface_prefix: 'org.ofono',
    object_manager: false
  )
]

phosh_wwan_sources = [
  'wwan/phosh-wwan-ofono.c',
  'wwan/phosh-wwan-ofono.h',
  'wwan/phosh-wwan-mm.c',
  'wwan/phosh-wwan-mm.h',
  'wwan/phosh-wwan-iface.c',
  'wwan/phosh-wwan-iface.h',
  generated_wwan_sources,
]
