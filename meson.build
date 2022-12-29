project('Relate', 'c', 'cpp',
  version: '1.1.9',
  default_options : ['cpp_std=c++14'])
zlib = dependency('ZLIB')
Relate = executable('Relate', 'pipeline/Relate.cpp', 'src/gzstream/gzstream.cpp', 'src/fast_painting.cpp', 'src/anc.cpp', 'src/anc_builder.cpp', 'src/branch_length_estimator.cpp', 'src/tree_builder.cpp', 'src/data.cpp', 'src/filesystem.cpp', 'src/mutations.cpp',
  install : true,
  include_directories : include_directories('src', 'src/gzstream'),
  dependencies : [zlib],
)
