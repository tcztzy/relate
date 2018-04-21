#include <iostream>
#include <fstream>
#include <utility>
#include <string>
#include <sys/time.h>
#include <sys/resource.h>

#include "gzstream.hpp"
#include "collapsed_matrix.hpp"
#include "anc.hpp"
#include "anc_builder.hpp"
#include "tree_comparer.hpp"
#include "cxxopts.hpp"

void 
logFactorial(std::vector<float>& logF, int N){

  logF.resize(N+1); 
  std::vector<float>::iterator it_logF = logF.begin();
  *it_logF = 0;
  it_logF++;
  std::vector<float>::iterator it_logF_prev = logF.begin();
  for(int k = 1; k < N+1; k++){
    *it_logF = *it_logF_prev + std::log(k);
    it_logF++;
    it_logF_prev++;
  }

}

/*
   float
   log_pvalue_old(int k_now, int k_then, int z, std::vector<float>& logF){

   if(k_then == -1) return 1;

   assert(k_then >= 2);
   assert(k_then < k_now);
   assert(z < k_now);

   float log_pvalue, tmp_pvalue;

   if(0){
   int z_tmp = 2;
   log_pvalue  = logF[k_now - z_tmp - 1] - logF[k_now - z_tmp - k_then + 1] - logF[k_then - 2];
   log_pvalue += log(z_tmp - 1);
   log_pvalue -= logF[k_now - 1] - logF[k_now - k_then - 1] - logF[k_then];

   for(int i = z_tmp + 1; i <= k_now - k_then + 1; i++){
   tmp_pvalue  = logF[k_now - i - 1] - logF[k_now - i - k_then + 1] - logF[k_then - 2];
   tmp_pvalue += log(i-1);
   tmp_pvalue -= logF[k_now - 1] - logF[k_now - k_then - 1] - logF[k_then];

   if(tmp_pvalue - log_pvalue <= 0.0){
   log_pvalue  = log( 1.0 + exp(tmp_pvalue - log_pvalue) ) + log_pvalue;
   }else{
   log_pvalue  = log( 1.0 + exp(log_pvalue - tmp_pvalue) ) + tmp_pvalue;
   }
   }
   assert(log_pvalue < 1e-2);
   }

   log_pvalue  = logF[k_now - z - 1] - logF[k_now - z - k_then + 1] - logF[k_then - 2];
   log_pvalue += log(z-1);
   log_pvalue -= logF[k_now - 1] - logF[k_now - k_then - 1] - logF[k_then];

   for(int i = z+1; i <= k_now - k_then + 1; i++){
   tmp_pvalue  = logF[k_now - i - 1] - logF[k_now - i - k_then + 1] - logF[k_then - 2];
   tmp_pvalue += log(i-1);
   tmp_pvalue -= logF[k_now - 1] - logF[k_now - k_then - 1] - logF[k_then];

   if(tmp_pvalue - log_pvalue <= 0.0){
   log_pvalue  = log( 1.0 + exp(tmp_pvalue - log_pvalue) ) + log_pvalue;
   }else{
   log_pvalue  = log( 1.0 + exp(log_pvalue - tmp_pvalue) ) + tmp_pvalue;
   }
   }

   if(std::fabs(log_pvalue) < 1e-2) log_pvalue = 0.0;


   if(log_pvalue > 0.0) std::cerr << log_pvalue << " " << k_now << " " << k_then << " " << z << std::endl;

   assert(log_pvalue <= 0.0);

   return log_pvalue/log(10);

   }
   */

float log_10 = std::log(10);

float 
log_pvalue(int k, float fk, int N, float fN, std::vector<float>& logF){

  float logp = 0.0;
  float px = 0.0;

  if(fk < 2) return 1.0;
  if(k == -1) return 1.0;

  assert(fN < N);
  assert(fk < k);
  assert(fN > 0);

  //Calculate P(fN | N, k, fk)
  px  = logF[N-fN-1] - logF[k-fk-1] - logF[N-k+fk-fN];
  px += logF[fN-1]   - logF[fk-1]   - logF[fN-fk];
  px -= logF[N-1]    - logF[k-1]    - logF[N-k];

  //std::cerr << px << std::endl;

  //Sum over fN <= f <= N-k+fk 
  logp = px;
  float x = fN - fk;
  int y = N - k;
  int c = N - 1;
  int var;
  while(x < N-k){
    var  = fk + x;
    px  += std::log( (y-x)/(x+1.0) * var/((float) (c - var)) );
    //std::cerr << px << " " << logp << " " << c << " " << var << " " << x << std::endl;
    //logp = std::log( 1.0 + exp( logp - px ) ) + px;
    logp = std::log( 1.0 + exp( px - logp ) ) + logp;
    x++;
  }

  //std::cerr << logp << std::endl;
  if(logp > 0.0) logp = 0.0;

  logp /= log_10;

  return(logp);

}

struct Qual{

  std::string id;
  int pos;
  int num_decendants;
  int freq;
  float frac_not_mapping;

};

void 
Selection(cxxopts::Options& options){

  //////////////////////////////////
  //Program options
  bool help = false;
  if(!options.count("input") || !options.count("output")){
    std::cout << "Not enough arguments supplied." << std::endl;
    std::cout << "Needed: input, output." << std::endl;
    help = true;
  }
  if(options.count("help") || help){
    std::cout << options.help({""}) << std::endl;
    std::cout << "Calculating pvalue for selection using output of mode Frequency." << std::endl;
    exit(0);
  }  

  std::cerr << "------------------------------------------------------" << std::endl;
  std::cerr << "Calculating evidence of selection for " + options["input"].as<std::string>() + ".\n";

  std::string line_freq, line_lin, read;

  //read in .freq and .lin files
  //obtain N, fN, k, fk for every SNP.
  //Calculate pvalue

  igzstream is_freq(options["input"].as<std::string>() + ".freq");
  if(is_freq.fail()) is_freq.open(options["input"].as<std::string>() + ".freq.gz");
  if(is_freq.fail()){
    std::cerr << "Error while opening file " << options["input"].as<std::string>() + ".freq(.gz)" << std::endl;
    exit(1);
  }
  igzstream is_lin(options["input"].as<std::string>() + ".lin");
  if(is_lin.fail()) is_lin.open(options["input"].as<std::string>() + ".freq.gz");
  if(is_lin.fail()){
    std::cerr << "Error while opening file " << options["input"].as<std::string>() + ".lin(.gz)" << std::endl;
    exit(1);
  }
  std::ofstream os(options["output"].as<std::string>() + ".sele");
  if(os.fail()){
    std::cerr << "Error while opening file " << options["output"].as<std::string>() + ".sele" << std::endl;
    exit(1);
  }

  //skip headers
  getline(is_freq, line_freq);
  getline(is_lin, line_lin);

  os << line_lin << "\n";

  //precalculate logF, where logF[k] = log(k!)
  std::vector<float> logF;

  //read line by line
  int N, k;
  float fN, fk;
  std::vector<float> num_lin, num_freq;

  while(getline(is_freq, line_freq)){
    getline(is_lin, line_lin);

    //get fN, N, k, fk from the line
    std::stringstream s_freq(line_freq);
    std::stringstream s_lin(line_lin);

    s_freq >> read;
    os << read << " ";
    s_freq >> read;
    os << read << " ";
    s_lin >> read;
    s_lin >> read;

    //read in k from s_lin and fk from s_freq
    if(logF.size() == 0){

      float foo;
      while(s_lin >> foo) num_lin.push_back(foo);
      num_freq.resize(num_lin.size()-1);
      for(int i = 0; i < num_freq.size(); i++){
        s_freq >> num_freq[i];
      }
      N = (int)num_lin[num_lin.size() - 2];
      logFactorial(logF, N);

    }else{

      for(int i = 0; i < num_lin.size(); i++){
        s_lin >> num_lin[i];
      }
      for(int i = 0; i < num_freq.size(); i++){
        s_freq >> num_freq[i];
      }

    }

    fN = num_freq[num_freq.size() - 1]; //frequency when N lineages are remaining

    for(int i = 0; i < num_freq.size(); i++){
      os << log_pvalue(num_lin[i], num_freq[i], N, fN, logF) << " ";
    }

    //pvalue from when freq = 2
    if(fN >= 2){
      os << log_pvalue(num_lin[num_lin.size() - 1], 2.0, N, fN, logF) << "\n";
    }else{
      os << "1\n";
    }

  }

  is_freq.close();
  is_lin.close();




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

void 
Frequency(cxxopts::Options& options){

  //////////////////////////////////
  //Program options
  bool help = false;
  if(!options.count("input") || !options.count("output")){
    std::cout << "Not enough arguments supplied." << std::endl;
    std::cout << "Needed: input, output. Optional: years_per_gen, first_snp, last_snp, quality." << std::endl;
    help = true;
  }
  if(options.count("help") || help){
    std::cout << options.help({""}) << std::endl;
    std::cout << "..." << std::endl;
    exit(0);
  }  

  std::cerr << "------------------------------------------------------" << std::endl;
  std::cerr << "Calculating frequency through time for " + options["input"].as<std::string>() + ".\n";

  std::string line, read;

  ////////// PARSE DATA //////////

  int N;
  igzstream is_N;
  is_N.open(options["input"].as<std::string>() + ".anc");
  if(is_N.fail()) is_N.open(options["input"].as<std::string>() + ".anc.gz");
  if(is_N.fail()){
    std::cerr << "Error while opening .anc file." << std::endl;
    exit(1);
  } 
  is_N.ignore(256, ' ');
  is_N >> N;
  is_N.close();

  int L = 0;
  igzstream is_L;
  is_L.open(options["input"].as<std::string>() + ".mut");
  if(is_L.fail()) is_L.open(options["input"].as<std::string>() + ".mut.gz"); 
  if(is_L.fail()){
    std::cerr << "Error while opening .mut file." << std::endl;
    exit(1);
  } 
  
  std::string unused;
  std::getline(is_L, unused); 
  while ( std::getline(is_L, unused) ){
    ++L;
  }
 
  Data data(N,L);
  int N_total = 2*data.N-1;

  std::vector<float> logF;
  logFactorial(logF, data.N);

  int first_snp, last_snp;
  if(!options.count("first_snp")){
    first_snp = 0;
  }else{
    first_snp = options["first_snp"].as<int>();
  }
  if(!options.count("last_snp")){
    last_snp = data.L-1;
  }else{
    last_snp = options["last_snp"].as<int>();
  }

  ///////// EPOCHES /////////
  float years_per_gen = 28.0;
  if(options.count("years_per_gen")){
    years_per_gen = options["years_per_gen"].as<float>();
  }

  int num_epochs = 30;
  if(options.count("num_bins") > 0){
    num_epochs = options["num_bins"].as<int>();
  }
  num_epochs++;
  std::vector<float> epoch(num_epochs);
  epoch[0] = 0.0;
  epoch[1] = 1e3/years_per_gen;
  float log_10 = std::log(10);
  for(int e = 2; e < num_epochs-1; e++){
    epoch[e] = std::exp( log_10 * ( 3.0 + 4.0 * (e-1.0)/(num_epochs-3.0) ))/years_per_gen; 
  }
  epoch[num_epochs-1] = 5e7;

  //read mutations file
  Mutations mutations(data);
  mutations.Read(options["input"].as<std::string>() + ".mut");

  //read in quality file if specified
  std::vector<Qual> quality;
  float quality_threshold;
  if(options.count("quality")){
    igzstream is_qual(options["quality"].as<std::string>());
    if(is_qual.fail()){
      std::cerr << "Failed to open file " << options["quality"].as<std::string>() << "." << std::endl;
      exit(1);
    }
    int num_lines = 0;
    getline(is_qual, line);
    while ( std::getline(is_qual, line) ){
      ++num_lines;
    }
    is_qual.close();

    is_qual.open(options["quality"].as<std::string>());
    getline(is_qual, line);
    quality.resize(num_lines);
    std::vector<Qual>::iterator it_qual = quality.begin();
    std::vector<float> frac_not_mapping(quality.size());
    std::vector<float>::iterator it_frac_not_mapping = frac_not_mapping.begin();
    char buffer[80];
    while(getline(is_qual, line)){
      sscanf(line.c_str(), "%s %d %d %d %f\n", buffer, &(*it_qual).pos, &(*it_qual).num_decendants, &(*it_qual).freq, &(*it_qual).frac_not_mapping);
      (*it_qual).id = buffer;
      *it_frac_not_mapping = (*it_qual).frac_not_mapping;
      it_qual++;
      it_frac_not_mapping++;
    }
    is_qual.close();
    std::sort(frac_not_mapping.begin(), frac_not_mapping.end());
    int index_quantile = (int) (0.95 * quality.size());
    quality_threshold = frac_not_mapping[index_quantile];
  }

  ////////////////////
  //for a mutation, record how its frequency is changing

  //1. Get coordinates vector and sort. Use this to determine the number of lineages.
  //2. From branch on which mutation sits, record time of all coalescent events below.
  //3. Count.

  MarginalTree mtr;
  std::vector<float> coordinates_tree(N_total), coordinates_tree_unsrt(N_total), coordinates_mutation(N_total);
  std::vector<int> convert_index;
  int root = N_total-1;
  int i = 0;
  int snp_of_next_tree;

  igzstream is_anc(options["input"].as<std::string>() + ".anc");
  if(is_anc.fail()) is_anc.open(options["input"].as<std::string>() + ".anc.gz");
  if(is_anc.fail()){
    std::cerr << "Error while opening .anc file." << std::endl;
    exit(1);
  }
  std::ofstream os_freq(options["output"].as<std::string>() + ".freq");
  if(os_freq.fail()){
    std::cerr << "Error while opening file." << std::endl;
    exit(1);
  }
  std::ofstream os_lin(options["output"].as<std::string>() + ".lin");
  if(os_lin.fail()){
    std::cerr << "Error while opening file." << std::endl;
    exit(1);
  }

  int count_tree = 0;

  os_freq << "pos rs_id ";
  for(int ep = num_epochs-1; ep >= 0; ep--){
    os_freq << std::to_string(epoch[ep]) << " ";
  }
  os_freq << "TreeFreq DataFreq\n";

  os_lin << "pos rs_id ";
  for(int ep = num_epochs-1; ep >= 0; ep--){
    os_lin << std::to_string(epoch[ep]) << " ";
  }
  os_lin << "k_when_mutation_has_freq2\n";


  getline(is_anc,line);
  getline(is_anc,line);
  getline(is_anc,line);

  //read tree
  mtr.Read(line, N);
  mtr.tree.GetCoordinates(coordinates_tree);
  coordinates_tree_unsrt = coordinates_tree;
  std::sort(coordinates_tree.begin(), coordinates_tree.end());

  SNPInfo snp_info;
  int freq;
  float rec;
  bool passing_quality_test = true;
  std::vector<Qual>::iterator it_qual = quality.begin();
  for(int snp = first_snp; snp <= last_snp; snp++){

    snp_info = mutations.info[snp];

    int freq = 0;
    for(std::vector<int>::iterator it_freq = snp_info.freq.begin(); it_freq != snp_info.freq.end(); it_freq++){
      freq += *it_freq;
      if(freq > 2) break;
    }

    if(options.count("quality")){

      while((*it_qual).pos < snp_info.pos) it_qual++;
      if((*it_qual).pos != snp_info.pos){
        std::cerr << (*it_qual).pos << " " << snp_info.pos << std::endl;
        std::cerr << "Quality assessment file not sorted by pos or doesn't contain all SNPs." << std::endl;
        exit(1);
      }
      if((*it_qual).frac_not_mapping >= quality_threshold){
        passing_quality_test = false;
      }else{
        passing_quality_test = true;
      }

    }

    if(snp_info.branch.size() == 1 && freq > 2 && !snp_info.flipped && passing_quality_test){

      if(count_tree < snp_info.tree){
        while(count_tree < snp_info.tree){
          if(!getline(is_anc,line)){
            break; 
          };
          count_tree++;
        } 
        assert(count_tree == snp_info.tree);

        //read tree
        mtr.Read(line, N);
        mtr.tree.GetCoordinates(coordinates_tree);
        coordinates_tree_unsrt = coordinates_tree;
        std::sort(coordinates_tree.begin(), coordinates_tree.end());
      }

      if(snp_info.age_begin <= coordinates_tree[root]){

        int b = *snp_info.branch.begin();

        if(b != -1 && b != root){

          os_freq << snp_info.pos << " " << snp_info.rs_id << " ";
          os_lin  << snp_info.pos << " " << snp_info.rs_id << " ";

          std::fill(coordinates_mutation.begin(), coordinates_mutation.end(), 0.0);
          mtr.tree.GetCoordinates(b, coordinates_mutation);
          coordinates_mutation[0] = coordinates_tree_unsrt[(*mtr.tree.nodes[b].parent).label];
          std::sort(coordinates_mutation.begin(), coordinates_mutation.end());

          std::vector<int> current_branches(data.N, 0.0);

          int num_carriers = 0, num_lineages = 1, k_when_mutation_appears = -1, k_when_mutation_has_freq2 = -1;
          int n_mut = root;
          int n_tree = root;
          int ep = num_epochs-1; 

          while(coordinates_tree[n_tree] < epoch[ep]){
            os_freq << 0 << " ";
            os_lin  << 0 << " ";
            ep--;
          }

          // coordinates_tree[n_tree] >= epoch[ep]
          do{

            assert(coordinates_tree[n_tree] >= coordinates_mutation[n_mut]);

            if(coordinates_tree[n_tree] > coordinates_mutation[n_mut]){
              num_lineages++;
              n_tree--;
            }else{
              assert(coordinates_tree[n_tree] == coordinates_mutation[n_mut]);

              if(k_when_mutation_appears == -1){

                num_lineages++;
                k_when_mutation_appears = num_lineages;
                current_branches[0]     = mtr.tree.nodes[b].label;

                n_tree--;
                n_mut--;

              }else{

                float coords = coordinates_mutation[n_mut];
                while(coords == coordinates_mutation[n_mut] && coords != 0.0){

                  num_lineages++;
                  num_carriers++;

                  //check which branch has same coordinates as n_tree-1
                  bool check_if_branch_is_found = false;
                  for(int k = 0; k < num_carriers; k++){
                    //std::cerr << "c " << coordinates_tree_unsrt[current_branches[k]] << " " << coordinates_mutation[n_mut] << std::endl;
                    if(coordinates_tree_unsrt[current_branches[k]] == coordinates_mutation[n_mut]){
                      int branch                       = current_branches[k];
                      current_branches[k]              = (*mtr.tree.nodes[branch].child_left).label;
                      current_branches[num_carriers]   = (*mtr.tree.nodes[branch].child_right).label; 

                      check_if_branch_is_found = true;
                      break;
                    }
                  }
                  assert(check_if_branch_is_found);

                  n_tree--;
                  n_mut--;

                }

              }

            } 

            if(num_carriers == 1){
              if(k_when_mutation_has_freq2 == -1){
                k_when_mutation_has_freq2 = num_lineages;
              }
            }

            assert(coordinates_mutation[n_mut] <= coordinates_tree[n_tree]);
            while(coordinates_tree[n_tree] < epoch[ep]){

              float num_muts = 0.0;
              if(k_when_mutation_appears != -1){ 

                if(num_carriers == 0){

                  for(int k = 0; k <= num_carriers; k++){
                    int branch   = current_branches[k];

                    assert(epoch[ep] >= coordinates_tree_unsrt[branch]);
                    assert(coordinates_tree_unsrt[branch] <= coordinates_mutation[n_mut]);
                    assert( coordinates_tree_unsrt[(*mtr.tree.nodes[branch].parent).label] - epoch[ep] <= (coordinates_tree_unsrt[(*mtr.tree.nodes[branch].parent).label] - coordinates_tree_unsrt[branch]));
                    num_muts    += (coordinates_tree_unsrt[(*mtr.tree.nodes[branch].parent).label] - epoch[ep])/(coordinates_tree_unsrt[(*mtr.tree.nodes[branch].parent).label] - coordinates_tree_unsrt[branch]); 

                  }

                  assert(1 + num_muts <= num_lineages);
                  //os << (1 + num_muts)/((double) num_lineages) << " ";

                  os_freq << num_muts << " ";
                  os_lin  << num_lineages << " "; 

                }else{
                  os_freq << (1 + num_carriers) << " ";
                  os_lin  << num_lineages << " "; 
                }

              }else{
                os_freq << 0 << " ";
                os_lin  << num_lineages << " ";
              }


              ep--;
              if(ep == -1) break;
            }

          }while(n_tree >= data.N);

          assert(num_lineages == data.N);
          num_carriers++;
          os_freq << num_carriers << " ";
          os_lin  << num_lineages << " ";

          os_freq << " " << num_carriers << " ";
          int carriers = 0;
          for(std::vector<int>::iterator it_freq = snp_info.freq.begin(); it_freq != snp_info.freq.end(); it_freq++){
            carriers += *it_freq;
          }
          os_freq << carriers << "\n";

          os_lin  << k_when_mutation_has_freq2 << "\n";
        }

      }

    }
  }

  is_anc.close();
  os_freq.close();
  os_lin.close();

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


void 
Quality(cxxopts::Options& options){

  //////////////////////////////////
  //Program options
  bool help = false;
  if(!options.count("input") || !options.count("output")){
    std::cout << "Not enough arguments supplied." << std::endl;
    std::cout << "Needed: input, output. Optional: first_snp, last_snp." << std::endl;
    help = true;
  }
  if(options.count("help") || help){
    std::cout << options.help({""}) << std::endl;
    std::cout << "..." << std::endl;
    exit(0);
  }  

  std::cerr << "------------------------------------------------------" << std::endl;
  std::cerr << "Annotating quality of SNPs for " + options["input"].as<std::string>() + ".\n";

  std::string line, read;

  ////////// PARSE DATA //////////

  int N;
  igzstream is_N;
  is_N.open(options["input"].as<std::string>() + ".anc");
  if(is_N.fail()) is_N.open(options["input"].as<std::string>() + ".anc.gz");
  if(is_N.fail()){
    std::cerr << "Error while opening .anc file." << std::endl;
    exit(1);
  } 
  is_N.ignore(256, ' ');
  is_N >> N;
  is_N.close();

  int L = 0;
  igzstream is_L;
  is_L.open(options["input"].as<std::string>() + ".mut");
  if(is_L.fail()) is_L.open(options["input"].as<std::string>() + ".mut.gz"); 
  if(is_L.fail()){
    std::cerr << "Error while opening .mut file." << std::endl;
    exit(1);
  } 
  std::string unused;
  std::getline(is_L, unused); 
  while ( std::getline(is_L, unused) ){
    ++L;
  }
 

  Data data(N,L);
  int N_total = 2*data.N-1;

  int first_snp, last_snp;
  if(!options.count("first_snp")){
    first_snp = 0;
  }else{
    first_snp = options["first_snp"].as<int>();
  }
  if(!options.count("last_snp")){
    last_snp = data.L-1;
  }else{
    last_snp = options["last_snp"].as<int>();
  }


  //read mutations file
  Mutations mutations(data);
  mutations.Read(options["input"].as<std::string>() + ".mut");

  std::vector<int> SNPmapping(mutations.info.size()), tree_index(mutations.info.size());
  std::vector<int>::iterator it_SNPmapping = SNPmapping.begin(), it_tree_index = tree_index.begin();
  for(std::vector<SNPInfo>::iterator it_info = mutations.info.begin(); it_info != mutations.info.end(); it_info++){
    *it_SNPmapping = ((*it_info).branch.size() > 1);
    *it_tree_index = (*it_info).tree;
    it_tree_index++;
    it_SNPmapping++;
  }


  //open anc file
  MarginalTree mtr;
  int root = N_total-1;
  int snp_of_next_tree;
  std::vector<Leaves> leaves;

  igzstream is_anc(options["input"].as<std::string>() + ".anc");
  if(is_anc.fail()) is_anc.open(options["input"].as<std::string>() + ".anc.gz");
  if(is_anc.fail()){
    std::cerr << "Error while opening .anc file." << std::endl;
    exit(1);
  }
  std::ofstream os(options["output"].as<std::string>() + ".qual");
  if(is_anc.fail()){
    std::cerr << "Error while opening file." << std::endl;
    exit(1);
  }

  getline(is_anc,line);
  getline(is_anc,line);
  getline(is_anc,line);

  //read tree
  mtr.Read(line, N);
  int num_snps_on_tree = 0;
  int num_snps_not_mapping_to_tree = 0;
  int i = first_snp;
  while(tree_index[i] == tree_index[first_snp]){
    num_snps_not_mapping_to_tree += SNPmapping[i];
    i++;
  }
  int num_snps_tree_persisting = i - first_snp;

  for(std::vector<Node>::iterator it_node = mtr.tree.nodes.begin(); it_node != mtr.tree.nodes.end(); it_node++){
    num_snps_on_tree += (*it_node).num_events;
  }
  mtr.tree.FindAllLeaves(leaves);

  SNPInfo snp_info;
  int freq;
  int count_tree = 0;
  int num_snps_not_mapping = 0;

  os << "ID pos decendants_below_branch carriers_in_data fraction_snps_not_mapping\n";

  if(last_snp - first_snp < 1000){
    std::cerr << "Need at least 1000 SNPs." << std::endl;
    exit(1);
  }

  for(int snp = first_snp; snp < first_snp + 500; snp++){
    num_snps_not_mapping += SNPmapping[snp];
  }

  for(int snp = first_snp; snp <= last_snp; snp++){

    snp_info = mutations.info[snp];

    if(snp - first_snp < 500){
      num_snps_not_mapping += SNPmapping[snp+500];
    }else if(last_snp - snp < 500){
      num_snps_not_mapping -= SNPmapping[snp-500];
    }else{
      num_snps_not_mapping += SNPmapping[snp+500] - SNPmapping[snp - 500];
    }

    if(count_tree < snp_info.tree){
      while(count_tree < snp_info.tree){
        if(!getline(is_anc,line)){
          break; 
        };
        count_tree++;
      } 
      assert(count_tree == snp_info.tree);

      //read tree
      mtr.Read(line, N);

      i = snp; 
      num_snps_not_mapping_to_tree = 0;
      while(tree_index[i] == tree_index[snp]){
        num_snps_not_mapping_to_tree += SNPmapping[i];
        i++;
      }
      num_snps_tree_persisting = i - snp;

      num_snps_on_tree = 0;
      for(std::vector<Node>::iterator it_node = mtr.tree.nodes.begin(); it_node != mtr.tree.nodes.end(); it_node++){
        num_snps_on_tree += (*it_node).num_events;
      }
      mtr.tree.FindAllLeaves(leaves);
    }

    int b = *snp_info.branch.begin();
    int carriers = 0;
    for(std::vector<int>::iterator it_freq = snp_info.freq.begin(); it_freq != snp_info.freq.end(); it_freq++){
      carriers += *it_freq;
    }
    os << snp_info.rs_id << " " << snp_info.pos << " ";
    os << leaves[b].num_leaves << " " << carriers << " ";
    //os << num_snps_not_mapping_to_tree/((float) num_snps_tree_persisting) << " " << num_snps_tree_persisting << " ";

    if(snp - first_snp < 500){
      os << num_snps_not_mapping/((float) (500 + snp - first_snp + 1));
    }else if(last_snp - snp < 500){
      os << num_snps_not_mapping/((float) (500 + last_snp - snp));
    }else{
      os << num_snps_not_mapping/1000.0;
    }

    os << "\n";


  }

  is_anc.close();
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



}


int main(int argc, char* argv[]){

  //////////////////////////////////
  //Program options
  cxxopts::Options options("RelateSelection");
  options.add_options()
    ("help", "Print help")
    ("mode", "Choose which part of the algorithm to run.", cxxopts::value<std::string>())
    ("first_snp", "Index of first SNP. Optional.", cxxopts::value<int>())
    ("last_snp", "Index of last SNP. Optional.", cxxopts::value<int>())
    ("quality", "Optional: Filename of file containing quality assessment of SNPs (obtained using mode quality).", cxxopts::value<std::string>())
    ("threshold", "Optional: Threshold for number of mutations that trees need for inclusion. Default = 0.", cxxopts::value<int>())
    ("years_per_gen", "Optional: Years per generation (float). Default: 28.", cxxopts::value<float>())
    ("num_bins", "Optional: Number of bins.", cxxopts::value<int>())
    ("i,input", "Filename of .anc and .mut file without file extension", cxxopts::value<std::string>())
    ("o,output", "Output file", cxxopts::value<std::string>());

  options.parse(argc, argv);
  std::string mode = options["mode"].as<std::string>();

  if(!mode.compare("Selection")){

    Selection(options);

  }else if(!mode.compare("Frequency")){

    Frequency(options);

  }else if(!mode.compare("Quality")){

    Quality(options);

  }else{

    std::cout << "####### error #######" << std::endl;
    std::cout << "Invalid or missing mode." << std::endl;
    std::cout << "Options for --mode are:" << std::endl;
    std::cout << "Frequency, Selection, Quality." << std::endl;

  }

  bool help = false;
  if(!options.count("mode")){
    std::cout << "Not enough arguments supplied." << std::endl;
    help = true;
  }
  if(options.count("help") || help){
    std::cout << options.help({""}) << std::endl;
    exit(0);
  }

}

