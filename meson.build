project('Relate', 'c', 'cpp',
  version: '1.1.9',
  default_options : ['cpp_std=c++14'])
subdir('src')
Relate = executable('Relate', 'pipeline/Relate.cpp',
  install : true,
  dependencies : [relate],
)
