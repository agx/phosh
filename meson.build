project('phosh', 'c',
          version: '0.24.0',
          license: 'GPLv3+',
    meson_version: '>= 0.54.0',
  default_options: [ 'warning_level=1', 'buildtype=debugoptimized', 'c_std=gnu11' ],
)

app_id = 'sm.puri.Phosh'
prefix = get_option('prefix')
bindir = join_paths(prefix, get_option('bindir'))
datadir = join_paths(prefix, get_option('datadir'))
localedir = join_paths(prefix, get_option('localedir'))
libdir = join_paths(prefix, get_option('libdir'))
libexecdir = join_paths(prefix, get_option('libexecdir'))
desktopdir = join_paths(datadir, 'applications')
sessiondir = join_paths(datadir, 'gnome-session')
pkgdatadir = join_paths(datadir, meson.project_name())
pkglibdir = join_paths(libdir, meson.project_name())
systemddir = join_paths(prefix, 'lib/systemd')
systemduserdir = join_paths(systemddir, 'user')
plugins_dir = join_paths(prefix, libdir, 'phosh', 'plugins')
plugin_prefs_dir = join_paths(prefix, libdir, 'phosh', 'plugins', 'prefs')
servicedir = join_paths(datadir, 'dbus-1', 'services')
schemasdir = datadir / 'glib-2.0/schemas'

glib_ver = 'GLIB_VERSION_2_66'

add_project_arguments([
  '-DGLIB_VERSION_MIN_REQUIRED=@0@'.format(glib_ver),
  '-DGLIB_VERSION_MAX_REQUIRED=@0@'.format(glib_ver),
  '-DG_LOG_USE_STRUCTURED',
], language: 'c')

root_inc = include_directories('.')

cc = meson.get_compiler('c')

global_c_args = []
test_c_args = [
  '-Wcast-align',
  '-Wdate-time',
  '-Wdeclaration-after-statement',
  ['-Werror=format-security', '-Werror=format=2'],
  '-Wendif-labels',
  '-Werror=incompatible-pointer-types',
  '-Werror=missing-declarations',
  '-Werror=overflow',
  '-Werror=return-type',
  '-Werror=shift-count-overflow',
  '-Werror=shift-overflow=2',
  '-Werror=implicit-fallthrough=3',
  '-Wfloat-equal',
  '-Wformat-nonliteral',
  '-Wformat-security',
  '-Winit-self',
  '-Wmaybe-uninitialized',
  '-Wmissing-field-initializers',
  '-Wmissing-include-dirs',
  '-Wmissing-noreturn',
  '-Wnested-externs',
  '-Wno-missing-field-initializers',
  '-Wno-sign-compare',
  '-Wno-strict-aliasing',
  '-Wno-unused-parameter',
  '-Wold-style-definition',
  '-Wpointer-arith',
  '-Wredundant-decls',
  '-Wshadow',
  '-Wstrict-prototypes',
  '-Wswitch-default',
  '-Wswitch-enum',
  '-Wtype-limits',
  '-Wundef',
  '-Wunused-function',
]
if get_option('buildtype') != 'plain'
  test_c_args += '-fstack-protector-strong'
endif

foreach arg: test_c_args
  if cc.has_multi_arguments(arg)
    global_c_args += arg
  endif
endforeach
add_project_arguments(
  global_c_args,
  language: 'c'
)

run_data = configuration_data()
run_data.set('ABS_BUILDDIR', meson.current_build_dir())
run_data.set('ABS_SRCDIR', meson.current_source_dir())
configure_file(
  input: 'run.in',
  output: 'run',
  configuration: run_data)

gnome = import('gnome')
i18n  = import('i18n')
pkgconfig = import ('pkgconfig')

glib_ver = '>= 2.66'

ecal_dep = dependency('libecal-2.0', version: '>= 3.33.1')
eds_dep = dependency('libedataserver-1.2', version: '>= 3.33.1')
fribidi_dep = dependency('fribidi')
gcr_dep = dependency('gcr-3', version: '>= 3.7.5')
glib_dep = dependency('glib-2.0', version: glib_ver)
gio_dep = dependency('gio-2.0', version: glib_ver)
gobject_dep = dependency('gobject-2.0', version: glib_ver)
gio_unix_dep =  dependency('gio-unix-2.0', version: glib_ver)
gmodule_dep = dependency('gmodule-2.0', version: glib_ver)
gnome_desktop_dep = dependency('gnome-desktop-3.0', version: '>=3.26')
gsettings_desktop_schemas_dep = dependency('gsettings-desktop-schemas')
gtk_dep = dependency('gtk+-3.0', version: '>=3.22')
gtk_wayland_dep = dependency('gtk+-wayland-3.0', version: '>=3.22')
gudev_dep = dependency('gudev-1.0')
libfeedback_dep = dependency('libfeedback-0.0',
  fallback: ['libfeedback', 'libfeedback_dep'],
  default_options: ['introspection=disabled', 'daemon=false', 'gtk_doc=false']
  )
libgvc = subproject('gvc',
  default_options: [
    'package_name=' + meson.project_name(),
    'package_version=' + meson.project_version(),
    'pkgdatadir=' + pkgdatadir,
    'pkglibdir=' + pkglibdir,
    'static=true',
    'introspection=false',
    'alsa=false'
  ])
libgvc_dep = libgvc.get_variable('libgvc_dep')
libhandy_dep = dependency('libhandy-1',
  version: '>=1.1.90',
  fallback: ['libhandy', 'libhandy_dep'],
  default_options: ['introspection=disabled']
  )
libcall_ui = subproject('libcall-ui',
  default_options: [
    'examples=false',
    'gtk_doc=false',
    'tests=false',
    'install-i18n=@0@'.format(get_option('callui-i18n')),
  ])
libcall_ui_dep = libcall_ui.get_variable('libcall_ui_dep')
gmobile = subproject('gmobile',
  default_options: [
    'examples=false',
    'gtk_doc=false',
    'tests=false',
  ])
gmobile_dep = gmobile.get_variable('gmobile_dep')
libnm_dep = dependency('libnm', version: '>= 1.14')
libpolkit_agent_dep = dependency('polkit-agent-1', version: '>= 0.105')
libsystemd_dep = dependency('libsystemd', version: '>= 241')
network_agent_dep = dependency('libsecret-1')
upower_glib_dep = dependency('upower-glib', version: '>=0.99.1')
wayland_client_dep = dependency('wayland-client', version: '>=1.14')
wayland_protos_dep = dependency('wayland-protocols', version: '>=1.12')

# gnome-desktop > 43 has GNOME_PLATFORM_VERSION
have_gnome_platform_version = dependency('gnome-desktop-3.0', version: '>=43', required: false).found()

code ='''
#include <linux/rfkill.h>

struct rfkill_event_ext e;
'''
have_rfkill_event_ext = cc.compiles(code, name : 'Have rfkill_event_ext')

have_polkit_subject_cleanup = cc.compiles('''
#define POLKIT_AGENT_I_KNOW_API_IS_SUBJECT_TO_CHANGE
#include <polkitagent/polkitagent.h>

int main()
{
   g_autoptr (PolkitSubject) *subj = NULL;
}''',
  name : 'Have PolkitSubject cleanup',
  dependencies: libpolkit_agent_dep)


have_polkit_agent_listener_cleanup = cc.compiles('''
#define POLKIT_AGENT_I_KNOW_API_IS_SUBJECT_TO_CHANGE
#include <polkitagent/polkitagent.h>

int main()
{
   g_autoptr (PolkitAgentListener) *listener = NULL;
}''',
  name : 'Have PolkitAgentListener cleanup',
  dependencies: libpolkit_agent_dep)


config_h = configuration_data()
config_h.set_quoted('GETTEXT_PACKAGE', 'phosh')
config_h.set_quoted('LOCALEDIR', localedir)
config_h.set_quoted('PHOSH_PLUGINS_DIR', plugins_dir)
config_h.set_quoted('PHOSH_VERSION', meson.project_version())
config_h.set('HAVE_RFKILL_EVENT_EXT', have_rfkill_event_ext)
config_h.set('PHOSH_USES_ASAN', get_option('b_sanitize') == 'address')
config_h.set('PHOSH_HAVE_GNOME_PLATFORM_VERSION', have_gnome_platform_version)
config_h.set_quoted('PHOSH_DATA_DIR', datadir)
config_h.set_quoted('PHOSH_APP_ID', app_id)
# Work around lack of replace in meson 0.56
m_ver = meson.version().split('.')
if m_ver[0].to_int() > 0 or m_ver[1].to_int() >= 58
  config_h.set_quoted('PHOSH_DBUS_PATH_PREFIX', '/' + app_id.replace('.', '/'))
else
  config_h.set_quoted('PHOSH_DBUS_PATH_PREFIX', '/sm/puri/Phosh')
endif
config_h.set('PHOSH_HAVE_POLKIT_SUBJECT_CLEANUP', have_polkit_subject_cleanup)
config_h.set('PHOSH_HAVE_POLKIT_AGENT_LISTENER_CLEANUP', have_polkit_agent_listener_cleanup)

configure_file(
  input: 'phosh-config.h.in',
  output: 'phosh-config.h',
  configuration: config_h,
)

meson.add_install_script(
  join_paths('build-aux', 'post_install.py'),
  [datadir, plugins_dir],
)

subdir('data')
subdir('po')
subdir('protocol')
subdir('src')
subdir('plugins')
subdir('tests')
subdir('tools')
subdir('docs')
subdir('calendar-server')

summary({
    'Tests': get_option('tests'),
    'Phoc tests': run_phoc_tests,
    'Documentation': get_option('gtk_doc'),
    'Manual pages': get_option('man'),
    'Tools': get_option('tools'),
    'Lockscreen Plugins': get_option('lockscreen-plugins'),
  },
  bool_yn: true,
  section: 'Build',
)

summary({
    'Compositor:': get_option('compositor'),
    'Systemd for session': get_option('systemd'),
  },
  bool_yn: true,
  section: 'Runtime',
)
