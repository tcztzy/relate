project (relate)

set(RELATE_SOURCES
  filesystem.cpp
  plot.cpp
  data.cpp
  sample.cpp
  fast_painting.cpp
  anc.cpp
  mutations.cpp
  tree_builder.cpp
  branch_length_estimator.cpp
  anc_builder.cpp
  tree_comparer.cpp
)

add_library(relateStatic STATIC ${RELATE_SOURCES})
target_link_libraries(relateStatic gzstreamStatic)
add_library(relateShared SHARED ${RELATE_SOURCES})
target_link_libraries(relateShared gzstreamShared)
