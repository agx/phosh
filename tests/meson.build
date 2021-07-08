run_phoc_tests = false

if get_option('tests')

subdir('system')
subdir('user')

phoc = find_program('phoc', required: get_option('phoc_tests'), disabler: true)
run_phoc_tests =  phoc.found() and not get_option('phoc_tests').disabled()

test_env_common = environment()
test_env_common.set('G_TEST_SRCDIR', meson.current_source_dir())
test_env_common.set('G_TEST_BUILDDIR', meson.current_build_dir())
test_env_common.set('G_DEBUG', 'gc-friendly,fatal-warnings')
test_env_common.set('GSETTINGS_BACKEND', 'memory')
test_env_common.set('GSETTINGS_SCHEMA_DIR', '@0@/data'.format(meson.build_root()))
test_env_common.set('PYTHONDONTWRITEBYTECODE', 'yes')
test_env_common.set('MALLOC_CHECK_', '2')
test_env_common.set('NO_AT_BRIDGE', '1')

# xdg setup for the app-grid:
test_env = test_env_common
test_env.set('XDG_CONFIG_HOME', '@0@/user/config/'.format(meson.current_source_dir()))
test_env.set('XDG_CONFIG_DIRS', '@0@/system/config/'.format(meson.current_source_dir()))
test_env.set('XDG_DATA_HOME', '@0@/user/share/'.format(meson.current_source_dir()))
# Ideally we would just set it so that we have a known set of .desktop etc
# but then we can't find the system gschemas
test_env.prepend('XDG_DATA_DIRS', '@0@/system/share/'.format(meson.current_source_dir()))

test_cflags = [
  '-DTEST_DATA_DIR="@0@/data"'.format(meson.current_source_dir()),
  '-DTEST_OUTPUT_DIR="@0@"'.format(meson.current_build_dir()),
]

test_link_args = [
  '-fPIC',
]

# Deps for all tests
test_stub_deps = [
  'stubs/phosh.c',
  'stubs/toplevel.c',
  'stubs/toplevel-manager.c',
  'stubs/thumbnail.c',
]

testlib_sources = [
  'testlib.c',
  'testlib-full-shell.c',
]

testlib = static_library('phoshtest', testlib_sources,
  dependencies: [phosh_deps, phosh_tool_dep],
  c_args: test_cflags)
testlib_dep = declare_dependency(
  include_directories: include_directories('.'),
  dependencies: phosh_tool_dep,
  link_with: testlib)

tests = [
  'activity',
  'app-grid-button',
  'app-list-model',
  'connectivity-info',
  'favourite-model',
  'media-player',
  'mount-notification',
  'notification',
  'notification-content',
  'notification-frame',
  'notification-list',
  'notification-source',
  'overview',
  'quick-setting',
  'status-icon',
]

tests_phoc = [
  'background',
  'end-session-dialog',
  'layer-surface',
  'lockshield',
  'notification-banner',
  'system-modal',
  'system-modal-dialog',
  'timestamp-label',
]

tests_manager = [
  'gtk-mount-manager',
  'idle-manager',
  'monitor-manager',
  'notify-manager',
  'screenshot-manager',
]

if get_option('g_tests')
  tests_manager += ['calls-manager']
endif

# Unit tests
foreach test : tests
  t = executable('test-@0@'.format(test),
                 test_stub_deps + ['test-@0@.c'.format(test)],
                 c_args: test_cflags,
                 pie: true,
                 link_args: test_link_args,
                 dependencies: testlib_dep)
  test(test, t, env: test_env)
endforeach

if run_phoc_tests
  test_env_phoc = test_env_common
  # Make sure this is valid when running the compositor
  test_env_phoc.set('XDG_RUNTIME_DIR', meson.current_build_dir())

  # Unit tests
  foreach test : tests_phoc
    t = executable('test-@0@'.format(test),
                   test_stub_deps + ['test-@0@.c'.format(test)],
                   c_args: test_cflags,
                   pie: true,
                   link_args: test_link_args,
                   dependencies: testlib_dep)
    test(test, t, env: test_env_phoc)
  endforeach

  # Integration tests using the full libphosh
  t = executable('test-shell',
	         'test-shell.c',
                 c_args: test_cflags,
                 pie: true,
                 link_args: test_link_args,
                 dependencies: [phosh_tool_dep, phosh_dep, testlib_dep])
  test('shell', t, env: test_env_phoc, is_parallel : false)
  foreach test : tests_manager
    t = executable('test-@0@'.format(test),
		   ['test-@0@.c'.format(test), generated_dbus_sources],
                   c_args: test_cflags,
                   pie: true,
                   link_args: test_link_args,
		   dependencies: [phosh_tool_dep, phosh_dep, testlib_dep])
    test(test, t, env: test_env_phoc, is_parallel : false)
  endforeach

endif

endif # tests
