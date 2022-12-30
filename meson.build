project('Relate', 'c', 'cpp',
  version: '1.1.9',
  default_options : ['cpp_std=c++14'])
subdir('src')
Relate = executable('Relate', 'pipeline/Relate.cpp',
  install : true,
  dependencies : [relate],
)
tskit_proj = subproject('tskit')
tskit_dep = tskit_proj.get_variable('tskit_dep')
RelateFileFormat = executable('RelateFileFormat', 'file_formats/RelateFileFormats.cpp',
  install : true,
  dependencies : [relate, tskit_dep],
)
Test = executable('Test', 'test/Tests.cpp', 'test/catch_amalgamated.cpp', dependencies : relate)
test('Run tests', Test)
