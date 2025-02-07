//Inference of branch lengths using EM and MCMC.

#include <iostream>
#include <sys/time.h>
#include <sys/resource.h>
#include <string>
#include <sstream>

#include "data.hpp"
#include "anc.hpp"
#include "branch_length_estimator.hpp"
#include "anc_builder.hpp"
#include "tree_builder.hpp"
#include "usage.hpp"

int GetBranchLengths(std::string output, int chunk_index, int first_section, int last_section, double mutation_rate, const double *effectiveN, const std::string *sample_ages_path, const std::string *coal, const int *const_seed){
  int seed;
  if(const_seed == NULL){
    seed = std::time(0) + getpid();
  }else{
		srand(*const_seed);
		for(int i = 0; i < chunk_index + 100*first_section; i++){
			seed = rand();
		}
  }
	srand(seed);

  std::string file_out = output + "/";

  int N, L, num_windows;
  FILE* fp = fopen((file_out + "parameters_c" + std::to_string(chunk_index) + ".bin").c_str(), "r");
  assert(fp != NULL);
  fread(&N, sizeof(int), 1, fp);
  fread(&L, sizeof(int), 1, fp);
  fread(&num_windows, sizeof(int), 1, fp);
  fclose(fp);
  num_windows--;

  int Ne = effectiveN == NULL ? 30000 : *effectiveN;
  Data data((file_out + "chunk_" + std::to_string(chunk_index) + ".dist").c_str(), (file_out + "parameters_c" + std::to_string(chunk_index) + ".bin").c_str(), Ne, mutation_rate); //struct data is defined in data.hpp 
  const std::string dirname = file_out + "chunk_" + std::to_string(chunk_index) + "/";

  std::cerr << "---------------------------------------------------------" << std::endl;
  std::cerr << "Inferring branch lengths of AncesTrees in sections " << first_section << "-" << last_section << "..." << std::endl;
 
  bool is_coal = false;
  std::vector<double> epoch, coalescent_rate;
  if(coal != NULL){

    std::string line;
    double tmp;
    is_coal = true;
    // read epochs and population size 
    std::ifstream is(*coal); 
    getline(is, line);
    getline(is, line);
    std::istringstream is_epoch(line);
    while(is_epoch){
      is_epoch >> tmp;
      epoch.push_back(tmp/data.Ne);
    }
    getline(is, line);
    is.close();

    std::istringstream is_pop_size(line);
    is_pop_size >> tmp >> tmp;
    while(is_pop_size){
      is_pop_size >> tmp;
      //tmp = 1.0/data.Ne; 
      if(tmp == 0.0 && coalescent_rate.size() > 0){
        if(*std::prev(coalescent_rate.end(),1) > 0.0){
          coalescent_rate.push_back(*std::prev(coalescent_rate.end(),1));
        }
        //coalescent_rate.push_back(1);
      }else{
        coalescent_rate.push_back(tmp * data.Ne);
      }
    }

    for(int i = (int)coalescent_rate.size()-1; i > 0; i--){
      if(coalescent_rate[i-1] == 0){
        if(coalescent_rate[i] > 0.0){
          coalescent_rate[i-1] = coalescent_rate[i];
        }else{
          coalescent_rate[i-1] = 1.0;
        }
      } 
    } 

  }


  //////////////////////////////////
  //Parse Data
  if(first_section >= num_windows) return 1;

  std::vector<double> sample_ages(N);
  if(sample_ages_path != NULL){
    igzstream is_ages(*sample_ages_path);
    int i = 0; 
    while(is_ages >> sample_ages[i]){
      i++;
      //sample_ages[i] = sample_ages[i-1];
      //i++;
      if(i == N) break;
    }
    if(i < N) sample_ages.clear();
  }else{
    sample_ages.clear(); 
  }

  ///////////////////////////////////////// TMRCA Inference /////////////////////////
  //Infer Branchlengths

  if(sample_ages.size() == 0){

    last_section = std::min(num_windows-1, last_section);
    for(int section = first_section; section <= last_section; section++){

      std::cerr << "[" << section << "/" << last_section << "]\r";
      std::cerr.flush(); 

      //Read anc
      AncesTree anc;
      std::string filename; 
      filename = dirname + output + "_" + std::to_string(section) + ".anc";
      anc.ReadBin(filename);

      //Infer branch lengths
      InferBranchLengths bl(data);
      //EstimateBranchLengths bl2(data);
      //EstimateBranchLengthsWithSampleAge bl2(data, sample_ages);

      int num_sec = (int) anc.seq.size()/100.0 + 1;

      if(is_coal){
        for(CorrTrees::iterator it_seq = anc.seq.begin(); it_seq != anc.seq.end(); it_seq++){
          bl.MCMCVariablePopulationSizeForRelate(data, (*it_seq).tree, epoch, coalescent_rate, rand()); //this is estimating times
          //bl2.MCMCVariablePopulationSizeSample(data, (*it_seq).tree, epoch, coalescent_rate, 2e4, 1, rand());
        }
      }else{
        int count = 0;
        CorrTrees::iterator it_seq = anc.seq.begin();
        for(; it_seq != anc.seq.end(); it_seq++){
          if(count % num_sec == 0){
            std::cerr << "[" << section << "/" << last_section << "] " << "[" << count << "/" << anc.seq.size() << "]\r";
            std::cerr.flush(); 
          }
          count++;
          //bl.MCMC(data, (*it_seq).tree, seed); //this is estimating times
          bl.MCMC(data, (*it_seq).tree, rand()); //this is estimating times
        }
        std::cerr << "[" << section << "/" << last_section << "] " << "[" << count << "/" << anc.seq.size() << "]\r";
      }

      //Dump to file
      anc.DumpBin(filename);

    }

  }else{

    last_section = std::min(num_windows-1, last_section);
    for(int section = first_section; section <= last_section; section++){

      std::cerr << "[" << section << "/" << last_section << "]\r";
      std::cerr.flush(); 

      //Read anc
      AncesTree anc;
      std::string filename; 
      filename = dirname + output + "_" + std::to_string(section) + ".anc";
      anc.ReadBin(filename);
      anc.sample_ages = sample_ages;

      //Infer branch lengths
      EstimateBranchLengthsWithSampleAge bl(data, anc.sample_ages);

      int num_sec = (int) anc.seq.size()/100.0 + 1;

      if(is_coal){
        for(CorrTrees::iterator it_seq = anc.seq.begin(); it_seq != anc.seq.end(); it_seq++){
          bl.MCMCVariablePopulationSizeForRelate(data, (*it_seq).tree, epoch, coalescent_rate, rand()); //this is estimating times
          //bl.MCMCVariablePopulationSizeSample(data, (*it_seq).tree, epoch, coalescent_rate, 2e4, 1, rand());
          //for(std::vector<Node>::iterator it_n = (*it_seq).tree.nodes.begin(); it_n != (*it_seq).tree.nodes.end(); it_n++){
          //  (*it_n).branch_length *= 2e4;
          //}
        }
      }else{
        int count = 0;
        CorrTrees::iterator it_seq = anc.seq.begin();
        for(; it_seq != anc.seq.end(); it_seq++){
          if(count % num_sec == 0){
            std::cerr << "[" << section << "/" << last_section << "] " << "[" << count << "/" << anc.seq.size() << "]\r";
            std::cerr.flush(); 
          }
          count++;
          bl.MCMC(data, (*it_seq).tree, rand()); //this is estimating times
        }
        std::cerr << "[" << section << "/" << last_section << "] " << "[" << count << "/" << anc.seq.size() << "]\r";
      }

      //Dump to file
      anc.DumpBin(filename);

    }

  
  
  }

  ResourceUsage();

  return 0;
}
