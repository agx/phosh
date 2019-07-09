if get_option('tests')

test_env = [
  'G_TEST_SRCDIR=@0@'.format(meson.current_source_dir()),
  'G_TEST_BUILDDIR=@0@'.format(meson.current_build_dir()),
  'G_DEBUG=gc-friendly,fatal-warnings',
  'GSETTINGS_BACKEND=memory',
  'GSETTINGS_SCHEMA_DIR=@0@/data'.format(meson.build_root()),
  'PYTHONDONTWRITEBYTECODE=yes',
  'MALLOC_CHECK_=2',
]

test_cflags = [
  '-I@0@/../src'.format(meson.current_source_dir()),
  '-I@0@/../src'.format(meson.current_build_dir()),
  '-I@0@/../src/dbus'.format(meson.current_build_dir()),
  '-DTEST_DATA_DIR="@0@/data"'.format(meson.current_source_dir()),
]

test_link_args = [
  '-fPIC',
]

# Deps for all tests
test_deps = [
  'phoshstub.c',
]

tests = [
  'app',
  'favorites',
]

# Unit tests
foreach test : tests
  t = executable('test-@0@'.format(test),
                 test_deps + ['test-@0@.c'.format(test)],
                 c_args: test_cflags,
                 pie: true,
                 link_args: test_link_args,
                 dependencies: phosh_dep)
  test(test[0], t, env: test_env)
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
