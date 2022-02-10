configure_file(
  input: 'run_tool.in',
  output: 'run_tool',
  configuration: run_data)

if get_option('tools') or get_option('tests')
  # app-buttons is used in the screnshot tests
  executable('app-buttons', ['app-buttons.c'] + test_stub_deps,
             dependencies: phosh_tool_dep)
endif

if get_option('tools')

  executable('app-scroll', ['app-scroll.c'] + test_stub_deps,
             dependencies: phosh_tool_dep)

  executable('app-grid-standalone', ['app-grid-standalone.c'] + test_stub_deps,
             dependencies: phosh_tool_dep)

  executable('image-notify', ['image-notify.c'] + test_stub_deps,
             dependencies: phosh_tool_dep)

  executable('notify-blocks', ['notify-blocks.c'],
             dependencies: phosh_tool_dep)

  executable('notify-server-standalone', ['notify-server-standalone.c'] + test_stub_deps,
             dependencies: phosh_tool_dep)

  executable('dump-app-list', ['dump-app-list.c'],
             dependencies: phosh_tool_dep)

endif
