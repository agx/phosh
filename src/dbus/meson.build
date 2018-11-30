# DBus client interfaces
generated_dbus_sources = []

# org.gnome.SessionManager.Presence
generated_dbus_sources += gnome.gdbus_codegen('gnome-session-presence-dbus',
					      'org.gnome.SessionManager.Presence.xml',
					      interface_prefix: 'org.gnome.SessionManager',
					      namespace: 'PhoshSessionPresenceDbus')

# DBus server protocole
generated_dbus_sources += gnome.gdbus_codegen('phosh-idle-dbus',
					     'org.gnome.Mutter.IdleMonitor.xml',
					     interface_prefix: 'org.gnome.Mutter.IdleMonitor.',
					     object_manager: true,
					     namespace: 'PhoshIdleDbus')
