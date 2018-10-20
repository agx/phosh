generated_idle_sources = gnome.gdbus_codegen(
  'phosh-idle-dbus',
  'org.gnome.Mutter.IdleMonitor.xml',
  interface_prefix: 'org.gnome.Mutter.IdleMonitor.',
  object_manager: true,
  namespace: 'PhoshIdleDbus',
)

phosh_idle_sources = [
  generated_idle_sources,
]
