subdir('dbus')
subdir('monitor')
subdir('notifications')
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
  'app-grid-button.h',
  'home.h',
  'mode-manager.h',
  'monitor/monitor.h',
  'notifications/notification.h',
  'notifications/notify-manager.h',
  'phosh-wayland.h',
] + schema_enum_headers

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
  generated_dbus_sources,
]

libphosh_tool_sources = [
  'activity.c',
  'activity.h',
  'animation.c',
  'animation.h',
  'app-grid.c',
  'app-grid.h',
  'app-grid-button.c',
  'app-grid-button.h',
  'app-list-model.c',
  'app-list-model.h',
  'background.c',
  'background.h',
  'connectivity-info.c',
  'connectivity-info.h',
  'docked-info.c',
  'docked-info.h',
  'docked-manager.c',
  'docked-manager.h',
  'favorite-list-model.c',
  'favorite-list-model.h',
  'feedback-manager.c',
  'feedback-manager.h',
  'layersurface.c',
  'layersurface.h',
  'lockshield.c',
  'lockshield.h',
  'log.h',
  'log.c',
  'media-player.c',
  'media-player.h',
  'mode-manager.c',
  'mode-manager.h',
  'overview.c',
  'overview.h',
  'status-icon.c',
  'status-icon.h',
  'thumbnail.c',
  'thumbnail.h',
  'quick-setting.c',
  'quick-setting.h',
  'phosh-wayland.c',
  'phosh-wayland.h',
  'util.c',
  'util.h',
  phosh_gtk_list_models_sources,
  phosh_notifications_sources,
  libphosh_generated_sources,
]

libphosh_sources = [
  'arrow.c',
  'arrow.h',
  'auth.c',
  'auth.h',
  'background-manager.c',
  'background-manager.h',
  'batteryinfo.c',
  'batteryinfo.h',
  'bt-info.c',
  'bt-info.h',
  'bt-manager.c',
  'bt-manager.h',
  'contrib/shell-network-agent.c',
  'contrib/shell-network-agent.h',
  'fader.c',
  'fader.h',
  'feedbackinfo.c',
  'feedbackinfo.h',
  'home.c',
  'home.h',
  'keyboard-events.c',
  'keyboard-events.h',
  'idle-manager.c',
  'idle-manager.h',
  'lockscreen-manager.c',
  'lockscreen-manager.h',
  'lockscreen.c',
  'lockscreen.h',
  'monitor-manager.c',
  'monitor-manager.h',
  'network-auth-prompt.c',
  'network-auth-prompt.h',
  'osk-manager.c',
  'osk-manager.h',
  'panel.c',
  'panel.h',
  'polkit-auth-agent.c',
  'polkit-auth-agent.h',
  'polkit-auth-prompt.c',
  'polkit-auth-prompt.h',
  'proximity.h',
  'proximity.c',
  'sensor-proxy-manager.c',
  'sensor-proxy-manager.h',
  'rotateinfo.c',
  'rotateinfo.h',
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
  'toplevel-thumbnail.c',
  'toplevel-thumbnail.h',
  'torch-info.c',
  'torch-info.h',
  'torch-manager.c',
  'torch-manager.h',
  'toplevel.c',
  'toplevel.h',
  'wifiinfo.c',
  'wifiinfo.h',
  'wifimanager.c',
  'wifimanager.h',
  'wwaninfo.c',
  'wwaninfo.h',
  phosh_settings_sources,
  phosh_wwan_sources,
  phosh_monitor_sources,
  phosh_osk_sources,
]

phosh_deps = [
  gcr_dep,
  gio_dep,
  gio_unix_dep,
  glib_dep,
  gnome_desktop_dep,
  gobject_dep,
  gtk_dep,
  gtk_wayland_dep,
  libfeedback_dep,
  libgvc_dep,
  libhandy_dep,
  libnm_dep,
  libpolkit_agent_dep,
  network_agent_dep,
  upower_glib_dep,
  wayland_client_dep,
  cc.find_library('pam', required: true),
  cc.find_library('m', required: false),
  cc.find_library('rt', required: false),
]

add_project_arguments([
  '-DPOLKIT_AGENT_MAJOR_VERSION=' + libpolkit_agent_dep.version().split('.')[0],
  '-DPOLKIT_AGENT_MINOR_VERSION=' + libpolkit_agent_dep.version().split('.')[1]
], language: 'c')

phosh_inc = include_directories('.')
# A static library used by tests and tools
phosh_tool_lib = static_library('phosh-tool', libphosh_tool_sources, dependencies: phosh_deps)
phosh_tool_dep = declare_dependency(sources: libphosh_generated_sources,
  include_directories: [root_inc, phosh_inc],
  link_with: phosh_tool_lib,
  dependencies: phosh_deps)

if get_option('gtk_doc')
# We only build the shared lib for gtk-doc:
phosh_doc_lib = library('phosh-doc', [libphosh_sources, libphosh_tool_sources], dependencies: phosh_deps)
phosh_doc_dep = declare_dependency(sources: libphosh_generated_sources,
  include_directories: [root_inc, phosh_inc],
  link_with: phosh_doc_lib,
  dependencies: phosh_deps)
endif

phosh = executable('phosh', ['main.c', libphosh_sources],
  dependencies: phosh_tool_dep,
  install: true,
  install_dir: libexecdir)

