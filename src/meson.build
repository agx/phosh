subdir('dbus')
subdir('monitor')
subdir('osk')
subdir('wwan')
subdir('settings')
subdir('gtk-list-models')

phosh_resources = gnome.compile_resources(
  'phosh-resources',
  'phosh.gresources.xml',

  c_name: 'phosh',
)

phosh_enum_headers = [
  'home.h',
  'notification.h',
  'notify-manager.h',
  'app-grid-button.h'
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

libphosh_generated_sources = [
  phosh_enums,
  phosh_resources,
  wl_proto_sources,
]

libphosh_sources = [
  'activity.c',
  'activity.h',
  'app-grid.c',
  'app-grid.h',
  'app-grid-button.c',
  'app-grid-button.h',
  'app-list-model.c',
  'app-list-model.h',
  'favorite-list-model.c',
  'favorite-list-model.h',
  'notification.c',
  'notification.h',
  'notification-banner.c',
  'notification-banner.h',
  'overview.c',
  'overview.h',
  'status-icon.c',
  'status-icon.h',
  'quick-setting.c',
  'quick-setting.h',
  'util.c',
  'util.h',
  phosh_gtk_list_models_sources,
  libphosh_generated_sources,
]

phosh_sources = [
  'arrow.c',
  'arrow.h',
  'auth.c',
  'auth.h',
  'background-manager.c',
  'background-manager.h',
  'background.c',
  'background.h',
  'batteryinfo.c',
  'batteryinfo.h',
  'contrib/shell-network-agent.c',
  'contrib/shell-network-agent.h',
  'fader.c',
  'fader.h',
  'home.c',
  'home.h',
  'idle-manager.c',
  'idle-manager.h',
  'layersurface.c',
  'layersurface.h',
  'lockscreen-manager.c',
  'lockscreen-manager.h',
  'lockscreen.c',
  'lockscreen.h',
  'lockshield.c',
  'lockshield.h',
  'main.c',
  'monitor-manager.c',
  'monitor-manager.h',
  'network-auth-prompt.c',
  'network-auth-prompt.h',
  'notify-manager.c',
  'notify-manager.h',
  'osk-manager.c',
  'osk-manager.h',
  'panel.c',
  'panel.h',
  'phosh-wayland.c',
  'phosh-wayland.h',
  'polkit-auth-agent.c',
  'polkit-auth-agent.h',
  'polkit-auth-prompt.c',
  'polkit-auth-prompt.h',
  'proximity.h',
  'proximity.c',
  'sensor-proxy-manager.c',
  'sensor-proxy-manager.h',
  'screen-saver-manager.c',
  'screen-saver-manager.h',
  'session-presence.c',
  'session-presence.h',
  'session.c',
  'session.h',
  'shell.c',
  'shell.h',
  'system-prompt.c',
  'system-prompt.h',
  'system-prompter.c',
  'system-prompter.h',
  'toplevel-manager.c',
  'toplevel-manager.h',
  'toplevel.c',
  'toplevel.h',
  'wifiinfo.c',
  'wifiinfo.h',
  'wifimanager.c',
  'wifimanager.h',
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
  gio_dep,
  gio_unix_dep,
  glib_dep,
  dependency('gnome-desktop-3.0', version: '>=3.26'),
  dependency('gobject-2.0', version: '>=2.50.0'),
  dependency('gtk+-3.0', version: '>=3.22'),
  dependency('gtk+-wayland-3.0', version: '>=3.22'),
  dependency('wayland-client', version: '>=1.14'),
  libhandy_dep,
  dependency('upower-glib', version: '>=0.99.1'),
  libgvc_dep,
  libnm_dep,
  libpolkit_agent_dep,
  network_agent_dep,
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

phosh_dep = declare_dependency(sources: libphosh_generated_sources,
  include_directories: [root_inc, phosh_inc],
  link_with: phosh_lib,
  dependencies: phosh_deps)

phosh = executable('phosh', phosh_sources,
  dependencies: phosh_dep,
  install: true,
  install_dir: libexecdir)
