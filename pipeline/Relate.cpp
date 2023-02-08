#include <string>
#include <cxxopts.hpp>

#include "MakeChunks.cpp"
#include "Paint.cpp"
#include "BuildTopology.cpp"
#include "FindEquivalentBranches.cpp"
#include "InferBranchLengths.cpp"
#include "CombineSections.cpp"
#include "Finalize.cpp"
#include "Clean.cpp"
#include "OptimizeParameters.cpp"

int main(int argc, char* argv[]){

  //////////////////////////////////
  //Program options  
  cxxopts::Options options("Relate");
  options.add_options()
    ("help", "Print help.")
    ("mode", "Choose which part of the algorithm to run.", cxxopts::value<std::string>()) 
    ("haps", "Filename of haps file (Output file format of Shapeit).", cxxopts::value<std::string>())
    ("sample", "Filename of sample file (Output file format of Shapeit).", cxxopts::value<std::string>())
    ("map", "Genetic map.", cxxopts::value<std::string>())
    ("m,mutation_rate", "Mutation rate.", cxxopts::value<float>())
    ("N,effectiveN", "Effective population size.", cxxopts::value<float>())
    ("o,output", "Filename of output without file extension.", cxxopts::value<std::string>())
    ("dist", "Optional but recommended. Distance in BP between SNPs. Can be generated using RelateFileFormats. If unspecified, distances in haps are used.", cxxopts::value<std::string>())
    ("annot", "Optional. Filename of file containing additional annotation of snps. Can be generated using RelateFileFormats.", cxxopts::value<std::string>()) 
    ("memory", "Optional. Approximate memory allowance in GB for storing distance matrices. Default is 5GB.", cxxopts::value<float>())
    ("sample_ages", "Optional. Filename of file containing sample ages (one per line).", cxxopts::value<std::string>()) 
    ("chunk_index", "Optional. Index of chunk. (Use when running parts of the algorithm on an individual chunk.)", cxxopts::value<int>())
    ("first_section", "Optional. Index of first section to infer. (Use when running parts of algorithm on an individual chunk.)", cxxopts::value<int>())
    ("last_section", "Optional. Index of last section to infer. (Use when running parts of algorithm on an individual chunk.)", cxxopts::value<int>())
    ("coal", "Optional. Filename of file containing coalescent rates. If specified, it will overwrite --effectiveN.", cxxopts::value<std::string>()) 
		("fb", "Optional. Force build a new tree every x bases.", cxxopts::value<float>()) 
    //("anc_allele_unknown", "Specify if ancestral allele is unknown.") 
		("transversion", "Only use transversion for bl estimation.")
    ("i,input", "Filename of input.", cxxopts::value<std::string>())
		("painting", "Optional. Copying and transition parameters in chromosome painting algorithm. Format: theta,rho. Default: 0.025,1.", cxxopts::value<std::string>())
    ("seed", "Optional. Seed for MCMC in branch lengths estimation.", cxxopts::value<int>());

  auto result = options.parse(argc, argv);
  auto help_text = options.help({""});

  std::string mode;
  if (result.count("mode")) {
    mode = result["mode"].as<std::string>();
  } else {
    mode = "";
  }
  if(result.count("output")){
    std::string output = result["output"].as<std::string>();
    for(std::string::iterator it_str = output.begin(); it_str != output.end(); it_str++){
      if(*it_str == '/'){
        std::cerr << "Output needs to be in working directory." << std::endl;
        exit(1);
      }
    }
  }

  if(!mode.compare("MakeChunks")){
  
    MakeChunks(result, help_text);
  
  }else if(!mode.compare("Paint")){
  
    bool help = false;
    if(!result.count("chunk_index") || !result.count("output")){
      std::cout << "Not enough arguments supplied." << std::endl;
      std::cout << "Needed: chunk_index, output." << std::endl; 
      help = true;
    }
    if(result.count("help") || help){
      std::cout << options.help({""}) << std::endl;
      std::cout << "Use after MakeChunks to paint all haps against all." << std::endl;
      exit(0);
    }


    Paint(result, result["chunk_index"].as<int>());
  
  }else if(!mode.compare("BuildTopology")){
  
    bool help = false;
    if(!result.count("chunk_index") || !result.count("output")){
      std::cout << "Not enough arguments supplied." << std::endl;
      //std::cout << "Needed: chunk_index, output. Optional: first_section, last_section, anc_allele_unknown, seed." << std::endl;
      std::cout << "Needed: chunk_index, output. Optional: first_section, last_section, seed." << std::endl; 
      help = true;
    }
    if(result.count("help") || help){
      std::cout << options.help({""}) << std::endl;
      std::cout << "Use after Paint to build tree topologies in a small chunk specified by first section - last_section." << std::endl;
      exit(0);
    }
    if(result.count("first_section") && result.count("last_section")){
    
      int first_section = result["first_section"].as<int>();
      int last_section  = result["last_section"].as<int>();
      BuildTopology(result, result["chunk_index"].as<int>(), first_section, last_section);

    }else{
    
      int N, L, num_sections;
      FILE* fp = fopen(("parameters_c" + std::to_string(result["chunk_index"].as<int>()) + ".bin").c_str(), "r");
      assert(fp != NULL);
      fread(&N, sizeof(int), 1, fp);
      fread(&L, sizeof(int), 1, fp);
      fread(&num_sections, sizeof(int), 1, fp);
      fclose(fp);
      Data data(N,L);
      num_sections--;

      BuildTopology(result, result["chunk_index"].as<int>(), 0, num_sections-1);

    }
  
  }else if(!mode.compare("FindEquivalentBranches")){
 
    if(result.count("chunk_index") || result.count("output")){
      FindEquivalentBranches(result["output"].as<std::string>(), result["chunk_index"].as<int>());
    }else{
      std::cerr << "Please specify the chunk_index, and output" << std::endl;
      exit(1);
    }
 
  }else if(!mode.compare("InferBranchLengths")){
  
    bool help = false;
    if(!result.count("chunk_index")){
      std::cout << "Not enough arguments supplied." << std::endl;
      std::cout << "Needed: effectiveN, mutation_rate, chunk_index, output. Optional: first_section, last_section, sample_ages." << std::endl; 
      help = true;
    }
    if(result.count("help") || help){
      std::cout << options.help({""}) << std::endl;
      std::cout << "Use after PropagateMutations to infer branch lengths." << std::endl;
      exit(0);
    }

    if(result.count("first_section") && result.count("last_section")){
    
      int first_section = result["first_section"].as<int>();
      int last_section  = result["last_section"].as<int>();
      GetBranchLengths(result, help_text, result["chunk_index"].as<int>(), first_section, last_section);

    }else{
    
      int N, L, num_sections;
      FILE* fp = fopen((result["output"].as<std::string>() + "/parameters_c" + std::to_string(result["chunk_index"].as<int>()) + ".bin").c_str(), "r");
      assert(fp != NULL);
      fread(&N, sizeof(int), 1, fp);
      fread(&L, sizeof(int), 1, fp);
      fread(&num_sections, sizeof(int), 1, fp);
      fclose(fp);
      Data data(N,L);
      num_sections--;

      GetBranchLengths(result, help_text, result["chunk_index"].as<int>(), 0, num_sections-1);
    }

  }else if(!mode.compare("CombineSections")){

    if(result.count("chunk_index")){
      CombineSections(result, help_text, result["chunk_index"].as<int>());
    }else{
      std::cerr << "Please specify the chunk_index" << std::endl;
      exit(1);
    }

  }else if(!mode.compare("Finalize")){
  
    Finalize(result, help_text);
  
  }else if(!mode.compare("Clean")){
  
    Clean(result, help_text);
  
  }else if(!mode.compare("All")){
    
    bool popsize = false;
    if(!result.count("effectiveN") && !result.count("coal")) popsize = true;
    bool help = false;
    if(!result.count("haps") || !result.count("sample") ||  !result.count("map") || popsize || !result.count("mutation_rate") || !result.count("output")){
      std::cout << "Not enough arguments supplied." << std::endl;
      //std::cout << "Needed: haps, sample, map, mutation_rate, effectiveN, output. Optional: seed, annot, dist, coal, max_memory, sample_ages, chunk_index, anc_allele_unknown." << std::endl;
      std::cout << "Needed: haps, sample, map, mutation_rate, effectiveN, output. Optional: seed, annot, dist, coal, max_memory, sample_ages, chunk_index." << std::endl;
      help = true;
    }
    if(result.count("help") || help){
      std::cout << options.help({""}) << std::endl;
      std::cout << "Executes all stages of the algorithm." << std::endl;
      exit(0);
    }
 
    std::cerr << std::endl;
    std::cerr << "*********************************************************" << std::endl;
    std::cerr << "---------------------------------------------------------" << std::endl;
    std::cerr << "Relate" << std::endl;
    std::cerr << " * Authors: Leo Speidel, Marie Forest, Sinan Shi, Simon Myers." << std::endl;
    std::cerr << " * Doc:     https://myersgroup.github.io/relate" << std::endl;
    std::cerr << "---------------------------------------------------------" << std::endl << std::endl;

    int N, L;
    double memory_size = 0.0;
    int start_chunk, end_chunk;
    std::string file_out = result["output"].as<std::string>() + "/";

    if(result.count("chunk_index")){

      std::cerr << "  chunk " << result["chunk_index"].as<int>() << std::endl;

      FILE* fp = fopen((file_out + "parameters_c" + std::to_string(result["chunk_index"].as<int>()) + ".bin").c_str(), "r");
      assert(fp != NULL);
      fread(&N, sizeof(int), 1, fp);
      fread(&L, sizeof(int), 1, fp);
      fclose(fp);

      start_chunk = result["chunk_index"].as<int>();
      end_chunk   = start_chunk;
    }else{

      std::cerr << "---------------------------------------------------------" << std::endl;
      std::cerr << "Using:" << std::endl;
      std::cerr << "  " << result["haps"].as<std::string>() << std::endl;    
      std::cerr << "  " << result["sample"].as<std::string>() << std::endl;
      std::cerr << "  " << result["map"].as<std::string>() << std::endl;
      if(!result.count("coal")){
        std::cerr << "with mu = " << result["mutation_rate"].as<float>() << " and 2Ne = " << result["effectiveN"].as<float>() << "." << std::endl;
      }else{
        std::cerr << "with mu = " << result["mutation_rate"].as<float>() << " and coal = " << result["coal"].as<std::string>() << "." << std::endl;
      }
      
      std::cerr << "---------------------------------------------------------" << std::endl << std::endl;

      MakeChunks(result, help_text);

      FILE* fp = fopen((file_out + "parameters.bin").c_str(), "r");
      assert(fp != NULL);
      fread(&N, sizeof(int), 1, fp);
      fread(&L, sizeof(int), 1, fp);
      fread(&end_chunk, sizeof(int), 1, fp);
      fread(&memory_size, sizeof(double), 1, fp);
      fclose(fp);

      end_chunk--;
      start_chunk = 0;
    }


    Data data(N,L);
    std::cerr << "---------------------------------------------------------" << std::endl;
    std::cerr << "Read " << data.N << " haplotypes with " << data.L << " SNPs per haplotype." << std::endl;
    std::cerr << "Expected minimum memory usage: " << memory_size << "Gb." << std::endl;
    std::cerr << "---------------------------------------------------------" << std::endl << std::endl;


    for(int c = start_chunk; c <= end_chunk; c++){

      std::cerr << "---------------------------------------------------------" << std::endl;
      std::cerr << "Starting chunk " << c << " of " << end_chunk << "." << std::endl;
      std::cerr << "---------------------------------------------------------" << std::endl << std::endl;


      int N, L, num_sections;
      FILE* fp = fopen((file_out + "parameters_c" + std::to_string(c) + ".bin").c_str(), "r");
      assert(fp != NULL);
      fread(&N, sizeof(int), 1, fp);
      fread(&L, sizeof(int), 1, fp);
      fread(&num_sections, sizeof(int), 1, fp);
      fclose(fp);
      num_sections--;

      Paint(result, c);
      BuildTopology(result, c, 0, num_sections-1);
      FindEquivalentBranches(result["output"].as<std::string>(), c);
      GetBranchLengths(result, help_text, c, 0, num_sections-1);
      CombineSections(result, help_text, c);

    }

    if(!result.count("chunk_index")){
      Finalize(result, help_text);
    }

    std::cerr << "---------------------------------------------------------" << std::endl;
    std::cerr << "Done." << std::endl;
    std::cerr << "---------------------------------------------------------" << std::endl;
    std::cerr << "*********************************************************" << std::endl << std::endl;

  }else if(!mode.compare("OptimizeParameters")){

    OptimizeParameters(result, help_text);

	}else{

    std::cout << "####### error #######" << std::endl;
    std::cout << "Invalid or missing mode." << std::endl;
    std::cout << "Options for --mode are:" << std::endl;
    std::cout << "All, Clean, MakeChunks, Paint, BuildTopology, FindEquivalentBranches, InferBranchLengths, CombineSections, Finalize, OptimizeParameters." << std::endl;
  
  }

  bool help = false;
  if(!result.count("mode")){
    std::cout << "Not enough arguments supplied." << std::endl;
    help = true;
  }
  if(result.count("help") || help){
    std::cout << help_text << std::endl;
    exit(0);
  }

  return 0;

}

