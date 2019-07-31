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

phosh_enum_headers = [
]

phosh_enums = gnome.mkenums('phosh-enums',
  h_template: 'phosh-enums.h.in',
  c_template: 'phosh-enums.c.in',
  sources: phosh_enum_headers,
)

phosh_settings_sources = [
  'settings.c',
  phosh_settings_widgets_sources,
]

libphosh_sources = [
  phosh_enums,
  'activity.c',
  'activity.h',
  'favorites.c',
  'favorites.h',
  phosh_resources,
  wl_proto_sources,
]

phosh_sources = [
  'auth.c',
  'auth.h',
  'polkit-auth-agent.c',
  'polkit-auth-agent.h',
  'polkit-auth-prompt.c',
  'polkit-auth-prompt.h',
  'background.c',
  'background.h',
  'background-manager.c',
  'background-manager.h',
  'batteryinfo.c',
  'batteryinfo.h',
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
  'osk-manager.c',
  'osk-manager.h',
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
  'toplevel.c',
  'toplevel.h',
  'toplevel-manager.c',
  'toplevel-manager.h',
  'panel.c',
  'panel.h',
  'phosh-wayland.c',
  'phosh-wayland.h',
  'util.c',
  'util.h',
  'wifimanager.c',
  'wifimanager.h',
  'wifiinfo.c',
  'wifiinfo.h',
  'wwaninfo.c',
  'wwaninfo.h',
  generated_dbus_sources,
  phosh_settings_sources,
  phosh_wwan_sources,
  phosh_monitor_sources,
  phosh_osk_sources,
]

phosh_deps = [
  dependency('gcr-3', version: '>= 3.7.5'),
  dependency('gio-2.0', version: '>=2.56'),
  dependency('gio-unix-2.0', version: '>=2.50.0'),
  dependency('glib-2.0', version: '>=2.50.0'),
  dependency('gnome-desktop-3.0', version: '>=3.26'),
  dependency('gobject-2.0', version: '>=2.50.0'),
  dependency('gtk+-3.0', version: '>=3.22'),
  dependency('gtk+-wayland-3.0', version: '>=3.22'),
  dependency('wayland-client', version: '>=1.14'),
  dependency('libhandy-0.0', version: '>=0.0.1', fallback: ['libhandy', 'libhandy_dep']),
  dependency('upower-glib', version: '>=0.99.1'),
  libgvc_dep,
  libnm_dep,
  libpolkit_agent_dep,
  cc.find_library('pam', required: true),
  cc.find_library('m', required: false),
  cc.find_library('rt', required: false),
]

add_project_arguments([
  '-DPOLKIT_AGENT_MAJOR_VERSION=' + libpolkit_agent_dep.version().split('.')[0],
  '-DPOLKIT_AGENT_MINOR_VERSION=' + libpolkit_agent_dep.version().split('.')[1]
], language: 'c')

phosh_lib = static_library('phosh', libphosh_sources, dependencies: phosh_deps)
phosh_inc = include_directories('.')

phosh_dep = declare_dependency(sources: libphosh_sources,
  include_directories: [root_inc, phosh_inc],
  link_with: phosh_lib,
  dependencies: phosh_deps)

phosh = executable('phosh', phosh_sources,
  dependencies: phosh_dep,
  install: true,
  install_dir: libexecdir)
