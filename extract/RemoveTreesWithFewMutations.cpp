#include <iostream>
#include <fstream>
#include <utility>
#include <string>
#include <limits>
#include <algorithm>
#include <sys/time.h>
#include <sys/resource.h>
#include <cxxopts.hpp>

#include "collapsed_matrix.hpp"
#include "anc.hpp"
#include "mutations.hpp"
#include "usage.hpp"

void
GetDistFromMut(cxxopts::ParseResult& result, const std::string& help_text){

  //////////////////////////////////
  //Program options

  bool help = false;
  if(!result.count("mut") || !result.count("output")){
    std::cout << "Not enough arguments supplied." << std::endl;
    std::cout << "Needed: mut, output." << std::endl;
    help = true;
  }
  if(result.count("help") || help){
    std::cout << help_text << std::endl;
    std::cout << "Extract dist file from mut." << std::endl;
    exit(0);
  }  

  std::cerr << "---------------------------------------------------------" << std::endl;
  std::cerr << "Extracting dist file from " << result["mut"].as<std::string>() << " ... " << std::endl;

  Mutations mut;
  mut.Read(result["mut"].as<std::string>());

 
  FILE* fp_dist = fopen((result["output"].as<std::string>() + ".dist").c_str(), "w");
  fprintf(fp_dist, "#pos dist\n");
  for(std::vector<SNPInfo>::iterator it_mut = mut.info.begin(); it_mut != mut.info.end();){
    fprintf(fp_dist, "%d %d\n", (*it_mut).pos, (*it_mut).dist);
    it_mut++;
  }
  fclose(fp_dist);

  ResourceUsage();
}

void
RemoveTreesWithFewMutations(cxxopts::ParseResult& result, const std::string& help_text){

  //////////////////////////////////
  //Program options
  
  bool help = false;
  if(!result.count("threshold") || !result.count("anc") || !result.count("mut") || !result.count("output")){
    std::cout << "Not enough arguments supplied." << std::endl;
    std::cout << "Needed: threshold, anc, mut, output." << std::endl;
    help = true;
  }
  if(result.count("help") || help){
    std::cout << help_text << std::endl;
    std::cout << "Estimate population size using coalescent rate." << std::endl;
    exit(0);
  }  

  std::cerr << "---------------------------------------------------------" << std::endl;
  std::cerr << "Removing trees with few mutations from " << result["mut"].as<std::string>() << " ... " << std::endl; 
 
  AncesTree anc;
  AncMutIterators ancmut(result["anc"].as<std::string>(), result["mut"].as<std::string>());
  int N = ancmut.NumTips();
  int L = ancmut.NumSnps();
  int num_trees = ancmut.NumTrees();
  MarginalTree mtr;
  Muts::iterator it_mut;
  float num_bases_tree_persists = 0.0;
  anc.N = N;
  anc.L = num_trees;
  anc.sample_ages = ancmut.sample_ages;
  Data data(N,L);

  Mutations mut;
  mut.Read(result["mut"].as<std::string>());

  Mutations mut_subset;
  mut_subset.header = mut.header;

  int num_mutations_threshold, num_mutations;
  double threshold = std::max(0.0f, std::min(1.0f, result["threshold"].as<float>()));
  std::vector<int> num_muts(ancmut.NumTrees(), 0), num_muts_sorted;
  std::vector<int>::iterator it_num_muts = num_muts.begin();

  std::vector<Node>::iterator it_node;
  num_bases_tree_persists = ancmut.NextTree(mtr, it_mut);
  while(num_bases_tree_persists >= 0.0){
    num_mutations = 0;
    it_node = mtr.tree.nodes.begin();
    for(; it_node != mtr.tree.nodes.end(); it_node++){
      num_mutations += (*it_node).num_events;
    }
    *it_num_muts = num_mutations;
    num_bases_tree_persists = ancmut.NextTree(mtr, it_mut);
    it_num_muts++;
  }

  num_muts_sorted = num_muts;
  std::sort(num_muts_sorted.begin(), num_muts_sorted.end());
  num_mutations_threshold = num_muts_sorted[(int)(threshold*num_muts.size())];
  ancmut.OpenFiles(result["anc"].as<std::string>(), result["mut"].as<std::string>());

  //count the number of mutatinos on each tree.
  num_bases_tree_persists = ancmut.NextTree(mtr, it_mut);
  int num_tree_before = 0, num_tree_after = 0, snp = 0;
  while(num_bases_tree_persists >= 0.0){

    if(num_muts[num_tree_before] >= num_mutations_threshold){ 
      //include SNPs to mut_subset
      while(mut.info[snp].tree < num_tree_before){
        snp++;
        if(snp == (int)mut.info.size()) break;
      }
      if(snp == (int)mut.info.size()) break;
      if(mut.info[snp].tree != num_tree_before) std::cerr << mut.info[snp].tree << " " << num_tree_before << std::endl;
      assert(mut.info[snp].tree == num_tree_before);
      while(mut.info[snp].tree == num_tree_before){
        mut.info[snp].tree = num_tree_after;
        mut_subset.info.push_back(mut.info[snp]);
        snp++;
        if(snp == (int)mut.info.size()) break;
      }
      num_tree_after++;
      anc.seq.push_back(mtr);
      if(snp == (int)mut.info.size()) break;
    }
    num_bases_tree_persists = ancmut.NextTree(mtr, it_mut);
    num_tree_before++;
  }

  if(anc.seq.size() == 0){
    std::cerr << "Error: Threshold value is too large. Please try a lower value." << std::endl;
    exit(1);
  }

  // dump anc and mut
  anc.Dump(result["output"].as<std::string>() + ".anc");
  mut_subset.Dump(result["output"].as<std::string>() + ".mut");

  ResourceUsage();
}
