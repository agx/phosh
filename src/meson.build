subdir('dbus')
subdir('monitor')
subdir('notifications')
subdir('wwan')
subdir('settings')
subdir('gtk-list-models')

phosh_resources = gnome.compile_resources(
  'phosh-resources',
  'phosh.gresources.xml',

  c_name: 'phosh',
)

phosh_enum_headers = files(
  'app-grid-button.h',
  'drag-surface.h',
  'gnome-shell-manager.h',
  'home.h',
  'lockscreen.h',
  'mode-manager.h',
  'monitor/monitor.h',
  'notifications/notification.h',
  'notifications/notify-manager.h',
  'phosh-wayland.h',
  'rotation-manager.h',
  'shell.h',
) + schema_enum_headers

phosh_enums = gnome.mkenums('phosh-enums',
  h_template: 'phosh-enums.h.in',
  c_template: 'phosh-enums.c.in',
  sources: phosh_enum_headers,
)

phosh_settings_sources = files(
  'settings.c') + [
  phosh_settings_widgets_sources,
]

phosh_marshalers = gnome.genmarshal('phosh-marshalers',
  sources : 'phosh-marshalers.list',
  prefix : '_phosh_marshal',
  valist_marshallers : true)

libphosh_generated_sources = [
  phosh_enums,
  phosh_marshalers,
  phosh_resources,
  wl_proto_sources,
  generated_dbus_sources,
]

phosh_layer_surface_sources = files(
  'layersurface.c',
  'layersurface.h',
)

# Symbols from these availabel in tools and unit tests
libphosh_tool_sources = files(
  'app-auth-prompt.c',
  'app-auth-prompt.h',
  'activity.c',
  'activity.h',
  'ambient.c',
  'ambient.h',
  'animation.c',
  'animation.h',
  'app-grid.c',
  'app-grid.h',
  'app-grid-button.c',
  'app-grid-button.h',
  'app-list-model.c',
  'app-list-model.h',
  'auth-prompt-option.c',
  'auth-prompt-option.h',
  'background.c',
  'background.h',
  'bidi.c',
  'bidi.h',
  'call.c',
  'call.h',
  'calls-manager.c',
  'calls-manager.h',
  'clamp.c',
  'clamp.h',
  'connectivity-info.c',
  'connectivity-info.h',
  'drag-surface.c',
  'drag-surface.h',
  'end-session-dialog.c',
  'end-session-dialog.h',
  'hks-info.c',
  'hks-info.h',
  'hks-manager.c',
  'hks-manager.h',
  'docked-info.c',
  'docked-info.h',
  'docked-manager.c',
  'docked-manager.h',
  'fading-label.c',
  'fading-label.h',
  'favorite-list-model.c',
  'favorite-list-model.h',
  'feedback-manager.c',
  'feedback-manager.h',
  'gnome-shell-manager.c',
  'gnome-shell-manager.h',
  'gtk-mount-manager.c',
  'gtk-mount-manager.h',
  'gtk-mount-prompt.c',
  'gtk-mount-prompt.h',
  'keypad.c',
  'keypad.h',
  'lockshield.c',
  'lockshield.h',
  'log.h',
  'log.c',
  'manager.h',
  'manager.c',
  'media-player.c',
  'media-player.h',
  'mode-manager.c',
  'mode-manager.h',
  'mount-manager.c',
  'mount-manager.h',
  'mount-operation.c',
  'mount-operation.h',
  'overview.c',
  'overview.h',
  'osd-window.c',
  'osd-window.h',
  'plugin-loader.c',
  'plugin-loader.h',
  'status-icon.c',
  'status-icon.h',
  'splash.c',
  'splash.h',
  'splash-manager.c',
  'splash-manager.h',
  'system-modal.c',
  'system-modal.h',
  'system-modal-dialog.c',
  'system-modal-dialog.h',
  'thumbnail.c',
  'thumbnail.h',
  'vpn-info.c',
  'vpn-info.h',
  'vpn-manager.c',
  'vpn-manager.h',
  'quick-setting.c',
  'quick-setting.h',
  'phosh-wayland.c',
  'phosh-wayland.h',
  'swipe-away-bin.c',
  'swipe-away-bin.h',
  'util.c',
  'util.h',
  'widget-box.c',
  'widget-box.h',
  'wl-buffer.c',
  'wl-buffer.h',
) + [
  libphosh_generated_sources,
  phosh_gtk_list_models_sources,
  phosh_layer_surface_sources,
  phosh_monitor_sources,
  phosh_notifications_sources,
]

# Symbols from these are not availabel in tools and unit tests
libphosh_sources = files(
  'arrow.c',
  'arrow.h',
  'app-tracker.h',
  'app-tracker.c',
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
  'location-info.c',
  'location-info.h',
  'location-manager.c',
  'location-manager.h',
  'lockscreen-manager.c',
  'lockscreen-manager.h',
  'lockscreen.c',
  'lockscreen.h',
  'monitor-manager.c',
  'monitor-manager.h',
  'network-auth-manager.c',
  'network-auth-manager.h',
  'network-auth-prompt.c',
  'network-auth-prompt.h',
  'osk-button.c',
  'osk-button.h',
  'osk-manager.c',
  'osk-manager.h',
  'top-panel.c',
  'top-panel.h',
  'polkit-auth-agent.c',
  'polkit-auth-agent.h',
  'polkit-auth-prompt.c',
  'polkit-auth-prompt.h',
  'portal-access-manager.c',
  'portal-access-manager.h',
  'portal-request.c',
  'portal-request.h',
  'proximity.h',
  'proximity.c',
  'rotation-manager.h',
  'rotation-manager.c',
  'run-command-dialog.c',
  'run-command-dialog.h',
  'run-command-manager.c',
  'run-command-manager.h',
  'sensor-proxy-manager.c',
  'sensor-proxy-manager.h',
  'rotateinfo.c',
  'rotateinfo.h',
  'screen-saver-manager.c',
  'screen-saver-manager.h',
  'screenshot-manager.c',
  'screenshot-manager.h',
  'session-presence.c',
  'session-presence.h',
  'session-manager.c',
  'session-manager.h',
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
  'wwan-info.c',
  'wwan-info.h',
) + [
  phosh_settings_sources,
  phosh_wwan_sources,
]

phosh_deps = [
  fribidi_dep,
  gcr_dep,
  gio_dep,
  gio_unix_dep,
  glib_dep,
  gmodule_dep,
  gnome_desktop_dep,
  gobject_dep,
  gsettings_desktop_schemas_dep,
  gtk_dep,
  gtk_wayland_dep,
  libcall_ui_dep,
  gudev_dep,
  libfeedback_dep,
  libgvc_dep,
  libhandy_dep,
  libnm_dep,
  libpolkit_agent_dep,
  libsystemd_dep,
  network_agent_dep,
  upower_glib_dep,
  wayland_client_dep,
  cc.find_library('pam', required: true),
  cc.find_library('m', required: false),
  cc.find_library('rt', required: false),
]

polkit_version = libpolkit_agent_dep.version().split('.')
if polkit_version.length() == 1
  polkit_major = polkit_version[0]
  polkit_minor = '0'
else
  polkit_major = polkit_version[0]
  polkit_minor = polkit_version[1]
endif
add_project_arguments([
  '-DPOLKIT_AGENT_MAJOR_VERSION=' + polkit_major,
  '-DPOLKIT_AGENT_MINOR_VERSION=' + polkit_minor,
], language: 'c')

phosh_inc = include_directories('.')
# A static library used by tests and tools
phosh_tool_lib = static_library('phosh-tool',
  libphosh_tool_sources,
  include_directories: [root_inc, phosh_inc],
  dependencies: phosh_deps)
phosh_tool_dep = declare_dependency(sources: libphosh_generated_sources,
  include_directories: [root_inc, phosh_inc],
  link_with: phosh_tool_lib,
  dependencies: phosh_deps)

phosh_lib = both_libraries('phosh',
  libphosh_sources,
  include_directories: [root_inc, phosh_inc],
  dependencies: [phosh_tool_dep, phosh_deps])

if get_option('gtk_doc')

  # Shared lib used by docs
  phosh_doc_dep = declare_dependency(sources: libphosh_generated_sources,
    include_directories: [root_inc, phosh_inc, proto_inc, dbus_inc],
    link_with: phosh_lib,
    dependencies: phosh_deps)

    phosh_gir_extra_args = [
      '--quiet',
    ]

  phosh_gir = gnome.generate_gir(phosh_lib,
                sources : [libphosh_sources, libphosh_tool_sources],
              nsversion : '0',
              namespace : 'Phosh',
        export_packages : 'phosh-0',
          symbol_prefix : 'phosh',
      identifier_prefix : 'Phosh',
              link_with : phosh_lib,
               includes : ['Gio-2.0', 'Gtk-3.0', 'Handy-1'],
             extra_args : phosh_gir_extra_args,
           dependencies : phosh_doc_dep,
   )
endif

# Static library used by shell and integration tests
phosh_dep = declare_dependency(
  include_directories: [root_inc, phosh_inc],
  link_with: phosh_lib.get_static_lib(),
  dependencies: [phosh_deps, phosh_tool_dep])

phosh = executable('phosh', 'main.c',
  dependencies: phosh_dep,
  install: true,
  install_dir: libexecdir)
