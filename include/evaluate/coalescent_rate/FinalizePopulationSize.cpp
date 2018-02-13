#include <iostream>
#include "plot.hpp"
#include "sample.hpp"
#include "anc.hpp"
#include "anc_builder.hpp"
#include "tree_comparer.hpp"
#include "cxxopts.hpp"

#include <sys/time.h>
#include <sys/resource.h>
#include <ctime>

int FinalizePopulationSize(cxxopts::Options& options){

  //////////////////////////////////
  //Program options

  bool help = false;
  if(!options.count("output")){
    std::cout << "Not enough arguments supplied." << std::endl;
    std::cout << "Needed: output." << std::endl;
    help = true;
  }
  if(options.count("help") || help){
    std::cout << options.help({""}) << std::endl;
    std::cout << "Estimate population size using coalescent rate." << std::endl;
    exit(0);
  } 

  std::cerr << "------------------------------------------------------" << std::endl;
  std::cerr << "Finalizing coalescent rate..." << std::endl;

  ////////// read labels of sequences //////////

  std::string line, seq;
  std::string read;

  /////////////////// INITIALIZE //////////////////////

  int num_epochs;
  std::vector<float> epoch;

  FILE* fp = fopen((options["output"].as<std::string>() + ".bin").c_str(),"rb");
  assert(fp != NULL);

  fread(&num_epochs, sizeof(int), 1, fp);
  epoch.resize(num_epochs);
  fread(&epoch[0], sizeof(float), num_epochs, fp);
  std::vector<CollapsedMatrix<float>> coalescent_rate_data(num_epochs);
  for(int e = 0; e < num_epochs; e++){
    coalescent_rate_data[e].ReadFromFile(fp);
  }
  fclose(fp);

  int N = coalescent_rate_data[0].size();

  std::vector<CollapsedMatrix<float>> coalescent_rate(num_epochs);
  for(std::vector<CollapsedMatrix<float>>::iterator it_c = coalescent_rate.begin(); it_c != coalescent_rate.end(); it_c++){
    (*it_c).resize(1,1);
    std::fill((*it_c).vbegin(), (*it_c).vend(), 0.0);
  }

  for(int i = 0; i < N; i++){
    for(int j = i+1; j < N; j++){

      //int i = 0, j = 1;
      std::vector<CollapsedMatrix<float>>::iterator it_coalescent_rate_data = coalescent_rate_data.begin();
      std::vector<CollapsedMatrix<float>>::iterator it_coalescent_rate      = coalescent_rate.begin();

      for(; it_coalescent_rate_data != std::prev(coalescent_rate_data.end(),1);){
        //i < j always holds
        if((*it_coalescent_rate_data)[i][j] > 0.0){
          (*it_coalescent_rate)[0][0] += (*it_coalescent_rate_data)[i][j]/(*it_coalescent_rate_data)[j][i];
        }
        
        it_coalescent_rate++;
        it_coalescent_rate_data++;
      }    

    }
  }


  std::ofstream os(options["output"].as<std::string>() + ".coal");
  if(os.fail()){
    std::cerr << "Error while opening file." << std::endl;
    exit(1);
  } 

  os << "group1\n";

  for(int e = 0; e < num_epochs; e++){
    os << epoch[e]  << " "; 
  }
  os << "\n";

  std::vector<double> coal(epoch.size());

  os << 0 << " " << 0 << " ";
  for(int e = 0; e < num_epochs; e++){
    coal[e] =  2.0 * coalescent_rate[e][0][0]/((float) (N*(N-1.0)));
    os << coal[e] << " ";
  }
  os << "\n"; 

  for(int e = 0; e < num_epochs; e++){
    if(coal[e] != 0.0) coal[e] = 0.5/coal[e]; 
  }

  plot p(60,10);
  p.draw(epoch, coal);

  os.close();

  /////////////////////////////////////////////
  //Resource Usage

  rusage usage;
  getrusage(RUSAGE_SELF, &usage);

  std::cerr << "CPU Time spent: " << usage.ru_utime.tv_sec << "." << std::setfill('0') << std::setw(6) << usage.ru_utime.tv_usec << "s; Max Memory usage: " << usage.ru_maxrss/1000.0 << "Mb." << std::endl;
  std::cerr << "------------------------------------------------------" << std::endl << std::endl;

  return 0;

}

int FinalizePopulationSizeByGroup(cxxopts::Options& options){

  //////////////////////////////////
  //Program options

  bool help = false;
  if(!options.count("poplabels") || !options.count("output")){
    std::cout << "Not enough arguments supplied." << std::endl;
    std::cout << "Needed: poplabels, output." << std::endl;
    help = true;
  }
  if(options.count("help") || help){
    std::cout << options.help({""}) << std::endl;
    std::cout << "Estimate population size using coalescent rate." << std::endl;
    exit(0);
  }  

  std::cerr << "------------------------------------------------------" << std::endl;
  std::cerr << "Finalizing coalescent rate..." << std::endl;

  ////////// read labels of sequences //////////

  Sample sample;
  sample.Read(options["poplabels"].as<std::string>());
  int N = sample.group_of_haplotype.size();

  /////////////////// INITIALIZE EPOCHES //////////////////////
  
  int num_epochs;
  std::vector<float> epoch;

  FILE* fp = fopen((options["output"].as<std::string>() + ".bin").c_str(),"rb");
  assert(fp != NULL);

  fread(&num_epochs, sizeof(int), 1, fp);
  epoch.resize(num_epochs);
  fread(&epoch[0], sizeof(float), num_epochs, fp);
  std::vector<CollapsedMatrix<float>> coalescent_rate_data(num_epochs);
  for(int e = 0; e < num_epochs; e++){
    coalescent_rate_data[e].ReadFromFile(fp);
  }
  fclose(fp);

  /////////////////// SUMMARIZE //////////////////////

  std::vector<CollapsedMatrix<float>> coalescent_rate(num_epochs);
  for(std::vector<CollapsedMatrix<float>>::iterator it_c = coalescent_rate.begin(); it_c != coalescent_rate.end(); it_c++){
    (*it_c).resize(sample.groups.size(), sample.groups.size());
    std::fill((*it_c).vbegin(), (*it_c).vend(), 0.0);
  }

  for(int i = 0; i < N; i++){
    for(int j = i+1; j < N; j++){

      //choose entry for groups i, j
      int group_i = sample.group_of_haplotype[i];
      int group_j = sample.group_of_haplotype[j];
      //make index of group_i < group_j
      if(group_i > group_j){
        int foo = group_i;
        group_i = group_j;
        group_j = foo;
      }

      std::vector<CollapsedMatrix<float>>::iterator it_coalescent_rate_data = coalescent_rate_data.begin();
      std::vector<CollapsedMatrix<float>>::iterator it_coalescent_rate      = coalescent_rate.begin();

      for(; it_coalescent_rate_data != std::prev(coalescent_rate_data.end(),1);){
        //i < j always holds 
        if((*it_coalescent_rate_data)[i][j] == 0.0){
          (*it_coalescent_rate)[group_i][group_j] += 0.0;
        }else{ 
          (*it_coalescent_rate)[group_i][group_j] += (*it_coalescent_rate_data)[i][j]/(*it_coalescent_rate_data)[j][i];
        }

        it_coalescent_rate++;
        it_coalescent_rate_data++;
      }    

    }
  }

  /////////////////// OUTPUT //////////////////////

  std::ofstream os(options["output"].as<std::string>() + ".coal");
  if(os.fail()){
    std::cerr << "Error while opening file." << std::endl;
    exit(1);
  } 

  for(std::vector<std::string>::iterator it_groups = sample.groups.begin(); it_groups != sample.groups.end(); it_groups++){
    os << *it_groups << " ";
  }
  os << "\n";

  for(int e = 0; e < num_epochs; e++){
    os << epoch[e]  << " "; 
  }
  os << "\n";

  for(int i = 0; i < sample.groups.size(); i++){
    for(int j = 0; j < sample.groups.size(); j++){
      os << i << " " << j << " ";
      for(int e = 0; e < num_epochs; e++){
        if(i == j){
          os << 2.0 * coalescent_rate[e][i][j]/((float) (sample.group_sizes[i] * (sample.group_sizes[j]-1.0))) << " ";
        }else if(i < j){
          os << coalescent_rate[e][i][j]/((float) (sample.group_sizes[i] * sample.group_sizes[j])) << " ";
        }else{
          os << coalescent_rate[e][j][i]/((float) (sample.group_sizes[j] * sample.group_sizes[i])) << " ";
        }
      }
      os << "\n"; 
    }
  }

  os.close();

  /////////////////////////////////////////////
  //Resource Usage

  rusage usage;
  getrusage(RUSAGE_SELF, &usage);

  std::cerr << "CPU Time spent: " << usage.ru_utime.tv_sec << "." << std::setfill('0') << std::setw(6) << usage.ru_utime.tv_usec << "s; Max Memory usage: " << usage.ru_maxrss/1000.0 << "Mb." << std::endl;
  std::cerr << "------------------------------------------------------" << std::endl << std::endl;

  return 0;


}

int FinalizePopulationSizeByHaplotype(cxxopts::Options& options){

  //////////////////////////////////
  //Program options

  bool help = false;
  if(!options.count("output")){
    std::cout << "Not enough arguments supplied." << std::endl;
    std::cout << "Needed: output." << std::endl;
    help = true;
  }
  if(options.count("help") || help){
    std::cout << options.help({""}) << std::endl;
    std::cout << "Estimate population size using coalescent rate." << std::endl;
    exit(0);
  }  

  std::cerr << "------------------------------------------------------" << std::endl;
  std::cerr << "Finalizing coalescent rate..." << std::endl;



  ////////// read labels of sequences //////////

  /////////////////// INITIALIZE //////////////////////
  
  int num_epochs;
  std::vector<float> epoch;

  FILE* fp = fopen((options["output"].as<std::string>() + ".bin").c_str(),"rb");
  assert(fp != NULL);

  fread(&num_epochs, sizeof(int), 1, fp);
  epoch.resize(num_epochs);
  fread(&epoch[0], sizeof(float), num_epochs, fp);
  std::vector<CollapsedMatrix<float>> coalescent_rate_data(num_epochs);
  for(int e = 0; e < num_epochs; e++){
    coalescent_rate_data[e].ReadFromFile(fp);
  }
  fclose(fp);

  int N = coalescent_rate_data[0].size();

  std::vector<CollapsedMatrix<float>> coalescent_rate(num_epochs);
  for(std::vector<CollapsedMatrix<float>>::iterator it_c = coalescent_rate.begin(); it_c != coalescent_rate.end(); it_c++){
    (*it_c).resize(N, N);
    std::fill((*it_c).vbegin(), (*it_c).vend(), 0.0);
  }

  for(int i = 0; i < N; i++){
    for(int j = i+1; j < N; j++){

      std::vector<CollapsedMatrix<float>>::iterator it_coalescent_rate_data = coalescent_rate_data.begin();
      std::vector<CollapsedMatrix<float>>::iterator it_coalescent_rate      = coalescent_rate.begin();

      for(; it_coalescent_rate_data != std::prev(coalescent_rate_data.end(),1);){
        //i < j always holds 
        if((*it_coalescent_rate_data)[i][j] == 0.0){
          (*it_coalescent_rate)[i][j] += 0.0;
        }else{ 
          (*it_coalescent_rate)[i][j] += (*it_coalescent_rate_data)[i][j]/(*it_coalescent_rate_data)[j][i];
        }

        it_coalescent_rate++;
        it_coalescent_rate_data++;
      }    

    }
  }


  std::ofstream os(options["output"].as<std::string>() + ".coal");
  if(os.fail()){
    std::cerr << "Error while opening file." << std::endl;
    exit(1);
  } 

  for(int i = 0; i < N; i++){
    os << i << " ";
  }
  os << "\n";

  for(int e = 0; e < num_epochs; e++){
    os << epoch[e]  << " "; 
  }
  os << "\n";

  for(int i = 0; i < N; i++){
    for(int j = i+1; j < N; j++){
      os << i << " " << j << " ";
      for(int e = 0; e < num_epochs; e++){
        os << coalescent_rate[e][i][j] << " ";
      }
      os << "\n"; 
    }
  }

  os.close();

  /////////////////////////////////////////////
  //Resource Usage

  rusage usage;
  getrusage(RUSAGE_SELF, &usage);

  std::cerr << "CPU Time spent: " << usage.ru_utime.tv_sec << "." << std::setfill('0') << std::setw(6);
#ifdef __APPLE__
  std::cerr << usage.ru_utime.tv_usec << "s; Max Memory usage: " << usage.ru_maxrss/1000000.0 << "Mb." << std::endl;
#else
  std::cerr << usage.ru_utime.tv_usec << "s; Max Memory usage: " << usage.ru_maxrss/1000.0 << "Mb." << std::endl;
#endif
  std::cerr << "---------------------------------------------------------" << std::endl << std::endl;

  return 0;


}
