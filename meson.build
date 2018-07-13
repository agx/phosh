project('phosh', 'c',
          version: '0',
          license: 'GPLv3+',
    meson_version: '>= 0.40.1',
  default_options: [ 'warning_level=1', 'buildtype=debugoptimized', 'c_std=gnu11' ],
)

config_h = configuration_data()
config_h.set_quoted('GETTEXT_PACKAGE', 'phosh')
config_h.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))

configure_file(
  input: 'config.h.in',
  output: 'config.h',
  configuration: config_h,
)

add_project_arguments([
  '-DHAVE_CONFIG_H',
  '-I' + meson.build_root(),
], language: 'c')

root_inc = include_directories('.')
src_inc = include_directories('src')

cc = meson.get_compiler('c')

global_c_args = []
test_c_args = [
  '-Wcast-align',
  '-Wdeclaration-after-statement',
  ['-Werror=format-security', '-Werror=format=2'],
  '-Wformat-nonliteral',
  '-Wformat-security',
  '-Wmissing-include-dirs',
  '-Wnested-externs',
  '-Wno-missing-field-initializers',
  '-Wno-sign-compare',
  '-Wno-strict-aliasing',
  '-Wno-uninitialized',
  '-Wno-unused-parameter',
  '-Wpointer-arith',
  '-Wredundant-decls',
  '-Wshadow',
  '-Wswitch-default',
  '-Wswitch-enum',
  '-Wundef',
]
if get_option('buildtype') != 'plain'
  test_c_args += '-fstack-protector-strong'
endif

foreach arg: test_c_args
  if cc.has_multi_arguments(arg)
    global_c_args += arg
  endif
endforeach
add_project_arguments(
  global_c_args,
  language: 'c'
)

run_data = configuration_data()
run_data.set('ABS_BUILDDIR', meson.current_build_dir())
run_data.set('ABS_SRCDIR', meson.current_source_dir())
configure_file(
  input: 'run.in',
  output: 'run',
  configuration: run_data)

gnome = import('gnome')

subdir('data')
subdir('po')
subdir('src')
subdir('tests')

git = find_program('git', required: false)
if git.found()
	all_files = run_command(
		git,
		['--git-dir=@0@/.git'.format(meson.current_source_dir()),
		 'ls-files',
		 ':/*.[ch]'])
	all_files = files(all_files.stdout().split())

	etags = find_program('etags', required: false)
	if etags.found() and all_files.length() > 0
		custom_target('etags',
			build_by_default: true,
			input: all_files,
			output: 'TAGS',
			command: [etags.path(), '-o', 'TAGS'] + all_files)
	endif

	ctags = find_program('ctags', required: false)
	if ctags.found() and all_files.length() > 0
		custom_target('ctags',
			build_by_default: true,
			input: all_files,
			output: 'tags',
			command: [ctags.path(), '-o', 'tags'] + all_files)
	endif
endif

summary = [
  '',
  '-----',
  'Phosh',
  '',
  'Tests: @0@'.format(get_option('tests')),
 '-----',

]

message('\n'.join(summary))
