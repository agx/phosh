subdir('wwan')
subdir('monitor')

phosh_resources = gnome.compile_resources(
   'phosh-resources',
   'phosh.gresources.xml',

   c_name: 'phosh',
)

phosh_sources = [
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
  'layersurface.c',
  'layersurface.h',
  'lockscreen.c',
  'lockscreen.h',
  'lockscreen-manager.c',
  'lockscreen-manager.h',
  'lockshield.c',
  'lockshield.h',
  'monitor-manager.c',
  'monitor-manager.h',
  'system-prompt.c',
  'system-prompt.h',
  'system-prompter.c',
  'system-prompter.h',
  'panel.c',
  'panel.h',
  'phosh.c',
  'phosh.h',
  'phosh-wayland.c',
  'phosh-wayland.h',
  'settings.c',
  'settings.h',
  'settings/brightness.c',
  'wwaninfo.c',
  'wwaninfo.h',
  phosh_resources,
  phosh_wwan_sources,
  phosh_monitor_sources,
  wl_proto_sources,
]

phosh_deps = [
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
  cc.find_library('pam', required: true),
  cc.find_library('m', required: false),
  cc.find_library('rt', required: false),
]

phosh = executable('phosh', phosh_sources,
  dependencies: phosh_deps,
  install: true,
  install_dir: libexecdir,
)

