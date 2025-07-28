test_stubs = files(
  'app-tracker.c',
  'background-manager.c',
  'evil-icon.c',
  'lockscreen-manager.c',
  'phosh.c',
  'thumbnail.c',
  'toplevel-manager.c',
  'toplevel.c',
)

test_stubs_lib = static_library(
  'test-stubs',
  test_stubs,
  include_directories: phosh_lib_inc,
  sources: [phosh_generated_headers],
  dependencies: phosh_deps,
)

test_stubs_dep = declare_dependency(link_with: test_stubs_lib)
