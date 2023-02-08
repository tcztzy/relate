//Combining all .anc and .mut files into one.

#include <iostream>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h> // required for stat.h
#include <sys/stat.h> // no clue why required -- man pages say so
#include <string>

#include "data.hpp"
#include "anc.hpp"
#include "mutations.hpp"
#include "usage.hpp"



int CombineSections(std::string output, int chunk_index = 0, int Ne = 30000){
  std::cerr << "---------------------------------------------------------" << std::endl;
  std::cerr << "Combining AncesTrees in Sections..." << std::endl;

  std::string file_out = output + "/";

  int N, L, num_windows;
  FILE* fp = fopen((file_out + "parameters_c" + std::to_string(chunk_index) + ".bin").c_str(), "r");
  assert(fp != NULL);
  fread(&N, sizeof(int), 1, fp);
  fread(&L, sizeof(int), 1, fp);
  fread(&num_windows, sizeof(int), 1, fp);
  fclose(fp);
  num_windows--;

  //////////////////////////////////
  //Parse Data

  Data data(N, L, Ne); //struct data is defined in data.hpp
  const std::string dirname = file_out + "chunk_" + std::to_string(chunk_index) + "/";
  const std::string output_file = dirname + output;

  ///////////////////////////////////////// Combine AncesTrees /////////////////////////

  AncesTree anc, anc_tmp;

  int num_tree = 0;
  std::string filename;
 
  //read ancs and splice together
  CorrTrees::iterator it_anc = anc.seq.end();
  for(int i = 0; i < num_windows; i++){
    it_anc = anc.seq.end();
    filename = output_file + "_" + std::to_string(i) + ".anc";
    anc_tmp.ReadBin(filename);
    anc.seq.splice(it_anc, anc_tmp.seq);
    num_tree += anc_tmp.seq.size();
  }
  anc.N = data.N;
  anc.L = num_tree; 

  ///////////////////////////////////////// Create Mutations File /////////////////////////

  std::vector<std::string> mut_filenames(num_windows);
  for(int i = 0; i < num_windows; i++){
    mut_filenames[i] = output_file + "_" + std::to_string(i) + ".mut";
  }
  Mutations mutations(data);

  mutations.ReadShortFormat(mut_filenames);
  mutations.GetAge(anc);
 
  //////////////////////////////////////// Output //////////////////////////////////
  //Dump AncesTree to File

  anc.DumpBin(output_file + "_c" + std::to_string(chunk_index) + ".anc");
  mutations.DumpShortFormat(output_file + "_c" + std::to_string(chunk_index) + ".mut");

  //////////////////////////////////////// Delete tmp files //////////////////////////////////

  for(int i = 0; i < num_windows; i++){
    std::remove((output_file + "_" + std::to_string(i) + ".anc").c_str());
    std::remove((output_file + "_" + std::to_string(i) + ".mut").c_str());
  }
	std::remove((file_out + "chunk_" + std::to_string(chunk_index) + ".bp").c_str());  
	std::remove((file_out + "chunk_" + std::to_string(chunk_index) + ".dist").c_str());
  std::remove((file_out + "parameters_c" + std::to_string(chunk_index) + ".bin").c_str());

  ResourceUsage();

  return 0;
}
