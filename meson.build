project('phosh', 'c',
          version: '0.5.0',
          license: 'GPLv3+',
    meson_version: '>= 0.49.0',
  default_options: [ 'warning_level=1', 'buildtype=debugoptimized', 'c_std=gnu11' ],
)

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

config_h = configuration_data()
config_h.set_quoted('GETTEXT_PACKAGE', 'phosh')
config_h.set_quoted('LOCALEDIR', localedir)
config_h.set_quoted('PHOSH_VERSION', meson.project_version())

configure_file(
  input: 'config.h.in',
  output: 'config.h',
  configuration: config_h,
)

glib_ver = 'GLIB_VERSION_2_58'

add_project_arguments([
  '-DHAVE_CONFIG_H',
  '-DGLIB_VERSION_MIN_REQUIRED=@0@'.format(glib_ver),
  '-DGLIB_VERSION_MAX_ALLOWED=@0@'.format(glib_ver),
  '-DG_LOG_USE_STRUCTURED',
  '-I' + meson.build_root(),
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

gcr_dep = dependency('gcr-3', version: '>= 3.7.5')
gio_dep = dependency('gio-2.0', version: '>=2.58')
gio_unix_dep =  dependency('gio-unix-2.0', version: '>=2.58')
glib_dep = dependency('glib-2.0', version: '>=2.58')
gnome_desktop_dep = dependency('gnome-desktop-3.0', version: '>=3.26')
gobject_dep =  dependency('gobject-2.0', version: '>=2.50.0')
gtk_dep = dependency('gtk+-3.0', version: '>=3.22')
gtk_wayland_dep = dependency('gtk+-wayland-3.0', version: '>=3.22')
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
  version: '>=1.0.0',
  fallback: ['libhandy', 'libhandy_dep'],
  default_options: ['introspection=disabled']
  )
libnm_dep = dependency('libnm', version: '>= 1.14')
libpolkit_agent_dep = dependency('polkit-agent-1', version: '>= 0.105')
network_agent_dep = dependency('libsecret-1')
upower_glib_dep = dependency('upower-glib', version: '>=0.99.1')
wayland_client_dep = dependency('wayland-client', version: '>=1.14')
wayland_protos_dep = dependency('wayland-protocols', version: '>=1.12')

subdir('data')
subdir('po')
subdir('protocol')
subdir('src')
subdir('tools')
subdir('tests')
subdir('docs')

summary = [
  '',
  '-----------',
  'Phosh @0@'.format(meson.project_version()),
  '',
  '        Tests: @0@'.format(get_option('tests')),
  '   Phoc Tests: @0@'.format(run_phoc_tests),
  'Documentation: @0@'.format(get_option('gtk_doc')),
  '-----------',
]

message('\n'.join(summary))
