# DBus client interfaces
generated_dbus_sources = []

# iio-sensor-proxy
generated_dbus_sources += gnome.gdbus_codegen('iio-sensor-proxy-dbus',
					      'net.hadess.SensorProxy.xml',
					      interface_prefix: 'net.hadess',
					      namespace: 'PhoshDBus')
# Geoclue
generated_dbus_sources += gnome.gdbus_codegen('geoclue-manager-dbus',
                                              'org.freedesktop.GeoClue2.Manager.xml',
                                              interface_prefix: 'org.freedesktop.GeoClue2',
                                              namespace: 'PhoshGeoClueDBus')
# org.freedesktop.hostname1
generated_dbus_sources += gnome.gdbus_codegen('hostname1-dbus',
					      'org.freedesktop.hostname1.xml',
					      interface_prefix: 'org.freedesktop',
					      namespace: 'PhoshHostname1DBus')
# org.freedesktop.login1.Session
generated_dbus_sources += gnome.gdbus_codegen('login1-session-dbus',
					      'org.freedesktop.login1.Session.xml',
					      interface_prefix: 'org.freedesktop.login1',
					      namespace: 'PhoshLogin1SessionDBus')
# org.freedesktop.login1.Manager
generated_dbus_sources += gnome.gdbus_codegen('login1-manager-dbus',
					      'org.freedesktop.login1.Manager.xml',
					      interface_prefix: 'org.freedesktop.login1',
					      namespace: 'PhoshLogin1ManagerDBus')

generated_dbus_sources +=  gnome.gdbus_codegen('phosh-wwan-mm-dbus',
                                               'org.freedesktop.ModemManager1.xml',
                                               namespace: 'Phosh_MM_DBus',
                                               interface_prefix: 'org.freedesktop.ModemManager1',
                                               object_manager: true)
# org.freedesktop.UPower.Torch
generated_dbus_sources += gnome.gdbus_codegen('upower-torch-dbus',
					      'org.freedesktop.UPower.Torch.xml',
					      interface_prefix: 'org.freedesktop.UPower',
					      namespace: 'PhoshUPowerDBus')
# org.gnome.SessionManager
generated_dbus_sources += gnome.gdbus_codegen('gnome-session-dbus',
					      'org.gnome.SessionManager.xml',
					      interface_prefix: 'org.gnome',
					      namespace: 'PhoshSessionDBus')
# org.gnome.SessionManager.ClientPrivate
generated_dbus_sources += gnome.gdbus_codegen('gnome-session-client-private-dbus',
					      'org.gnome.SessionManager.ClientPrivate.xml',
					      interface_prefix: 'org.gnome.SessionManager',
					      namespace: 'PhoshSessionClientPrivateDBus')
# org.gnome.SessionManager.Presence
generated_dbus_sources += gnome.gdbus_codegen('gnome-session-presence-dbus',
					      'org.gnome.SessionManager.Presence.xml',
					      interface_prefix: 'org.gnome.SessionManager',
					      namespace: 'PhoshSessionPresenceDBus')
# org.gnome.SettingsDaemon.Rfkill
generated_dbus_sources += gnome.gdbus_codegen('gsd-rfkill-dbus',
					      'org.gnome.SettingsDaemon.Rfkill.xml',
					      interface_prefix: 'org.gnome.SettingsDaemon',
					      namespace: 'PhoshRfkillDBus')
# MPris2
generated_dbus_sources += gnome.gdbus_codegen('mpris-dbus',
					      'org.mpris.MediaPlayer2.xml',
					      interface_prefix: 'org.mpris',
					      namespace: 'PhoshMprisDBus')

generated_dbus_sources += gnome.gdbus_codegen('phosh-wwan-ofono-dbus',
                                              'org.ofono.xml',
                                              namespace: 'PhoshOfonoDBus',
                                              interface_prefix: 'org.ofono',
                                              object_manager: false)
# DBus server protocols
generated_dbus_sources += gnome.gdbus_codegen('geoclue-agent-dbus',
                                              'org.freedesktop.GeoClue2.Agent.xml',
                                              interface_prefix: 'org.freedesktop.Agent',
                                              namespace: 'PhoshGeoClueDBus')

generated_dbus_sources += gnome.gdbus_codegen('notify-dbus',
                                              'org.freedesktop.Notifications.xml',
					      interface_prefix: 'org.freedesktop',
					      namespace: 'PhoshNotifyDBus')

generated_dbus_sources += gnome.gdbus_codegen('phosh-gnome-shell-dbus',
					      'org.gnome.Shell.xml',
					      interface_prefix: 'org.gnome',
					      namespace: 'PhoshGnomeShellDBus')

generated_dbus_sources += gnome.gdbus_codegen('phosh-idle-dbus',
					      'org.gnome.Mutter.IdleMonitor.xml',
					      interface_prefix: 'org.gnome.Mutter',
					      object_manager: true,
					      namespace: 'PhoshIdleDBus')

generated_dbus_sources += gnome.gdbus_codegen('phosh-screen-saver-dbus',
                                              'org.gnome.ScreenSaver.xml',
					      interface_prefix: 'org.gnome',
					      namespace: 'PhoshScreenSaverDBus')

generated_dbus_sources += gnome.gdbus_codegen('phosh-screenshot-dbus',
                                              'org.gnome.Shell.Screenshot.xml',
					      interface_prefix: 'org.gnome.Shell',
					      namespace: 'PhoshDBus')

generated_dbus_sources += gnome.gdbus_codegen('phosh-end-session-dialog-dbus',
                                              'org.gnome.SessionManager.EndSessionDialog.xml',
					      interface_prefix: 'org.gnome.SessionManager',
					      namespace: 'PhoshDBus')
