#ifndef FFI_HPP
#define FFI_HPP
#include <vector>
inline std::vector<double> construct_vector_double(const double *value, size_t len) {
    std::vector<double> v(value, value + len);
    return v;
}
int FindEquivalentBranches(std::string output, int chunk_index);
#endif