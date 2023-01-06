# Relate 
## What I did compare with [MyersGroup/Relate](https://github.com/MyersGroup/Relate)?
1. Copy C++ code source and start from new HEAD.
   Origin repository contains large binary files.
2. Use [mesonbuild](https://mesonbuild.com) to replace CMake.
   WrapDB is a good choice for C++ dependency management.
3. gzstream/tskit/cxxopts/catch2 code are removed from repository and upgrade to latest.
4. Only include minimum required headers to optimize header file dependencies.
5. Use [google/autocxx](https://github.com/google/autocxx) to generate Rust binding and refactor CLI in Rust.
