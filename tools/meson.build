configure_file(
  input: 'run_tool.in',
  output: 'run_tool',
  configuration: run_data)

executable('app-scroll', ['app-scroll.c', '../tests/stubs/phosh.c', '../tests/stubs/toplevel.c', '../tests/stubs/toplevel-manager.c'],
           dependencies: phosh_dep)
