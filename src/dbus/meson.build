dbus_inc = include_directories('.')

# DBus client interfaces
generated_dbus_sources = []
generated_dbus_headers = []

dbus_prefix = 'PhoshDBus'
dbus_inc_dir = inc_dir / 'dbus'

#
# Protocols where Phosh is the DBus client:
#
dbus_client_protos = [
  # Sorted by xml filename:
  ['iio-sensor-proxy-dbus', 'net.hadess.SensorProxy.xml', 'net.hadess'],
  ['hostname1-dbus', 'org.freedesktop.hostname1.xml', 'org.freedesktop'],
  ['portal-dbus', 'org.freedesktop.impl.portal.xml', 'org.freedesktop'],
  ['login1-manager-dbus', 'org.freedesktop.login1.Manager.xml','org.freedesktop.login1'],
  ['login1-session-dbus', 'org.freedesktop.login1.Session.xml', 'org.freedesktop.login1'],
  ['gsd-color-dbus', 'org.gnome.SettingsDaemon.Color.xml', 'org.gnome.SettingsDaemon'],
  ['gnome-session-dbus', 'org.gnome.SessionManager.xml', 'org.gnome'],
]

#
# Protocols where Phosh is the DBus server:
#
dbus_server_protos = [
  # Sorted by xml filename:
  ['phosh-gnome-shell-dbus','org.gnome.Shell.xml', 'org'],
  ['phosh-display-dbus', 'org.gnome.Mutter.DisplayConfig.xml', 'org.gnome.Mutter'],
  ['phosh-screen-saver-dbus', 'org.gnome.ScreenSaver.xml', 'org.gnome'],
  ['phosh-screenshot-dbus', 'org.gnome.Shell.Screenshot.xml', 'org.gnome.Shell'],
  ['phosh-end-session-dialog-dbus', 'org.gnome.SessionManager.EndSessionDialog.xml','org.gnome.SessionManager'],
  ['phosh-gtk-mountoperation-dbus', 'org.Gtk.MountOperationHandler.xml', 'org.Gtk'],
]

foreach p : dbus_client_protos + dbus_server_protos
  name = p[0]
  xml = p[1]
  prefix = p[2]
  targets = gnome.gdbus_codegen(name, xml,
                                interface_prefix: prefix,
                                install_header: bindings_lib,
                                install_dir: dbus_inc_dir,
                                namespace: dbus_prefix)
  generated_dbus_sources += targets[0]
  generated_dbus_headers += targets[1]
endforeach

#
# Protocols where Phosh is the DBus client
# TODO: Use `dbus_prefix` as namespace
#
# Geoclue
generated_dbus_sources += gnome.gdbus_codegen('geoclue-manager-dbus',
                                              'org.freedesktop.GeoClue2.Manager.xml',
                                              interface_prefix: 'org.freedesktop.GeoClue2',
                                              install_header: bindings_lib,
                                              install_dir: dbus_inc_dir,
                                              namespace: 'PhoshGeoClueDBus')
generated_dbus_sources +=  gnome.gdbus_codegen('phosh-wwan-mm-dbus',
                                               'org.freedesktop.ModemManager1.xml',
                                               namespace: 'Phosh_MM_DBus',
                                               interface_prefix: 'org.freedesktop.ModemManager1',
                                               install_header: bindings_lib,
                                               install_dir: dbus_inc_dir,
                                               object_manager: true)
generated_dbus_sources += gnome.gdbus_codegen('calls-dbus',
                                              'org.gnome.Calls.Call.xml',
                                              object_manager: true,
                                              interface_prefix: 'org.gnome',
                                              install_header: bindings_lib,
                                              install_dir: dbus_inc_dir,
                                              namespace: 'PhoshCallsDBus')
generated_dbus_sources += gnome.gdbus_codegen('calls-emergency-dbus',
                                              'org.gnome.Calls.EmergencyCalls.xml',
                                              interface_prefix: 'org.gnome.Calls',
                                              install_header: bindings_lib,
                                              install_dir: dbus_inc_dir,
                                              namespace: 'Phosh')
# org.gnome.SessionManager.ClientPrivate
generated_dbus_sources += gnome.gdbus_codegen('gnome-session-client-private-dbus',
                                              'org.gnome.SessionManager.ClientPrivate.xml',
                                              interface_prefix: 'org.gnome.SessionManager',
                                              install_header: bindings_lib,
                                              install_dir: dbus_inc_dir,
                                              namespace: 'PhoshSessionClientPrivateDBus')
# org.gnome.SessionManager.Presence
generated_dbus_sources += gnome.gdbus_codegen('gnome-session-presence-dbus',
                                              'org.gnome.SessionManager.Presence.xml',
                                              interface_prefix: 'org.gnome.SessionManager',
                                              install_header: bindings_lib,
                                              install_dir: dbus_inc_dir,
                                              namespace: 'PhoshSessionPresenceDBus')
# org.gnome.SettingsDaemon.Rfkill
generated_dbus_sources += gnome.gdbus_codegen('gsd-rfkill-dbus',
                                              'org.gnome.SettingsDaemon.Rfkill.xml',
                                              interface_prefix: 'org.gnome.SettingsDaemon',
                                              install_header: bindings_lib,
                                              install_dir: dbus_inc_dir,
                                              namespace: 'PhoshRfkillDBus')
# MPris2
generated_dbus_sources += gnome.gdbus_codegen('mpris-dbus',
                                              'org.mpris.MediaPlayer2.xml',
                                              interface_prefix: 'org.mpris',
                                              install_header: bindings_lib,
                                              install_dir: dbus_inc_dir,
                                              namespace: 'PhoshMprisDBus')

generated_dbus_sources += gnome.gdbus_codegen('phosh-wwan-ofono-dbus',
                                              'org.ofono.xml',
                                              namespace: 'PhoshOfonoDBus',
                                              install_header: bindings_lib,
                                              install_dir: dbus_inc_dir,
                                              interface_prefix: 'org.ofono')

generated_dbus_sources += gnome.gdbus_codegen('phosh-osk0-dbus',
                                              'sm.puri.OSK0.xml',
                                              install_header: bindings_lib,
                                              install_dir: dbus_inc_dir,
                                              namespace: 'PhoshOsk0')
#
# Protocols where Phosh is the DBus server
# TODO: Use `dbus_prefix` as namespace
#
generated_dbus_sources += gnome.gdbus_codegen('geoclue-agent-dbus',
                                              'org.freedesktop.GeoClue2.Agent.xml',
                                              interface_prefix: 'org.freedesktop.Agent',
                                              install_header: bindings_lib,
                                              install_dir: dbus_inc_dir,
                                              namespace: 'PhoshGeoClueDBus')

generated_dbus_sources += gnome.gdbus_codegen('notify-dbus',
                                              'org.freedesktop.Notifications.xml',
                                              interface_prefix: 'org.freedesktop',
                                              install_header: bindings_lib,
                                              install_dir: dbus_inc_dir,
                                              namespace: 'PhoshNotifyDBus')

generated_dbus_sources += gnome.gdbus_codegen('phosh-idle-dbus',
                                              'org.gnome.Mutter.IdleMonitor.xml',
                                              interface_prefix: 'org.gnome.Mutter',
                                              object_manager: true,
                                              install_header: bindings_lib,
                                              install_dir: dbus_inc_dir,
                                              namespace: 'PhoshIdleDBus')
