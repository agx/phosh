subdir('dbus')
subdir('monitor')
subdir('notifications')
subdir('wwan')
subdir('settings')
subdir('gtk-list-models')

phosh_enums_header_subdir = 'phosh'

install_headers('phosh-settings-enums.h', subdir : phosh_enums_header_subdir)

pkgconfig.generate(
  requires : gsettings_desktop_schemas_dep,
  subdirs : phosh_enums_header_subdir,
  version : meson.project_version(),
  install_dir : libdir / 'pkgconfig',
  filebase : 'phosh-settings',
  name : 'phosh-settings-schemas',
  description : 'Shared gsettings schema and enum headers for Phosh',
)

phosh_resources = gnome.compile_resources(
  'phosh-resources',
  'phosh.gresources.xml',

  c_name: 'phosh',
)

phosh_enum_headers = files(
  'animation.h',
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
  'top-panel.h',
) + schema_enum_headers

phosh_enums = gnome.mkenums_simple(
  'phosh-enums',
  sources : phosh_enum_headers,
  install_header: bindings_lib,
  install_dir: lib_inc_dir,
)

phosh_settings_sources = files(
  'settings.c') + [
  phosh_settings_widgets_sources,
]

phosh_marshalers = gnome.genmarshal('phosh-marshalers',
  sources : 'phosh-marshalers.list',
  prefix : '_phosh_marshal',
  valist_marshallers : true)


# sources generated with glib tooling
# hence parsable by gi-docgen
libphosh_glib_generated_sources = [
  phosh_enums[0],
  phosh_marshalers[0],
  phosh_resources[0],
  generated_dbus_sources,
]

libphosh_glib_generated_headers = [
  phosh_enums[1],
  phosh_marshalers[1],
  phosh_resources[1],
  generated_dbus_headers,
]

libphosh_generated_sources = [
  libphosh_glib_generated_sources,
  wl_proto_sources,
]

libphosh_generated_headers = [
  libphosh_glib_generated_headers,
  wl_proto_headers,
]

libphosh_tool_headers = files(
  'app-auth-prompt.h',
  'activity.h',
  'ambient.h',
  'animation.h',
  'app-grid.h',
  'app-grid-base-button.h',
  'app-grid-button.h',
  'app-grid-folder-button.h',
  'app-list-model.h',
  'auth-prompt-option.h',
  'background-cache.h',
  'background-image.h',
  'background.h',
  'bidi.h',
  'call.h',
  'calls-manager.h',
  'call-notification.h',
  'clamp.h',
  'connectivity-info.h',
  'drag-surface.h',
  'end-session-dialog.h',
  'emergency-contact-row.h',
  'emergency-contact.h',
  'emergency-menu.h',
  'folder-info.h',
  'hks-info.h',
  'hks-manager.h',
  'docked-info.h',
  'docked-manager.h',
  'fading-label.h',
  'favorite-list-model.h',
  'feedback-manager.h',
  'gnome-shell-manager.h',
  'gtk-mount-manager.h',
  'gtk-mount-prompt.h',
  'keypad.h',
  'launcher-entry-manager.h',
  'layersurface.h',
  'lockshield.h',
  'log.h',
  'manager.h',
  'media-player.h',
  'mode-manager.h',
  'mount-manager.h',
  'mount-operation.h',
  'overview.h',
  'password-entry.h',
  'osd-window.h',
  'plugin-loader.h',
  'power-menu.h',
  'power-menu-manager.h',
  'revealer.h',
  'status-icon.h',
  'splash.h',
  'splash-manager.h',
  'suspend-manager.h',
  'system-modal.h',
  'system-modal-dialog.h',
  'vpn-info.h',
  'vpn-manager.h',
  'quick-setting.h',
  'phosh-wayland.h',
  'swipe-away-bin.h',
  'util.h',
  'wall-clock.h',
  'widget-box.h',
  'wl-buffer.h',
)

# Symbols from these available in tools and unit tests
libphosh_tool_sources = files(
  'app-auth-prompt.c',
  'activity.c',
  'ambient.c',
  'animation.c',
  'app-grid.c',
  'app-grid-base-button.c',
  'app-grid-button.c',
  'app-grid-folder-button.c',
  'app-list-model.c',
  'app-list-model.h',
  'auth-prompt-option.c',
  'background-cache.c',
  'background-image.c',
  'background.c',
  'bidi.c',
  'call.c',
  'calls-manager.c',
  'call-notification.c',
  'clamp.c',
  'connectivity-info.c',
  'drag-surface.c',
  'end-session-dialog.c',
  'emergency-contact-row.c',
  'emergency-contact.c',
  'emergency-menu.c',
  'folder-info.c',
  'hks-info.c',
  'hks-manager.c',
  'docked-info.c',
  'docked-manager.c',
  'fading-label.c',
  'favorite-list-model.c',
  'feedback-manager.c',
  'gnome-shell-manager.c',
  'gtk-mount-manager.c',
  'gtk-mount-prompt.c',
  'keypad.c',
  'launcher-entry-manager.c',
  'layersurface.c',
  'lockshield.c',
  'log.c',
  'manager.c',
  'media-player.c',
  'mode-manager.c',
  'mount-manager.c',
  'mount-operation.c',
  'overview.c',
  'password-entry.c',
  'osd-window.c',
  'plugin-loader.c',
  'power-menu.c',
  'power-menu-manager.c',
  'revealer.c',
  'status-icon.c',
  'splash.c',
  'splash-manager.c',
  'suspend-manager.c',
  'system-modal.c',
  'system-modal-dialog.c',
  'vpn-info.c',
  'vpn-manager.c',
  'quick-setting.c',
  'phosh-wayland.c',
  'swipe-away-bin.c',
  'util.c',
  'wall-clock.c',
  'widget-box.c',
  'wl-buffer.c',
) + [
  libphosh_tool_headers,
  phosh_monitor_sources,
  phosh_notifications_sources,
]

libphosh_headers = files(
  'arrow.h',
  'app-tracker.h',
  'auth.h',
  'background-manager.h',
  'batteryinfo.h',
  'bt-device-row.h',
  'bt-info.h',
  'bt-manager.h',
  'bt-status-page.h',
  'emergency-calls-manager.h',
  'fader.h',
  'feedbackinfo.h',
  'home.h',
  'keyboard-events.h',
  'idle-manager.h',
  'layout-manager.h',
  'location-info.h',
  'location-manager.h',
  'lockscreen-manager.h',
  'lockscreen.h',
  'monitor-manager.h',
  'network-auth-manager.h',
  'osk-button.h',
  'osk-manager.h',
  'top-panel.h',
  'plugin-shell.h',
  'polkit-auth-agent.h',
  'polkit-auth-prompt.h',
  'portal-access-manager.h',
  'portal-request.h',
  'proximity.h',
  'rotation-manager.h',
  'run-command-dialog.h',
  'run-command-manager.h',
  'sensor-proxy-manager.h',
  'rotateinfo.h',
  'screen-saver-manager.h',
  'screenshot-manager.h',
  'session-presence.h',
  'session-manager.h',
  'shell.h',
  'status-page.h',
  'status-page-placeholder.h',
  'system-prompt.h',
  'system-prompter.h',
  'thumbnail.h',
  'toplevel-manager.h',
  'toplevel-thumbnail.h',
  'torch-info.h',
  'torch-manager.h',
  'toplevel.h',
  'wifi-status-page.h',
  'wifi-network-row.h',
  'wifi-network.h',
  'wifi-info.h',
  'wifi-manager.h',
  'wwan-info.h',
)
# Not visible in docs and gir
libphosh_private_headers = files(
  'contrib/shell-network-agent.h',
  'network-auth-prompt.h',
)

# Symbols from these are not available in tools and unit tests
# Prefer adding to libphosh_tool_sources
libphosh_sources = files(
  'arrow.c',
  'app-tracker.c',
  'auth.c',
  'background-manager.c',
  'batteryinfo.c',
  'bt-device-row.c',
  'bt-info.c',
  'bt-manager.c',
  'bt-status-page.c',
  'contrib/shell-network-agent.c',
  'emergency-calls-manager.c',
  'fader.c',
  'feedbackinfo.c',
  'home.c',
  'keyboard-events.c',
  'idle-manager.c',
  'layout-manager.c',
  'location-info.c',
  'location-manager.c',
  'lockscreen-manager.c',
  'lockscreen.c',
  'monitor-manager.c',
  'network-auth-manager.c',
  'network-auth-prompt.c',
  'osk-button.c',
  'osk-manager.c',
  'top-panel.c',
  'polkit-auth-agent.c',
  'polkit-auth-prompt.c',
  'portal-access-manager.c',
  'portal-request.c',
  'proximity.c',
  'rotation-manager.c',
  'run-command-dialog.c',
  'run-command-manager.c',
  'sensor-proxy-manager.c',
  'rotateinfo.c',
  'screen-saver-manager.c',
  'screenshot-manager.c',
  'session-presence.c',
  'session-manager.c',
  'shell.c',
  'status-page.c',
  'status-page-placeholder.c',
  'system-prompt.c',
  'system-prompter.c',
  'thumbnail.c',
  'toplevel-manager.c',
  'toplevel-thumbnail.c',
  'torch-info.c',
  'torch-manager.c',
  'toplevel.c',
  'wifi-status-page.c',
  'wifi-network-row.c',
  'wifi-network.c',
  'wifi-info.c',
  'wifi-manager.c',
  'wwan-info.c',
) + [
  libphosh_headers,
  phosh_settings_sources,
  phosh_wwan_sources,
]

# See https://gitlab.gnome.org/GNOME/libsoup/-/merge_requests/394
if libsoup_dep.version().version_compare('< 3.5.1')
  libsoup_dep = declare_dependency (
    include_directories: libsoup_dep.get_variable(pkgconfig: 'includedir') / 'libsoup-3.0',
    dependencies: cc.find_library('soup-3.0', required: true),
  )
endif


# Headers are bundled as they're not shipped by gnome-bluetooth
# https://gitlab.gnome.org/GNOME/gnome-bluetooth/-/merge_requests/200
gnome_bluetooth_headers_dep = declare_dependency(
  include_directories: 'contrib/gnome-bluetooth',
)
# We build our own dep to avoid `-Wmissing-include-dirs` as the
# directory in the Cflags of gnome-bluetooth-3.0.pc do not
# necessarily exist
gnome_bluetooth_custom_dep = gnome_bluetooth_dep.partial_dependency(
  link_args: true,
  links: true,
)

phosh_deps = [
  libsoup_dep,
  fribidi_dep,
  gcr_dep,
  gio_dep,
  gio_unix_dep,
  glib_dep,
  gmodule_dep,
  gmobile_dep,
  gnome_bluetooth_custom_dep,
  gnome_bluetooth_headers_dep,
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

phosh_inc = include_directories('.')
phosh_lib_inc = [root_inc,
                 phosh_inc,
                 phosh_monitor_inc,
                 phosh_notifications_inc,
                 phosh_wwan_inc,
                 proto_inc,
                 dbus_inc,
                 phosh_gtk_list_models_inc,
  ]

# A static library used by tests and tools
phosh_tool_lib = static_library('phosh-tool',
  [
    libphosh_tool_sources,
    libphosh_generated_headers,
    libphosh_generated_sources,
    phosh_gtk_list_models_sources,
  ],
  include_directories: phosh_lib_inc,
  dependencies: phosh_deps,
)
phosh_tool_dep = declare_dependency(
  sources: [
    libphosh_generated_headers,
    # Ensure the resources are available to indirect users too
    phosh_resources[0],
  ],
  include_directories: phosh_lib_inc,
  link_with: phosh_tool_lib,
  dependencies: phosh_deps)

# A library containing everything
phosh_lib = both_libraries('phosh',
  libphosh_private_headers,
  libphosh_sources,
  include_directories: phosh_lib_inc,
  dependencies: [phosh_tool_dep, phosh_deps],
  soversion: soversion,
  install: bindings_lib,
)

phosh_static_lib_dep = declare_dependency(
  include_directories: phosh_lib_inc,
  link_with: phosh_lib.get_static_lib(),
  dependencies: [phosh_deps, phosh_tool_dep],
)

# Dependencies that can be used by plugins
phosh_plugins_dep = declare_dependency(
  include_directories: phosh_lib_inc,
  sources: [generated_dbus_headers, wl_proto_headers],
)

if enable_introspection
  phosh_gir_sources = [ libphosh_sources,
                        libphosh_tool_sources,
                        libphosh_glib_generated_sources,
                        libphosh_glib_generated_headers,
                      ]

  phosh_gir_extra_args = [
    '--quiet',
  ]

  phosh_gir = gnome.generate_gir(phosh_lib,
                 header : 'libphosh.h',
                sources : phosh_gir_sources,
              nsversion : '0',
              namespace : 'Phosh',
        export_packages : 'libphosh-0',
          symbol_prefix : 'phosh',
      identifier_prefix : 'Phosh',
              link_with : phosh_lib,
               includes : ['Gcr-3', 'Gio-2.0', 'Gtk-3.0', 'GnomeDesktop-3.0',
                           'Handy-1', 'NM-1.0', 'GnomeBluetooth-3.0'],
             extra_args : phosh_gir_extra_args,
           dependencies : phosh_static_lib_dep,
         fatal_warnings : true,
   )
endif

if bindings_lib
  pkgconfig.generate(
    phosh_lib,
    name: 'libphosh',
    subdirs: 'libphosh-0',
    filebase: 'libphosh-0',
    version : meson.project_version(),
    description: 'Private library for building external components',
    requires: 'phosh-settings',
    url: 'https://world.pages.gitlab.gnome.org/Phosh/phosh/',
  )

  install_headers(
    [
      'libphosh.h',
      libphosh_tool_headers,
      libphosh_headers,
    ],
    subdir : lib_inc_subdir,
  )

  install_headers(
    phosh_monitor_headers,
    subdir : lib_inc_subdir / 'monitor',
  )

  install_headers(
    phosh_notifications_headers,
    subdir : lib_inc_subdir / 'notifications',
  )

  install_headers(
    phosh_settings_widgets_headers,
    subdir : lib_inc_subdir / 'settings',
  )

  install_headers(
    phosh_wwan_headers,
    subdir : lib_inc_subdir / 'wwan',
  )
endif

phosh_export_sym_link_arg = '-Wl,--dynamic-list=@0@/phosh-exported-symbols.txt'.format(meson.current_source_dir())
phosh = executable(
  'phosh', 'main.c',
  dependencies: phosh_static_lib_dep,
  link_depends: ['phosh-exported-symbols.txt'],
  link_args: phosh_export_sym_link_arg,
  install: true,
  install_dir: libexecdir
)
