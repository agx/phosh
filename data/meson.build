gsd_required_components = [
  'org.gnome.SettingsDaemon.A11ySettings',
  'org.gnome.SettingsDaemon.Color',
  'org.gnome.SettingsDaemon.Datetime',
  'org.gnome.SettingsDaemon.Housekeeping',
  'org.gnome.SettingsDaemon.Keyboard',
  'org.gnome.SettingsDaemon.MediaKeys',
  'org.gnome.SettingsDaemon.Power',
  'org.gnome.SettingsDaemon.PrintNotifications',
  'org.gnome.SettingsDaemon.Rfkill',
  'org.gnome.SettingsDaemon.ScreensaverProxy',
  'org.gnome.SettingsDaemon.Sharing',
  'org.gnome.SettingsDaemon.Smartcard',
  'org.gnome.SettingsDaemon.Sound',
  'org.gnome.SettingsDaemon.UsbProtection',
  'org.gnome.SettingsDaemon.Wacom',
  'org.gnome.SettingsDaemon.Wwan',
]

desktop_required_components = gsd_required_components + [
  'sm.puri.Phosh',
  'sm.puri.OSK0',
]

desktopconf = configuration_data()
desktopconf.set('bindir', bindir)
desktopconf.set('libexecdir', libexecdir)
desktopconf.set('required_components', ';'.join(desktop_required_components) + ';')
if get_option('systemd')
  desktopconf.set('hidden_under_systemd', 'X-GNOME-HiddenUnderSystemd=true')
else
  desktopconf.set('hidden_under_systemd', '')
endif

desktop_utils = find_program('desktop-file-validate', required: false)
desktop_files = {
  'sm.puri.Phosh.desktop': true,
}
foreach desktop_file, install : desktop_files
  merged = i18n.merge_file(
    input: configure_file(
      input: desktop_file + '.in.in',
      output: desktop_file + '.in',
      configuration: desktopconf
    ),
    output: desktop_file,
    po_dir: '../po',
    install: true,
    install_dir: desktopdir,
    type: 'desktop'
  )

  if desktop_utils.found()
    test('Validate desktop file',
         desktop_utils, args: [merged],
         suite: 'tools',
        )
  endif
endforeach

sessions = [
  'phosh.session',
]
foreach session: sessions
  desktop = session + '.desktop'
  i18n.merge_file(
    input: configure_file(
      input: desktop + '.in.in',
      output: desktop + '.in',
      configuration: desktopconf
    ),
    output: session,
    po_dir: '../po',
    install: true,
    install_dir: join_paths(sessiondir, 'sessions'),
    type: 'desktop',
  )
endforeach

runconf = configuration_data()
runconf.set('bindir', bindir)
runconf.set('libexecdir', libexecdir)
runconf.set('pkgdatadir', pkgdatadir)
runconf.set('version', meson.project_version())
runconf.set('wlrootsdir', join_paths(libexecdir, 'wlroots'))
runconf.set('compositor', get_option('compositor'))

if get_option('systemd')
  runconf.set('session_manager', '--systemd')
else
  runconf.set('session_manager', '--builtin')
endif

configure_file(
  input: 'phosh-session.in',
  output: 'phosh-session',
  install_dir: bindir,
  configuration: runconf,
  install: true
)

# Generate XML enum definitions for GSettings schema
schema_enum_headers = files(
  meson.project_source_root() / 'src' / 'app-grid.h',
  meson.project_source_root() / 'src' / 'layout-manager.h',
  meson.project_source_root() / 'src' / 'wwan' / 'phosh-wwan-backend.h',
  meson.project_source_root() / 'src' / 'notifications' / 'notify-feedback.h',
)
generate_enums_schema = gnome.mkenums('sm.puri.phosh.enums.xml',
  sources: schema_enum_headers,
  comments: '<!-- @comment@ -->',
  fhead: '<schemalist>',
  vhead: '  <@type@ id="sm.puri.phosh.@EnumName@">',
  vprod: '    <value nick="@valuenick@" value="@valuenum@"/>',
  vtail: '  </@type@>',
  ftail: '</schemalist>',
  install_header: true,
  install_dir: schemasdir
)

#workaround due to https://github.com/mesonbuild/meson/issues/1687
copy_schema = custom_target('copy-gschema-to-builddir',
  input: 'sm.puri.phosh.gschema.xml',
  output: 'sm.puri.phosh.gschema.xml',
  command: ['cp', '@INPUT@', '@OUTPUT@']
)
schemas = ['sm.puri.phosh.gschema.xml']
compile_schemas = custom_target('glib-compile-schemas',
  build_by_default: true,
  output: 'gschemas.compiled',
  install: false,
  command: [find_program('glib-compile-schemas'),
    meson.current_build_dir()
  ],
  depends: [generate_enums_schema, copy_schema]
)

overrides = ['00_sm.puri.Phosh.gschema.override']

install_data(
  schemas + overrides,
  install_dir: schemasdir
)

subdir('systemd')
subdir('icons')

install_data('phoc.ini', install_dir : pkgdatadir)
install_data('wayland-sessions/phosh.desktop',
	     install_dir : 'share/wayland-sessions')

install_data('phosh.portal', install_dir : join_paths(datadir, 'xdg-desktop-portal', 'portals'))
install_data('phosh-portals.conf', install_dir : join_paths(datadir, 'xdg-desktop-portal'))
