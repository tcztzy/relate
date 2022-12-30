project('Relate', 'c', 'cpp',
  version: '1.1.9',
  default_options : ['cpp_std=c++14'])
subdir('src')
Relate = executable('Relate', 'pipeline/Relate.cpp',
  install : true,
  dependencies : [relate],
)

executable('RelateExtract', 'extract/RelateExtract.cpp', install : true, dependencies: relate)
executable('RelateCoalescentRate', 'evaluate/coalescent_rate/RelateCoalescentRate.cpp', 'evaluate/coalescent_rate/coal_tree.cpp', install : true, dependencies: relate)
executable('RelateSelection', 'evaluate/selection/RelateSelection.cpp', install : true, dependencies: relate)
executable('RelateMutationRate', 'evaluate/mutation_rate/RelateMutationRate.cpp', install : true, dependencies: relate)
executable('RelateTreeView', 'treeview/RelateTreeView.cpp', install : true, dependencies: relate)

tskit_proj = subproject('tskit')
tskit_dep = tskit_proj.get_variable('tskit_dep')
RelateFileFormat = executable('RelateFileFormat', 'file_formats/RelateFileFormats.cpp',
  install : true,
  dependencies : [relate, tskit_dep],
)
catch2_proj = subproject('catch2')
catch2_with_main_dep = catch2_proj.get_variable('catch2_with_main_dep')
Test = executable('Test', 'test/Tests.cpp', dependencies : [relate, catch2_with_main_dep])
test('Run tests', Test)
