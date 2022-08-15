calendar_sources = [
  'calendar-server.c',
  'calendar-debug.h',
  'calendar-sources.c',
  'calendar-sources.h'
]


calendar_server_name = meson.project_name() + '-calendar-server'
calendar_server = executable(calendar_server_name, calendar_sources,
  dependencies: [ecal_dep, eds_dep, gio_dep],
  include_directories: include_directories('..'),
  c_args: [
    '-DPREFIX="@0@"'.format(prefix),
    '-DLIBDIR="@0@"'.format(libdir),
    '-DDATADIR="@0@"'.format(datadir),
    '-DG_LOG_DOMAIN="ShellCalendarServer"',
    '-DCALENDAR_SERVER_NAME="@0@"'.format(calendar_server_name),
  ],
  install_dir: libexecdir,
  install: true
)

service_data = configuration_data()
service_data.set('app_id', app_id)
service_data.set('shell', meson.project_name())
service_data.set('libexecdir', libexecdir)
service_file = 'CalendarServer.service'

configure_file(
  input: service_file + '.in',
  output: app_id + '.' + service_file,
  configuration: service_data,
  install_dir: servicedir
)
