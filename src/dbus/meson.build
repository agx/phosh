dbus_inc = include_directories('.')

# DBus client interfaces
generated_dbus_sources = []
dbus_doc_targets = []

dbus_prefix = 'PhoshDBus'

# iio-sensor-proxy
generated_dbus_sources += gnome.gdbus_codegen('iio-sensor-proxy-dbus',
					      'net.hadess.SensorProxy.xml',
					      interface_prefix: 'net.hadess',
					      namespace: dbus_prefix)
# Geoclue
generated_dbus_sources += gnome.gdbus_codegen('geoclue-manager-dbus',
                                              'org.freedesktop.GeoClue2.Manager.xml',
                                              interface_prefix: 'org.freedesktop.GeoClue2',
                                              namespace: 'PhoshGeoClueDBus')
# org.freedesktop.hostname1
generated_dbus_sources += gnome.gdbus_codegen('hostname1-dbus',
					      'org.freedesktop.hostname1.xml',
					      interface_prefix: 'org.freedesktop',
					      namespace: dbus_prefix)
# org.freedesktop.impl.portal
generated_dbus_sources += gnome.gdbus_codegen('portal-dbus',
					      'org.freedesktop.impl.portal.xml',
					      interface_prefix: 'org.freedesktop',
					      namespace: dbus_prefix)
# org.freedesktop.login1.Session
generated_dbus_sources += gnome.gdbus_codegen('login1-session-dbus',
					      'org.freedesktop.login1.Session.xml',
					      interface_prefix: 'org.freedesktop.login1',
					      namespace: dbus_prefix)
# org.freedesktop.login1.Manager
generated_dbus_sources += gnome.gdbus_codegen('login1-manager-dbus',
					      'org.freedesktop.login1.Manager.xml',
					      interface_prefix: 'org.freedesktop.login1',
					      namespace: dbus_prefix)

generated_dbus_sources +=  gnome.gdbus_codegen('phosh-wwan-mm-dbus',
                                               'org.freedesktop.ModemManager1.xml',
                                               namespace: 'Phosh_MM_DBus',
                                               interface_prefix: 'org.freedesktop.ModemManager1',
                                               object_manager: true)
generated_dbus_sources += gnome.gdbus_codegen('calls-dbus',
                                              'org.gnome.Calls.Call.xml',
                                              object_manager: true,
                                              interface_prefix: 'org.gnome',
                                              namespace: 'PhoshCallsDBus')
generated_dbus_sources += gnome.gdbus_codegen('calls-emergency-dbus',
                                              'org.gnome.Calls.EmergencyCalls.xml',
                                              interface_prefix: 'org.gnome.Calls',
                                              namespace: 'Phosh')
# org.gnome.SessionManager
generated_dbus_sources += gnome.gdbus_codegen('gnome-session-dbus',
					      'org.gnome.SessionManager.xml',
					      interface_prefix: 'org.gnome',
					      namespace: dbus_prefix)
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
# org.gnome.SettingsDaemon.Color
generated_dbus_sources += gnome.gdbus_codegen('gsd-color-dbus',
					      'org.gnome.SettingsDaemon.Color.xml',
					      interface_prefix: 'org.gnome.SettingsDaemon',
					      namespace: dbus_prefix)
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
                                              interface_prefix: 'org.ofono')

generated_dbus_sources += gnome.gdbus_codegen('phosh-osk0-dbus',
                                              'sm.puri.OSK0.xml',
                                              namespace: 'PhoshOsk0')

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
					      interface_prefix: 'org',
					      namespace: dbus_prefix)

generated_dbus_sources += gnome.gdbus_codegen('phosh-display-dbus',
                                              'org.gnome.Mutter.DisplayConfig.xml',
                                              interface_prefix: 'org.gnome.Mutter',
                                              namespace: dbus_prefix)

generated_dbus_sources += gnome.gdbus_codegen('phosh-idle-dbus',
					      'org.gnome.Mutter.IdleMonitor.xml',
					      interface_prefix: 'org.gnome.Mutter',
					      object_manager: true,
					      namespace: 'PhoshIdleDBus')

generated_dbus_sources += gnome.gdbus_codegen('phosh-screen-saver-dbus',
                                              'org.gnome.ScreenSaver.xml',
					      interface_prefix: 'org.gnome',
					      namespace: dbus_prefix)

generated_dbus_sources += gnome.gdbus_codegen('phosh-screenshot-dbus',
                                              'org.gnome.Shell.Screenshot.xml',
					      interface_prefix: 'org.gnome.Shell',
					      namespace: dbus_prefix)

generated_dbus_sources += gnome.gdbus_codegen('phosh-end-session-dialog-dbus',
                                              'org.gnome.SessionManager.EndSessionDialog.xml',
					      interface_prefix: 'org.gnome.SessionManager',
					      namespace: dbus_prefix)

generated_dbus_sources += gnome.gdbus_codegen('phosh-gtk-mountoperation-dbus',
                                              'org.Gtk.MountOperationHandler.xml',
					      interface_prefix: 'org.Gtk',
					      namespace: dbus_prefix)

dbus_doc_dep = declare_dependency(sources: dbus_doc_targets)
