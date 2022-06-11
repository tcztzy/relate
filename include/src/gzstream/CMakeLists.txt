project (gzstream)

add_library(gzstreamStatic STATIC gzstream.cpp)
target_link_libraries(gzstreamStatic ${ZLIB_LIBRARIES})
add_library(gzstreamShared SHARED gzstream.cpp)
target_link_libraries(gzstreamShared ${ZLIB_LIBRARIES})
