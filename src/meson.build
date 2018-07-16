subdir('dbus')
subdir('monitor')
subdir('osk')
subdir('wwan')
subdir('settings')

phosh_resources = gnome.compile_resources(
   'phosh-resources',
   'phosh.gresources.xml',

   c_name: 'phosh',
)

phosh_settings_sources = [
  'settings.c',
  phosh_settings_widgets_sources,
]

phosh_sources = [
  'app.c',
  'app.h',
  'auth.c',
  'auth.h',
  'background.c',
  'background.h',
  'batteryinfo.c',
  'batteryinfo.h',
  'favorites.c',
  'favorites.h',
  'home.c',
  'home.h',
  'idle-manager.c',
  'idle-manager.h',
  'layersurface.c',
  'layersurface.h',
  'lockscreen.c',
  'lockscreen.h',
  'lockscreen-manager.c',
  'lockscreen-manager.h',
  'lockshield.c',
  'lockshield.h',
  'main.c',
  'monitor-manager.c',
  'monitor-manager.h',
  'session.c',
  'session.h',
  'session-presence.c',
  'session-presence.h',
  'shell.c',
  'shell.h',
  'system-prompt.c',
  'system-prompt.h',
  'system-prompter.c',
  'system-prompter.h',
  'panel.c',
  'panel.h',
  'phosh-wayland.c',
  'phosh-wayland.h',
  'util.c',
  'util.h',
  'wwaninfo.c',
  'wwaninfo.h',
  generated_dbus_sources,
  phosh_resources,
  phosh_settings_sources,
  phosh_wwan_sources,
  phosh_monitor_sources,
  phosh_osk_sources,
  wl_proto_sources,
]

phosh_deps = [
  dependency('gio-2.0', version: '>= 2.56'),
  dependency('gcr-3', version: '>= 3.7.5'),
  dependency('gio-2.0', version: '>=2.50.0'),
  dependency('gio-unix-2.0', version: '>=2.50.0'),
  dependency('glib-2.0', version: '>=2.50.0'),
  dependency('gnome-desktop-3.0', version: '>=3.26'),
  dependency('gobject-2.0', version: '>=2.50.0'),
  dependency('gtk+-3.0', version: '>=3.22'),
  dependency('gtk+-wayland-3.0', version: '>=3.22'),
  dependency('wayland-client', version: '>=1.14'),
  dependency('libhandy-0.0', version: '>=0.0.1'),
  dependency('upower-glib', version: '>=0.99.1'),
  libgvc_dep,
  cc.find_library('pam', required: true),
  cc.find_library('m', required: false),
  cc.find_library('rt', required: false),
]

phosh = executable('phosh', phosh_sources,
  include_directories: [root_inc, src_inc],
  dependencies: phosh_deps,
  install: true,
  install_dir: libexecdir,
)

