generated_monitor_sources = gnome.gdbus_codegen(
  'phosh-display-dbus',
  'org.gnome.Mutter.DisplayConfig.xml',
  interface_prefix: 'org.gnome.Mutter',
  namespace: 'PhoshDisplayDbus',
)

phosh_monitor_sources = [
  'monitor/monitor.c',
  'monitor/monitor.h',
  generated_monitor_sources,
]
