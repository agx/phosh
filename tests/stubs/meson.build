test_stubs = files(
  'phosh.c',
  'app-tracker.c',
  'lockscreen-manager.c',
  'background-manager.c',
  'toplevel.c',
  'toplevel-manager.c',
  'thumbnail.c',
)

test_stubs_lib = static_library('test-stubs',
  test_stubs,
  include_directories: phosh_lib_inc,
  sources: [libphosh_generated_headers],
  dependencies: phosh_deps,
)

test_stubs_dep = declare_dependency(
  link_with: test_stubs_lib,
)
