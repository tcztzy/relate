//Make overlapping chunks with fixed number of SNPs from data set
#ifndef MAKE_CHUNKS
#define MAKE_CHUNKS

#include <iostream>
#include <sys/time.h>
#include <sys/resource.h>
#include <gzstream.h>
#include <cxxopts.hpp>

#include "filesystem.hpp"
#include "data.hpp"
#include "usage.hpp"

int MakeChunks(cxxopts::ParseResult& result, const std::string& help_text, int chunk_size = 0){

  bool help = false;
  if(!result.count("haps") || !result.count("sample") || !result.count("map") || !result.count("output")){
    std::cout << "Not enough arguments supplied." << std::endl;
    std::cout << "Needed: haps, sample, map, output. Optional: memory, dist, transversion." << std::endl;
    help = true;
  }
  if(result.count("help") || help){
    std::cout << help_text << std::endl;
    std::cout << "Use to make smaller chunks from the data." << std::endl;
    exit(0);
  }

  std::cerr << "---------------------------------------------------------" << std::endl;
  std::cerr << "Parsing data.." << std::endl;

  bool use_transitions = true;
  if(result.count("transversion")) use_transitions = false;
 
  //////////////////////////////////
  //Parse Data

  struct stat info;
  //check if directory exists
  if( stat( (result["output"].as<std::string>() + "/").c_str(), &info ) == 0 ){
    std::cerr << "Error: Directory " << result["output"].as<std::string>() << " already exists. Relate will use this directory to store temporary files." << std::endl;
    exit(1);
  }
  filesys f;
  f.MakeDir((result["output"].as<std::string>() + "/").c_str());

  Data data;
  /*
  if(options.count("chunk_size")){

    if(options.count("dist")){
      data.MakeChunks(options["haps"].as<std::string>(), options["sample"].as<std::string>(), options["map"].as<std::string>(), options["dist"].as<std::string>(), options["chunk_size"].as<int>());
    }else{
      data.MakeChunks(options["haps"].as<std::string>(), options["sample"].as<std::string>(), options["map"].as<std::string>(), "unspecified", options["chunk_size"].as<int>());
    }

  }else{

  }
  */

  if(result.count("memory")){
    if(result.count("dist")){
      data.MakeChunks(result["haps"].as<std::string>(), result["sample"].as<std::string>(), result["map"].as<std::string>(), result["dist"].as<std::string>(), result["output"].as<std::string>(), use_transitions, result["memory"].as<float>());
    }else{
      data.MakeChunks(result["haps"].as<std::string>(), result["sample"].as<std::string>(), result["map"].as<std::string>(), "unspecified", result["output"].as<std::string>(), use_transitions, result["memory"].as<float>());
    }
  }else{
    if(result.count("dist")){
      data.MakeChunks(result["haps"].as<std::string>(), result["sample"].as<std::string>(), result["map"].as<std::string>(), result["dist"].as<std::string>(), result["output"].as<std::string>(), use_transitions);
    }else{
      data.MakeChunks(result["haps"].as<std::string>(), result["sample"].as<std::string>(), result["map"].as<std::string>(), "unspecified", result["output"].as<std::string>(), use_transitions);
    }
  }
  
  std::vector<double> sample_ages(data.N);
  if(result.count("sample_ages")){
    igzstream is_ages(result["sample_ages"].as<std::string>());
		if(is_ages.fail()){
			std::cerr << "Warning: unable to open sample ages file" << std::endl;
		}else{
			int i = 0; 
			while(is_ages >> sample_ages[i]){
				i++;
				//sample_ages[i] = sample_ages[i-1];
				//i++;
				if(i == data.N) break;
			}
			if(i != data.N){
				std::cerr << "Warning: sample ages specified but number of samples does not agree with other input files. Ignoring sample ages." << std::endl;
				sample_ages.clear();
			}
		}
  }else{

    sample_ages.clear(); 
  }


  ResourceUsage();

  return 0;

}

#endif //MAKE_CHUNKS
