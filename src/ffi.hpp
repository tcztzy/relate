#ifndef FFI_HPP
#define FFI_HPP
#include <vector>
inline std::vector<double> construct_vector_double(const double *value, size_t len) {
    std::vector<double> v(value, value + len);
    return v;
}
int FindEquivalentBranches(std::string output, int chunk_index);
int GetBranchLengths(std::string output, int chunk_index, int first_section, int last_section, double mutation_rate, const double *effectiveN, const std::string *sample_ages_path, const std::string *coal, const int *const_seed);
int CombineSections(std::string output, int chunk_index, int Ne);
int Finalize(std::string output, const std::string *sample_ages_path, const std::string *annot);
#endif