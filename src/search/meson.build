phosh_search_sources = []

phoshsearch_sources = [
  'search-client.c',
  'search-client.h',
  'search-result-meta.c',
  'search-result-meta.h',
  'search-source.c',
  'search-source.h',
  generated_dbus_sources,
  generated_dbus_headers,
]

phosh_search_deps = [gio_dep, gio_unix_dep, dependency('gdk-pixbuf-2.0')]

phosh_search_lib = static_library(
  'phoshsearch',
  phoshsearch_sources,
  include_directories: [root_inc, dbus_inc],
  dependencies: phosh_search_deps,
  install_dir: pkglibdir,
  install: true,
)
phosh_search_inc = include_directories('.')

phosh_search_dep = declare_dependency(
  sources: generated_dbus_sources + generated_dbus_headers,
  include_directories: [dbus_inc, phosh_search_inc],
  link_with: phosh_search_lib,
  dependencies: phosh_search_deps,
)
