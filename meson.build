project(
    'Relate',
    'c',
    'cpp',
    version: '1.1.9',
    default_options: ['cpp_std=c++17'],
)
subdir('src')
cxxopts_proj = subproject('cxxopts')
cxxopts_dep = cxxopts_proj.get_variable('cxxopts_dep')

executable(
    'Relate',
    'pipeline/Relate.cpp',
    install: true,
    dependencies: [relate, cxxopts_dep],
)
executable(
    'RelateExtract',
    'extract/RelateExtract.cpp',
    install: true,
    dependencies: [relate, cxxopts_dep],
)
executable(
    'RelateCoalescentRate',
    'evaluate/coalescent_rate/RelateCoalescentRate.cpp',
    'evaluate/coalescent_rate/coal_tree.cpp',
    install: true,
    dependencies: [relate, cxxopts_dep],
)
executable(
    'RelateSelection',
    'evaluate/selection/RelateSelection.cpp',
    install: true,
    dependencies: [relate, cxxopts_dep],
)
executable(
    'RelateMutationRate',
    'evaluate/mutation_rate/RelateMutationRate.cpp',
    install: true,
    dependencies: [relate, cxxopts_dep],
)
executable(
    'RelateTreeView',
    'treeview/RelateTreeView.cpp',
    install: true,
    dependencies: [relate, cxxopts_dep],
)

tskit_proj = subproject('tskit')
tskit_dep = tskit_proj.get_variable('tskit_dep')
executable(
    'RelateFileFormat',
    'file_formats/RelateFileFormats.cpp',
    install: true,
    dependencies: [relate, tskit_dep, cxxopts_dep],
)
catch2_proj = subproject('catch2')
catch2_with_main_dep = catch2_proj.get_variable('catch2_with_main_dep')
Test = executable(
    'Test',
    'test/Tests.cpp',
    dependencies: [relate, catch2_with_main_dep],
)
test('Run tests', Test)
