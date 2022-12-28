#include <iostream>
#include <fstream>
#include <utility>
#include <string>
#include <limits>
#include <algorithm>
#include <sys/time.h>
#include <sys/resource.h>

#include "filesystem.hpp"
#include "gzstream.hpp"
#include "collapsed_matrix.hpp"
#include "anc.hpp"
#include "cxxopts.hpp"


void
MakeAncesTreeFile(cxxopts::Options& options, Mutations& mut, std::vector<int>& include_snp){

  //////////////////////////////////
  //Program options

  bool help = false;
  if(!options.count("pop_of_interest") || !options.count("poplabels") || !options.count("anc") || !options.count("mut") || !options.count("output")){
    std::cout << "Not enough arguments supplied." << std::endl;
    std::cout << "Needed: pop_of_interest, poplabels, anc, mut, output." << std::endl;
    help = true;
  }
  if(options.count("help") || help){
    std::cout << options.help({""}) << std::endl;
    std::cout << "Estimate population size using coalescent rate." << std::endl;
    exit(0);
  }  

  AncMutIterators ancmut(options["anc"].as<std::string>(), options["mut"].as<std::string>());
  int N = ancmut.NumTips();
  int L = ancmut.NumSnps();
  int num_trees = ancmut.NumTrees();
  MarginalTree mtr;
  Muts::iterator it_mut;
  float num_bases_tree_persists = 0.0;

  int i;
  std::string line, read;

  std::cerr << "---------------------------------------------------------" << std::endl;
  std::cerr << "Calculating anc file for " << options["anc"].as<std::string>() << " and subpopulation(s) ";

  Sample sample;
  sample.Read(options["poplabels"].as<std::string>());
  std::string label;
  if(!options.count("pop_of_interest")){
    label = sample.AssignPopOfInterest("All");
  }else{
    label = sample.AssignPopOfInterest(options["pop_of_interest"].as<std::string>());
  }

  if(label.compare("All")){
    for(std::vector<int>::iterator it_group_of_interest = sample.group_of_interest.begin(); it_group_of_interest != sample.group_of_interest.end(); it_group_of_interest++ ){
      std::cerr << sample.groups[*it_group_of_interest] << " ";
    }
    std::cerr << std::endl;
  }else{
    std::cerr << "All" << std::endl;
  }

  Data data(sample.group_of_interest_size, 1);
  int N_total = 2*sample.group_of_interest_size-1;
  int root    = N_total - 1;

  //////////////////////// Extract Subtrees //////////////

  //Associate branches
  AncesTreeBuilder ancbuilder(data);
  ancbuilder.PreCalcPotentialBranches(); // precalculating the number of decendants a branch needs to be equivalent (narrowing search space)
  std::vector<int> check_if_trees_are_equivalent;
  std::vector<int> convert_index, number_in_subpop;
  std::vector<float> coordinates(N_total, 0.0);

  Tree subtree;
  std::vector<AncesTree> v_anc(1);
  v_anc[0].seq.resize(num_trees);
  CorrTrees::iterator it_subseq = v_anc[0].seq.begin();

  if(ancmut.sample_ages.size() > 0){
    v_anc[0].sample_ages.resize(data.N);
    int hap = 0, hap_sub = 0;
    for(std::vector<int>::iterator it_group_of_haplotype = sample.group_of_haplotype.begin(); it_group_of_haplotype != sample.group_of_haplotype.end(); it_group_of_haplotype++){
      bool exists = false;
      for(std::vector<int>::iterator it_group_of_interest = sample.group_of_interest.begin(); it_group_of_interest != sample.group_of_interest.end(); it_group_of_interest++){
        if(*it_group_of_haplotype == *it_group_of_interest){
          exists = true;
          break;
        }
      }
      if(exists){
        v_anc[0].sample_ages[hap_sub] = ancmut.sample_ages[hap];
        hap_sub++;
      }
      hap++;
    }
    if(hap_sub < data.N){
      v_anc[0].sample_ages.resize(0);
    }
  }else{
    v_anc[0].sample_ages.clear();
  }
  for(it_subseq = v_anc[0].seq.begin(); it_subseq != v_anc[0].seq.end(); it_subseq++){
    (*it_subseq).tree.sample_ages = &v_anc[0].sample_ages;
  }

  it_subseq = v_anc[0].seq.begin();


  int count_tree = 0, count_included_tree = 0;
  int snp = 0;
  int branch;
  float freq;
  int num_snps_mapped_onto_tree = 0;
  num_bases_tree_persists = ancmut.NextTree(mtr, it_mut);
  while(num_bases_tree_persists >= 0.0){

    //read tree
    mtr.tree.GetSubTree(sample, (*it_subseq).tree, convert_index, number_in_subpop);
    (*it_subseq).pos = include_snp.size();
    (*it_subseq).tree.GetCoordinates(coordinates);

    //set branch lengths lifespan to lifespan of tree (will be extended later)
    for(std::vector<Node>::iterator it_node = (*it_subseq).tree.nodes.begin(); it_node != (*it_subseq).tree.nodes.end(); it_node++){
      //(*it_node).SNP_begin = (*it_subseq).pos;
      (*it_node).SNP_begin = include_snp.size();
      (*it_node).num_events = 0.0;
    }
    //previous tree extends to one position before current tree
    if(it_subseq != v_anc[0].seq.begin()){
      for(std::vector<Node>::iterator it_node = (*std::prev(it_subseq,1)).tree.nodes.begin(); it_node != (*std::prev(it_subseq,1)).tree.nodes.end(); it_node++){
        (*it_node).SNP_end = include_snp.size() - 1;
      }
    }

    //Map mutations to subtree
    num_snps_mapped_onto_tree = 0;

    while(mut.info[snp].tree < count_tree){
      snp++;
      if(snp == (int)mut.info.size()) break; 
    }
    if(snp == (int)mut.info.size()) break; 
    assert(mut.info[snp].tree == count_tree);

    if(mut.info[snp].freq.size() == sample.groups.size()){

      while(mut.info[snp].tree == count_tree){

        freq = 0.0; 
        for(std::vector<int>::iterator it_group_of_interest = sample.group_of_interest.begin(); it_group_of_interest != sample.group_of_interest.end(); it_group_of_interest++){
          freq += mut.info[snp].freq[*it_group_of_interest];
          if(freq > 0.0) break;
        }

        if(freq > 0.0){

          if(mut.info[snp].branch.size() == 1){
            branch = convert_index[*mut.info[snp].branch.begin()];
            if(branch != -1 && branch != root && number_in_subpop[*mut.info[snp].branch.begin()] > 0){
              num_snps_mapped_onto_tree++;
              include_snp.push_back(snp);
              mut.info[snp].age_begin = coordinates[branch];
              mut.info[snp].age_end   = coordinates[(*(*it_subseq).tree.nodes[branch].parent).label];
              assert(count_included_tree >= 0);
              mut.info[snp].tree      = count_included_tree;
            }
          }
          for(std::vector<int>::iterator it_branch = mut.info[snp].branch.begin(); it_branch != mut.info[snp].branch.end(); it_branch++){
            branch = convert_index[*it_branch];
            if(branch != -1){
              (*it_subseq).tree.nodes[branch].num_events += 1.0/((float) mut.info[snp].branch.size());
              *it_branch = branch;
            }
          }

        }
        snp++;
        if(snp == (int)mut.info.size()) break; 

      }

    }else{
    
      while(mut.info[snp].tree == count_tree){

          if(mut.info[snp].branch.size() == 1){
            branch = convert_index[*mut.info[snp].branch.begin()];
            if(branch != -1 && branch != root && number_in_subpop[*mut.info[snp].branch.begin()] > 0){
              num_snps_mapped_onto_tree++;
              include_snp.push_back(snp);
              mut.info[snp].age_begin = coordinates[branch];
              mut.info[snp].age_end   = coordinates[(*(*it_subseq).tree.nodes[branch].parent).label];
              assert(count_included_tree >= 0);
              mut.info[snp].tree      = count_included_tree;
            }
          }
          for(std::vector<int>::iterator it_branch = mut.info[snp].branch.begin(); it_branch != mut.info[snp].branch.end(); it_branch++){
            branch = convert_index[*it_branch];
            if(branch != -1){
              (*it_subseq).tree.nodes[branch].num_events += 1.0/((float) mut.info[snp].branch.size());
              *it_branch = branch;
            }
          }

        snp++;
        if(snp == (int)mut.info.size()) break; 
      
      }

    }


    //increment except if no mutations mapped onto it
    if(num_snps_mapped_onto_tree != 0){
      count_included_tree++;
      it_subseq++;
    }
    count_tree++;
    num_bases_tree_persists = ancmut.NextTree(mtr, it_mut);
    if(snp == (int)mut.info.size()) break; 
  }

  it_subseq--;
  for(std::vector<Node>::iterator it_node = (*it_subseq).tree.nodes.begin(); it_node != (*it_subseq).tree.nodes.end(); it_node++){
    (*it_node).SNP_end = include_snp.size() - 1;
  }
  v_anc[0].seq.resize(count_included_tree);

  ////////////////////////// Associate branches ////////////////

  CorrTrees::iterator it_seq_prev;
  CorrTrees::iterator it_seq; 

  /////
  // Find equivalent branches
  it_seq_prev = v_anc[0].seq.begin();
  it_seq      = std::next(it_seq_prev,1); 

  std::vector<std::vector<int>> equivalent_branches;
  std::vector<std::vector<int>>::iterator it_equivalent_branches;

  for(; it_seq != v_anc[0].seq.end();){
    equivalent_branches.emplace_back();
    it_equivalent_branches = std::prev(equivalent_branches.end(),1);
    ancbuilder.BranchAssociation((*it_seq_prev).tree, (*it_seq).tree, *it_equivalent_branches); //O(N^2) 
    it_seq++;
    it_seq_prev++;
  }  //Write equivalent_branches to file

  filesys f;
  
  std::string dirname         = options["output"].as<std::string>() + "/";
  f.MakeDir((dirname).c_str());

  std::string output_filename = dirname + "equivalent_branches_0.bin";
  FILE* pf = fopen(output_filename.c_str(), "wb");
  assert(pf != NULL);

  int size = equivalent_branches.size();
  fwrite(&size, sizeof(int), 1, pf);

  for(int i = 0; i < size; i++){
    fwrite(&equivalent_branches[i][0], sizeof(int), N_total, pf);
  }
  fclose(pf); 

  //Associate branches of adjacent trees
  //Data data(((*v_anc[0].seq.begin()).tree.nodes.size() + 1)/2, 1);
  //AncesTreeBuilder ancbuilder(data);
  ancbuilder.AssociateTrees(v_anc, dirname);
  v_anc[0].Dump(options["output"].as<std::string>() + ".anc");
 
  std::remove((dirname + "equivalent_branches_0.bin").c_str());
  f.RmDir( dirname.c_str() ); 

}

void
CreateAncesTreeFileForSubpopulation(cxxopts::Options& options){

  //////////////////////////////////
  //Program options

  bool help = false;
  if(!options.count("pop_of_interest") || !options.count("poplabels") || !options.count("anc") || !options.count("mut") || !options.count("output")){
    std::cout << "Not enough arguments supplied." << std::endl;
    std::cout << "Needed: pop_of_interest, poplabels, anc, mut, output." << std::endl;
    help = true;
  }
  if(options.count("help") || help){
    std::cout << options.help({""}) << std::endl;
    std::cout << "Estimate population size using coalescent rate." << std::endl;
    exit(0);
  } 

  std::vector<int> include_snp;
  //std::vector<AncesTree> v_anc(1);
  Mutations mut;
  mut.Read(options["mut"].as<std::string>());
  if(mut.info[0].freq.size() == 0){
    std::cerr << "We recommend to first add population annotation to .mut using RelateFileFormats --mode GenerateSNPAnnotations" << std::endl;
    //exit(1);
  }
  MakeAncesTreeFile(options, mut, include_snp);
  ////////////////////////////////////////////////////////////////////////////////////////////////
  //Propagate mutations

  //Associate branches of adjacent trees
  //Data data(((*v_anc[0].seq.begin()).tree.nodes.size() + 1)/2, 1);
  //AncesTreeBuilder ancbuilder(data);
  //ancbuilder.AssociateTrees(v_anc, "./");
  //v_anc[0].Dump(options["output"].as<std::string>() + ".anc");

  //////////////////////// Extract Subtrees //////////////

  Sample sample;
  sample.Read(options["poplabels"].as<std::string>());
  std::string label;
  if(!options.count("pop_of_interest")){
    label = sample.AssignPopOfInterest("All");
  }else{
    label = sample.AssignPopOfInterest(options["pop_of_interest"].as<std::string>());
  }

  std::string line;
  char id[512], pop[512];
  igzstream is_pops(options["poplabels"].as<std::string>());
  std::ofstream os_pops(options["output"].as<std::string>() + ".poplabels");
  getline(is_pops, line);
  os_pops << line << "\n";
  while(getline(is_pops, line)){
    sscanf(line.c_str(), "%s %s", id, pop);
    for(std::vector<int>::iterator it_goi = sample.group_of_interest.begin(); it_goi != sample.group_of_interest.end(); it_goi++){
      if( strcmp(pop, sample.groups[*it_goi].c_str()) == 0 ){
        os_pops << line << "\n"; 
        break;   
      }
    }  
  }
  is_pops.close();
  os_pops.close();

  Mutations mut_subset;
  mut_subset.info.resize(include_snp.size());
  mut_subset.header = "snp;pos_of_snp;dist;rs-id;tree_index;branch_indices;is_not_mapping;is_flipped;age_begin;age_end;ancestral_allele/alternative_allele;upstream_allele;downstream_allele;";
  for(std::vector<int>::iterator it_goi = sample.group_of_interest.begin(); it_goi != sample.group_of_interest.end(); it_goi++){
    mut_subset.header += sample.groups[*it_goi] + ";";
  } 

  int snp = 0;
  std::vector<int>::iterator it_snp_next = std::next(include_snp.begin(),1);
  for(std::vector<int>::iterator it_snp = include_snp.begin(); it_snp != include_snp.end(); it_snp++){
    mut_subset.info[snp] = mut.info[*it_snp];

    //sum dist to next snp
    if(it_snp_next != include_snp.end()){
      for(int tmp_snp = *it_snp + 1; tmp_snp < *it_snp_next; tmp_snp++){
        mut_subset.info[snp].dist += mut.info[tmp_snp].dist;
      }
      it_snp_next++;
    }else{
      for(int tmp_snp = *it_snp + 1; tmp_snp < mut.info.size(); tmp_snp++){
        mut_subset.info[snp].dist += mut.info[tmp_snp].dist;
      }
    }

    if(mut.info[snp].freq.size() == sample.groups.size()){
      //update freq
      mut_subset.info[snp].freq.resize(sample.group_of_interest.size());
      int k = 0;
      for(std::vector<int>::iterator it_group_of_interest = sample.group_of_interest.begin(); it_group_of_interest != sample.group_of_interest.end(); it_group_of_interest++){
        mut_subset.info[snp].freq[k] = mut.info[*it_snp].freq[*it_group_of_interest];
        k++;
      }
    }
    snp++;
  }
  mut_subset.Dump(options["output"].as<std::string>() + ".mut");
  std::remove("equivalent_branches_0.bin");

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

}
