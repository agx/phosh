# DBus client interfaces
generated_dbus_sources = []


# iio-sensor-proxy
generated_dbus_sources += gnome.gdbus_codegen('iio-sensor-proxy-dbus',
					      'net.hadess.SensorProxy.xml',
					      interface_prefix: 'net.hadess',
					      namespace: 'PhoshDbus')
# org.gnome.SessionManager.Presence
generated_dbus_sources += gnome.gdbus_codegen('gnome-session-presence-dbus',
					      'org.gnome.SessionManager.Presence.xml',
					      interface_prefix: 'org.gnome.SessionManager',
					      namespace: 'PhoshSessionPresenceDbus')
# org.gnome.SettingsDaemon.Rfkill
generated_dbus_sources += gnome.gdbus_codegen('gsd-rfkill-dbus',
					      'org.gnome.SettingsDaemon.Rfkill.xml',
					      interface_prefix: 'org.gnome.SettingsDaemon',
					      namespace: 'PhoshRfkillDbus')
# MPris2
generated_dbus_sources += gnome.gdbus_codegen('mpris-dbus',
					      'org.mpris.MediaPlayer2.xml',
					      interface_prefix: 'org.mpris',
					      namespace: 'PhoshMprisDBus')

# DBus server protocols
generated_dbus_sources += gnome.gdbus_codegen('notify-dbus',
                                              'org.freedesktop.Notifications.xml',
					      interface_prefix: 'org.freedesktop',
					      namespace: 'PhoshNotifyDbus')

generated_dbus_sources += gnome.gdbus_codegen('phosh-idle-dbus',
					     'org.gnome.Mutter.IdleMonitor.xml',
					     interface_prefix: 'org.gnome.Mutter',
					     object_manager: true,
					     namespace: 'PhoshIdleDbus')

generated_dbus_sources += gnome.gdbus_codegen('phosh-screen-saver-dbus',
                                              'org.gnome.ScreenSaver.xml',
					      interface_prefix: 'org.gnome',
					      namespace: 'PhoshScreenSaverDbus')
