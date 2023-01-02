//Finalizing
#ifndef CLEAN
#define CLEAN

#include <filesystem>
#include <cxxopts.hpp>

#include "usage.hpp"
namespace fs = std::filesystem;


int Clean(cxxopts::ParseResult& result, const std::string& help_text){

  bool help = false;
  if(!result.count("output")){
    std::cout << "Not enough arguments supplied." << std::endl;
    std::cout << "Needed: output." << std::endl;
    help = true;
  }
  if(result.count("help") || help){
    std::cout << help_text << std::endl;
    std::cout << "This function will attempt to delete all temporary files created by Relate. Use when Relate crashes." << std::endl;
    exit(0);
  }


  std::cerr << "---------------------------------------------------------" << std::endl;
  std::cerr << "Cleaning directory..." << std::endl;

  std::string file_out = result["output"].as<std::string>() + "/";

  int N, L, num_chunks;
  FILE* fp = fopen((file_out + "parameters.bin").c_str(), "r");
  if(fp == NULL){
    std::cerr << "Cannot delete files. Please delete temporary files manually." << std::endl;
    exit(1);
  }
  fread(&N, sizeof(int), 1, fp);
  fread(&L, sizeof(int), 1, fp);
  fread(&num_chunks, sizeof(int), 1, fp);
  fclose(fp); 
 
  /////////////////////////////
  //delete painting and data binaries
  
  std::string filename, output_filename; 
  for(int chunk_index = 0; chunk_index < num_chunks; chunk_index++){

    std::string dirname = file_out + "chunk_" + std::to_string(chunk_index) + "/";
    //check if directory exists
    if( fs::exists(dirname) ){

      int N, L, num_windows;
      fp = fopen((file_out + "parameters_c" + std::to_string(chunk_index) + ".bin").c_str(), "r");
      if(fp != NULL){

        fread(&N, sizeof(int), 1, fp);
        fread(&L, sizeof(int), 1, fp);
        fread(&num_windows, sizeof(int), 1, fp);
        fclose(fp);
        num_windows--;

        for(int i = 0; i < num_windows; i++){
          filename        = dirname + "equivalent_branches_" + std::to_string(i) + ".bin";
          output_filename = dirname + result["output"].as<std::string>();
          std::remove(filename.c_str());
          std::remove((output_filename + "_" + std::to_string(i) + ".anc").c_str());
          std::remove((output_filename + "_" + std::to_string(i) + ".mut").c_str());
        }


        //check if directory exists
        if (fs::exists(file_out + "chunk_" + std::to_string(chunk_index) + "/paint/")) {
          //paint/ exists so delete it.  
          char painting_filename[1024];
          for(int w = 0; w < num_windows; w++){
						snprintf(painting_filename, sizeof(char) * 1024, "%s_%i.bin", (file_out + "chunk_" + std::to_string(chunk_index) + "/paint/relate").c_str(), w);
						std::remove(painting_filename);
          }
        }

      }

      std::string file_prefix = dirname + result["output"].as<std::string>();
      std::remove((file_prefix + "_c" + std::to_string(chunk_index) + ".mut").c_str());
      std::remove((file_prefix + "_c" + std::to_string(chunk_index) + ".anc").c_str());

    }
 
    std::remove((file_out + "chunk_" + std::to_string(chunk_index) + ".hap").c_str());
    std::remove((file_out + "chunk_" + std::to_string(chunk_index) + ".r").c_str());
    std::remove((file_out + "chunk_" + std::to_string(chunk_index) + ".rpos").c_str());
    std::remove((file_out + "chunk_" + std::to_string(chunk_index) + ".state").c_str());
    std::remove((file_out + "chunk_" + std::to_string(chunk_index) + ".bp").c_str());
    std::remove((file_out + "parameters_c" + std::to_string(chunk_index) + ".bin").c_str());

  }

  for(int c = 0; c < num_chunks; c++){
    //now delete directories
    if (fs::exists((file_out + "chunk_" + std::to_string(c) + "/paint/").c_str())) fs::remove_all((file_out + "chunk_" + std::to_string(c) + "/paint/"));
    if (fs::exists((file_out + "chunk_" + std::to_string(c) + "/").c_str())) fs::remove_all((file_out + "chunk_" + std::to_string(c) + "/"));
  }

  std::remove((file_out + "parameters.bin").c_str());
  std::remove((file_out + "props.bin").c_str());

  if (fs::exists(file_out)) fs::remove_all(file_out);

  ResourceUsage();

  return 0;

}

#endif //CLEAN
