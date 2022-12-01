if get_option('gtk_doc')

subdir('xml')

private_headers = [
  'config.h',
]

content_files = [
  'special.xml',
]

expand_content_md_files = [
  'patterns.md',
]

glib_prefix = glib_dep.get_variable(pkgconfig: 'prefix')
glib_docpath = join_paths(glib_prefix, 'share', 'gtk-doc', 'html')
docpath = join_paths(get_option('datadir'), 'gtk-doc', 'html')

# gtk-markdown-to-docbook uses pandoc
pandoc = find_program('pandoc', required: true)
expand_md = find_program('gtk-markdown-to-docbook')
expand_md_files = []
foreach t : expand_content_md_files
  expand_md_files += custom_target(t,
                                     input   : [ t ],
                                     output  : [ t + '.xml' ],
                                     command : [ expand_md, '@INPUT@', '@OUTPUT@'])
endforeach

gnome.gtkdoc('phosh',
           main_xml: 'phosh-docs.xml',
      content_files: content_files + expand_md_files,
            src_dir: [
                       join_paths(meson.source_root(), 'src'),
                       join_paths(meson.build_root(), 'src'),
                     ],
       dependencies: [phosh_doc_dep, dbus_doc_dep],
  gobject_typesfile: 'phosh.types',
          scan_args: [
                       '--rebuild-types',
                       '--ignore-headers=' + ' '.join(private_headers),
                     ],
       fixxref_args: [
                       '--html-dir=@0@'.format(docpath),
                       '--extra-dir=@0@'.format(join_paths(glib_docpath, 'glib')),
                       '--extra-dir=@0@'.format(join_paths(glib_docpath, 'gobject')),
                       '--extra-dir=@0@'.format(join_paths(glib_docpath, 'gio')),
                       '--extra-dir=@0@'.format(join_paths(glib_docpath, 'gtk3')),
                     ],
        install_dir: 'phosh',
            install: true)
endif
