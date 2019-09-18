if get_option('tests')

subdir('system')
subdir('user')

test_env = environment()
test_env.set('G_TEST_SRCDIR', meson.current_source_dir())
test_env.set('G_TEST_BUILDDIR', meson.current_build_dir())
test_env.set('G_DEBUG', 'gc-friendly,fatal-warnings')
test_env.set('GSETTINGS_BACKEND', 'memory')
test_env.set('GSETTINGS_SCHEMA_DIR', '@0@/data'.format(meson.build_root()))
test_env.set('PYTHONDONTWRITEBYTECODE', 'yes')
test_env.set('MALLOC_CHECK_', '2')
test_env.set('XDG_CONFIG_HOME', '@0@/user/config/'.format(meson.current_source_dir()))
test_env.set('XDG_CONFIG_DIRS', '@0@/system/config/'.format(meson.current_source_dir()))
test_env.set('XDG_DATA_HOME', '@0@/user/share/'.format(meson.current_source_dir()))
test_env.set('XDG_DATA_DIRS', '@0@/system/share/'.format(meson.current_source_dir()))

test_cflags = [
  '-DTEST_DATA_DIR="@0@/data"'.format(meson.current_source_dir()),
]

test_link_args = [
  '-fPIC',
]

# Deps for all tests
test_deps = [
  'stubs/phosh.c',
  'stubs/toplevel.c',
  'stubs/toplevel-manager.c'
]

tests = [
  'activity',
  'app-grid-button',
  'app-list-model',
  'overview',
  'favourite-model',
]

# Unit tests
foreach test : tests
  t = executable('test-@0@'.format(test),
                 test_deps + ['test-@0@.c'.format(test)],
                 c_args: test_cflags,
                 pie: true,
                 link_args: test_link_args,
                 dependencies: phosh_dep)
  test(test, t, env: test_env)
endforeach

endif

# Integration tests
t = executable('test-idle-manager',
		 ['test-idle-manager.c', generated_dbus_sources],
		 c_args: test_cflags,
		 link_args: test_link_args,
		 dependencies: phosh_deps,
		 )
test('test-idle', t, env: test_env)
