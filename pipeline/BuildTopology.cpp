//Build topology for trees in regions of 500 SNPs.
// Input: Forward and Backward probabilities calculating in Painting
// Output: .anc and .mut file 

#include <iostream>
#include <sys/time.h>
#include <sys/resource.h>
#include <cxxopts.hpp>

#include "data.hpp"
#include "anc.hpp"
#include "anc_builder.hpp"
#include "usage.hpp"

int BuildTopology(cxxopts::ParseResult& result, int chunk_index, int first_section, int last_section){

  //////////////////////////////////
  //Parse Data

  std::string file_out = result["output"].as<std::string>() + "/";

  int N, L, num_windows;
  std::vector<int> window_boundaries;
  FILE* fp = fopen((file_out + "parameters_c" + std::to_string(chunk_index) + ".bin").c_str(), "r");
  assert(fp != NULL);
  fread(&N, sizeof(int), 1, fp);
  fread(&L, sizeof(int), 1, fp);
  fread(&num_windows, sizeof(int), 1, fp);
  window_boundaries.resize(num_windows);
  fread(&window_boundaries[0], sizeof(int), num_windows, fp);
  fclose(fp);
  num_windows--;

	Data data((file_out + "chunk_" + std::to_string(chunk_index) + ".hap").c_str(), (file_out + "chunk_" + std::to_string(chunk_index) + ".bp").c_str(), (file_out + "chunk_" + std::to_string(chunk_index) + ".dist").c_str(), (file_out + "chunk_" + std::to_string(chunk_index) + ".r").c_str(), (file_out + "chunk_" + std::to_string(chunk_index) + ".rpos").c_str(), (file_out + "chunk_" + std::to_string(chunk_index) + ".state").c_str()); //struct data is defined in data.hpp
  data.name = (file_out + "chunk_" + std::to_string(chunk_index) + "/paint/relate");

  if(result.count("effectiveN")){
    data.Ne = result["effectiveN"].as<float>();
  }
  data.Ne *= 50;

  const std::string dirname = file_out + "chunk_" + std::to_string(chunk_index) + "/";
  if(first_section >= num_windows) return 1;

	if(result.count("painting")){

		std::string val;
		std::string painting = result["painting"].as<std::string>();
		int i = 0;
		for(;i < painting.size(); i++){
			if(painting[i] == ',') break;
			val += painting[i];
		}
		data.theta = std::stof(val);
		data.ntheta = 1.0 - data.theta;

		i++;
		val.clear();
		for(;i < painting.size(); i++){
			if(painting[i] == ',') break;
			val += painting[i];
		}
		double rho = std::stof(val);
		for(int l = 0; l < (int)data.r.size(); l++){
			data.r[l] *= rho;
		}

	}

  int seed;
  if(!result.count("seed")){
		seed = std::time(0) + getpid();
  }else{
    seed = result["seed"].as<int>();
		srand(seed);
		for(int i = 0; i < chunk_index + 100*first_section; i++){
      seed = rand();
		}
  }
	srand(seed);

  bool ancestral_state = true;
  if(result.count("anc_allele_unknown")){
     ancestral_state = false;
  }

  std::vector<double> sample_ages(N);
  if(result.count("sample_ages")){
    igzstream is_ages(result["sample_ages"].as<std::string>());
		if(is_ages.fail()){
			std::cerr << "Warning: unable to open sample ages file" << std::endl;
		}else{
			int i = 0; 
			while(is_ages >> sample_ages[i]){
				i++;
				if(i == N) break;
			}
			if(i < N) sample_ages.clear();
		}
  }else{
    sample_ages.clear(); 
  }
  //sample_ages.clear();

  int fb = 0;
	if(result.count("fb")){
	  fb = result["fb"].as<float>();
	}

  ///////////////////////////////////////////// Build AncesTree //////////////////////////
  //input:  Data and distance matrix
  //output: AncesTree (tree sequence)

  last_section = std::min(num_windows-1, last_section);

  std::cerr << "---------------------------------------------------------" << std::endl;
  std::cerr << "Estimating topologies of AncesTrees in sections " << first_section << "-" << last_section << "..." << std::endl;

  for(int section = first_section; section <= last_section; section++){

    std::cerr << "[" << section << "/" << last_section << "]\r";
    std::cerr.flush(); 

    AncesTree anc;
    AncesTreeBuilder ancbuilder(data, sample_ages);

    int section_startpos = window_boundaries[section];
    int section_endpos   = window_boundaries[section+1]-1;
    if(section_endpos >= data.L) section_endpos = data.L-1;

    ancbuilder.BuildTopology(section, section_startpos, section_endpos, data, anc, rand(), ancestral_state, fb);

    /////////////////////////////////////////// Dump AncesTree to File //////////////////////

    anc.DumpBin(dirname + result["output"].as<std::string>() + "_" + std::to_string(section) + ".anc");
    ancbuilder.mutations.DumpShortFormat(dirname + result["output"].as<std::string>() + "_" + std::to_string(section) + ".mut", section_startpos, section_endpos);

  }

  ResourceUsage();

  return 0;
}
