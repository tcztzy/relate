project('gzstream', 'cpp', version: '1.5', license: 'LGPL')
install_headers('gzstream.h')
pkgc = import('pkgconfig')
pkgc.generate(
    name: 'gzstream',
    version: meson.project_version(),
    description: 'Gzstream is a small C++ library, basically just a wrapper, that provides the functionality of the zlib C-library in a C++ iostream.',
)
zlib_dep = dependency('ZLIB')
gzstream_dep = declare_dependency(
    sources: ['gzstream.C'],
    dependencies: zlib_dep,
    include_directories: include_directories('.'),
)
