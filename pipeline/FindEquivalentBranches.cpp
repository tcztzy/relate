// Find equivalent branches in neighbouring trees

#include <filesystem>
#include <iostream>

#include "anc.hpp"
#include "anc_builder.hpp"
#include "data.hpp"
#include "usage.hpp"
namespace fs = std::filesystem;

int FindEquivalentBranches(std::string output, int chunk_index) {
  std::string file_out = output + "/";

  int N, L, num_windows;
  FILE *fp =
      fopen((file_out + "parameters_c" + std::to_string(chunk_index) + ".bin")
                .c_str(),
            "r");
  assert(fp != NULL);
  fread(&N, sizeof(int), 1, fp);
  fread(&L, sizeof(int), 1, fp);
  fread(&num_windows, sizeof(int), 1, fp);
  fclose(fp);
  num_windows--;

  std::cerr << "---------------------------------------------------------"
            << std::endl;
  std::cerr << "Propagating mutations across AncesTrees..." << std::endl;

  //////////////////////////////////
  // Parse Data

  Data data(N, L); // struct data is defined in data.hpp
  data.name =
      (file_out + "chunk_" + std::to_string(chunk_index) + "/paint/relate");
  const std::string dirname =
      file_out + "chunk_" + std::to_string(chunk_index) + "/";

  /////////////////////////////
  // delete painting and data binaries
  // check if directory exists
  if (fs::exists((file_out + "chunk_" + std::to_string(chunk_index) + "/paint/")
                     .c_str())) {
    // paint/ exists so delete it.
    char painting_filename[1024];
    for (int w = 0; w < num_windows; w++) {
      snprintf(painting_filename, sizeof(char) * 1024, "%s_%i.bin",
               data.name.c_str(), w);
      std::remove(painting_filename);
    }
  }
  std::remove(
      (file_out + "chunk_" + std::to_string(chunk_index) + ".hap").c_str());
  std::remove(
      (file_out + "chunk_" + std::to_string(chunk_index) + ".r").c_str());
  std::remove(
      (file_out + "chunk_" + std::to_string(chunk_index) + ".rpos").c_str());
  std::remove(
      (file_out + "chunk_" + std::to_string(chunk_index) + ".state").c_str());
  //////////////////

  AncesTree anc;
  AncesTreeBuilder ancbuilder(data);
  ancbuilder.PreCalcPotentialBranches(); // precalculating the number of
                                         // decendants a branch needs to be
                                         // equivalent (narrowing search space)

  CorrTrees::iterator it_seq_prev;
  CorrTrees::iterator it_seq;

  /////
  // First anc
  std::string filename = dirname + output + "_0.anc";
  anc.ReadBin(filename);
  for (int anc_index = 0; anc_index < num_windows; anc_index++) {

    // Find equivalent branches
    it_seq_prev = anc.seq.begin();
    it_seq = std::next(it_seq_prev, 1);

    std::vector<std::vector<int>> equivalent_branches;
    std::vector<std::vector<int>>::iterator it_equivalent_branches;

    for (; it_seq != anc.seq.end();) {
      equivalent_branches.emplace_back();
      it_equivalent_branches = std::prev(equivalent_branches.end(), 1);
      ancbuilder.BranchAssociation((*it_seq_prev).tree, (*it_seq).tree,
                                   *it_equivalent_branches); // O(N^2)
      it_seq++;
      it_seq_prev++;
    }

    // If its not the last window, I have to find equivalent branches to the anc
    // of the next window
    if (anc_index < num_windows - 1) {
      AncesTree anc_next;
      filename =
          dirname + output + "_" + std::to_string(anc_index + 1) + ".anc";
      anc_next.ReadBin(filename);

      it_seq = anc_next.seq.begin();
      equivalent_branches.emplace_back();
      it_equivalent_branches = std::prev(equivalent_branches.end(), 1);

      ancbuilder.BranchAssociation((*it_seq_prev).tree, (*it_seq).tree,
                                   *it_equivalent_branches); // O(N^2)
      anc = anc_next;
    }

    // Write equivalent_branches to file
    std::string output_filename =
        dirname + "equivalent_branches_" + std::to_string(anc_index) + ".bin";
    FILE *pf = fopen(output_filename.c_str(), "wb");
    assert(pf != NULL);

    int size = equivalent_branches.size();
    fwrite(&size, sizeof(int), 1, pf);

    int N_total = 2 * N - 1;
    for (int i = 0; i < size; i++) {
      fwrite(&equivalent_branches[i][0], sizeof(int), N_total, pf);
    }
    fclose(pf);
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////
  // Propagate mutations

  std::vector<AncesTree> v_anc(num_windows);

  // Read ancs
  for (int i = 0; i < num_windows; i++) {
    filename = dirname + output + "_" + std::to_string(i) + ".anc";
    v_anc[i].ReadBin(filename);
  }

  // Associate branches of adjacent trees
  ancbuilder.AssociateTrees(v_anc, dirname);

  // Write ancs back to files
  for (int i = 0; i < num_windows; i++) {
    filename = dirname + output + "_" + std::to_string(i) + ".anc";
    v_anc[i].DumpBin(filename);
  }

  for (int anc_index = 0; anc_index < num_windows; anc_index++) {
    filename =
        dirname + "equivalent_branches_" + std::to_string(anc_index) + ".bin";
    std::remove(filename.c_str());
  }

  ResourceUsage();

  return 0;
}
