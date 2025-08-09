if not get_option('searchd')
  subdir_done()
endif

searchd_args = ['-DG_LOG_USE_STRUCTURED', '-DG_LOG_DOMAIN="mobi.phosh.Shell.Search"']

executable(
  'phosh-searchd',
  ['searchd.c', 'search-provider.c'],
  include_directories: [root_inc],
  dependencies: phosh_search_dep,
  c_args: searchd_args,
  install_dir: libexecdir,
  install: true,
)
