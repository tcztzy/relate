project (relate)

find_package(cxxopts 3.0.0 REQUIRED)
add_library(relateStatic STATIC filesystem.cpp plot.cpp fast_log.cpp collapsed_matrix.cpp data.cpp sample.cpp fast_painting.cpp anc.cpp mutations.cpp tree_builder.cpp branch_length_estimator.cpp anc_builder.cpp tree_comparer.cpp)
target_link_libraries(relateStatic gzstreamStatic cxxopts::cxxopts)
add_library(relateShared SHARED filesystem.cpp plot.cpp fast_log.cpp collapsed_matrix.cpp data.cpp sample.cpp fast_painting.cpp anc.cpp mutations.cpp tree_builder.cpp branch_length_estimator.cpp anc_builder.cpp tree_comparer.cpp)
target_link_libraries(relateShared gzstreamShared cxxopts::cxxopts)
