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
  '-fPIE',
  '-I../src',
  '-I@0@/../src'.format(meson.current_build_dir()),
  '-DTEST_DATA_DIR="@0@/data"'.format(meson.current_source_dir()),
]

test_link_args = [
  '-fPIC',
]

test_stubs = [
  'phoshstub.c',
  wl_proto_sources,
]

cls_names = [
  'favorites',
]

foreach cls_name : cls_names
  t = executable('test-@0@'.format(cls_name),
		 ['test-@0@.c'.format(cls_name),
		  '../src/@0@.c'.format(cls_name),
		  test_stubs],
		 c_args: test_cflags,
		 link_args: test_link_args,
		 dependencies: phosh_deps,
		)
  test(cls_name, t, env: test_env)
endforeach

endif
