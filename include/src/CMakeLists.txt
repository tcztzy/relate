project (relate)

find_package(cxxopts CONFIG REQUIRED)
add_library(relateStatic STATIC filesystem plot fast_log collapsed_matrix data sample fast_painting anc mutations tree_builder branch_length_estimator anc_builder tree_comparer)
target_link_libraries(relateStatic gzstreamStatic cxxopts::cxxopts)
add_library(relateShared SHARED filesystem plot fast_log collapsed_matrix data sample fast_painting anc mutations tree_builder branch_length_estimator anc_builder tree_comparer)
target_link_libraries(relateShared gzstreamShared cxxopts::cxxopts)
