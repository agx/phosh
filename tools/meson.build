configure_file(
  input: 'run_tool.in',
  output: 'run_tool',
  configuration: run_data)

stubs = [
  '../tests/stubs/phosh.c',
  '../tests/stubs/toplevel.c',
  '../tests/stubs/toplevel-manager.c',
]

executable('app-scroll', ['app-scroll.c'] + stubs,
           dependencies: phosh_dep)

executable('app-grid-standalone', ['app-grid-standalone.c'] + stubs,
           dependencies: phosh_dep)

executable('image-notify', ['image-notify.c'] + stubs,
           dependencies: phosh_dep)
