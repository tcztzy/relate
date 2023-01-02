gzstream_proj = subproject('gzstream')
gzstream_dep = gzstream_proj.get_variable('gzstream_dep')
relate_sources = [
    'fast_painting.cpp',
    'anc.cpp',
    'anc_builder.cpp',
    'branch_length_estimator.cpp',
    'tree_builder.cpp',
    'data.cpp',
    'mutations.cpp',
    'plot.cpp',
    'sample.cpp',
    'tree_comparer.cpp',
]
librelate = library(
    'relate',
    relate_sources,
    version: '1',
    install: true,
    dependencies: gzstream_dep,
)
relate = declare_dependency(
    link_with: librelate,
    dependencies: gzstream_dep,
    include_directories: include_directories('.'),
)
