#include <iostream>
#include <gzstream.h>
#include "collapsed_matrix.hpp"
#include "cxxopts.hpp"
#include "usage.hpp"

#include <ctime>
#include <tgmath.h>

int SummarizeCoalescentRateForGenome(cxxopts::ParseResult& result, const std::string& help_text){

  //////////////////////////////////
  //Program options

  bool help = false;
  if( ((!result.count("first_chr") || !result.count("last_chr")) && !result.count("chr")) || !result.count("output")){
    std::cout << "Not enough arguments supplied." << std::endl;
    std::cout << "Needed: chr or (first_chr, last_chr), output." << std::endl;
    help = true;
  }
  if(result.count("help") || help){
    std::cout << help_text << std::endl;
    std::cout << "Summarizes .bin files for chromosomes." << std::endl;
    exit(0);
  }  

  std::cerr << "---------------------------------------------------------" << std::endl;
  std::cerr << "Summarizing over chromosomes..." << std::endl;  

  //calculate epoch times
	std::vector<std::string> filenames;
	std::string filename_base = result["output"].as<std::string>();
	if(result.count("chr")){

		igzstream is_chr(result["chr"].as<std::string>());
		if(is_chr.fail()){
			std::cerr << "Error while opening file " << result["chr"].as<std::string>() << std::endl;
		}
		std::string line;
		while(getline(is_chr, line)){
			filenames.push_back(filename_base + "_chr" + line + ".bin");
		}
		is_chr.close();

	}else{

    int start     = result["first_chr"].as<int>(); 
    int end       = result["last_chr"].as<int>();
		for(int chr = start; chr <= end; chr++){
			filenames.push_back(filename_base + "_chr" + std::to_string(chr) + ".bin");
		}

	}

  //populate coalescent_rate_data
  FILE* fp = fopen(filenames[0].c_str(),"rb");
  assert(fp != NULL);

  int num_epochs;
  std::vector<float> epochs;
  fread(&num_epochs, sizeof(int), 1, fp);
  epochs.resize(num_epochs);
  fread(&epochs[0], sizeof(float), num_epochs, fp);

  std::vector<CollapsedMatrix<float>> coalescent_rate_data(num_epochs);
  for(int e = 0; e < num_epochs; e++){
    coalescent_rate_data[e].ReadFromFile(fp); 
  }
  fclose(fp);
  std::remove(filenames[0].c_str());

  std::vector<float>::iterator it_coalescent_rate_data;
  CollapsedMatrix<float> coalescent_rate_data_section;
  for(int i = 1; i < (int) filenames.size(); i++){
    fp = fopen(filenames[i].c_str(),"rb");
    assert(fp != NULL);
    fread(&num_epochs, sizeof(int), 1, fp); 
    fread(&epochs[0], sizeof(float), num_epochs, fp);

    for(int e = 0; e < num_epochs; e++){    
      coalescent_rate_data_section.ReadFromFile(fp);
      it_coalescent_rate_data     = coalescent_rate_data[e].vbegin();
      for(std::vector<float>::iterator it_coalescent_rate_data_section = coalescent_rate_data_section.vbegin(); it_coalescent_rate_data_section != coalescent_rate_data_section.vend();){
        *it_coalescent_rate_data += *it_coalescent_rate_data_section;
        it_coalescent_rate_data_section++;
        it_coalescent_rate_data++;
      }
    }
    fclose(fp);
    std::remove(filenames[i].c_str());
  }

  //output as bin
  fp = fopen((result["output"].as<std::string>() + ".bin").c_str(), "wb");  

  fwrite(&num_epochs, sizeof(int), 1, fp);
  fwrite(&epochs[0], sizeof(float), epochs.size(), fp);
  for(std::vector<CollapsedMatrix<float>>::iterator it_coalescent_rate_data = coalescent_rate_data.begin(); it_coalescent_rate_data != coalescent_rate_data.end();){
    (*it_coalescent_rate_data).DumpToFile(fp);
    it_coalescent_rate_data++;
  }
  fclose(fp);


  ResourceUsage();


  return 0;

}
