configure_file(
  input: 'run_tool.in',
  output: 'run_tool',
  configuration: run_data)

stubs = [
  '../tests/stubs/phosh.c',
  '../tests/stubs/app-tracker.c',
  '../tests/stubs/lockscreen-manager.c',
  '../tests/stubs/toplevel.c',
  '../tests/stubs/toplevel-manager.c',
]

osk_stub_deps = [
  gio_dep,
]

executable('app-scroll', ['app-scroll.c'] + stubs,
           dependencies: phosh_tool_dep)

executable('app-grid-standalone', ['app-grid-standalone.c'] + stubs,
           dependencies: phosh_tool_dep)

executable('image-notify', ['image-notify.c'] + stubs,
           dependencies: phosh_tool_dep)

executable('phosh-osk-stub', ['phosh-osk-stub.c'],
           include_directories: [root_inc, phosh_inc],
           install: get_option('osk-stub'),
           dependencies: osk_stub_deps)

executable('app-buttons', ['app-buttons.c'] + stubs,
           dependencies: phosh_tool_dep)

executable('notify-blocks', ['notify-blocks.c'],
           dependencies: phosh_tool_dep)

executable('notify-server-standalone', ['notify-server-standalone.c'] + stubs,
           dependencies: phosh_tool_dep)

executable('dump-app-list', ['dump-app-list.c'],
           dependencies: phosh_tool_dep)
