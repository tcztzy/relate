project (tskit)

add_library(kastoreStatic STATIC kastore.c)
add_library(kastoreShared SHARED kastore.c)

add_library(tskitStatic STATIC core.c tables.c trees.c genotypes.c convert.c stats.c tskit.h)

