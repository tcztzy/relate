#include "GetTreeOfInterest.cpp"
#include "CreateAncesTreeFileForSubpopulation.cpp"
#include "RemoveTreesWithFewMutations.cpp"
#include "AncMutChunks.cpp"
#include "Convert.cpp"
#include "Annotate.cpp"

#include "filesystem.hpp"
#include <cxxopts.hpp>
#include <string>

int main(int argc, char* argv[]){

  //////////////////////////////////
  //Program options  
  cxxopts::Options options("RelateExtract");
  options.add_options()
    ("help", "Print help.")
    ("mode", "Choose which part of the algorithm to run.", cxxopts::value<std::string>())
    ("poplabels", "Filename of file containing population labels.", cxxopts::value<std::string>()) 
    ("anc", "Filename of file containing trees.", cxxopts::value<std::string>())
    ("mut", "Filename of file containing mut.", cxxopts::value<std::string>())
		("haps", "Filename of haps file (Output file format of Shapeit).", cxxopts::value<std::string>())
		("sample", "Filename of sample file (Output file format of Shapeit).", cxxopts::value<std::string>())
		("dist", "Optional but recommended. Distance in BP between SNPs. Can be generated using RelateFileFormats. If unspecified, distances in haps are used.", cxxopts::value<std::string>())
    ("pop_of_interest", "Population label. If not specified, use all haplotypes.", cxxopts::value<std::string>())
    ("bp_of_interest", "BP of position of interest.", cxxopts::value<int>())
    ("first_bp", "BP of first SNP of interest.", cxxopts::value<int>())
    ("last_bp", "BP of last SNP of interest.", cxxopts::value<int>())
		("years_per_gen", "Years per generation.", cxxopts::value<float>())
    ("threshold", "Threshold used in RemoveTreesWithFewMutations.", cxxopts::value<float>())
		("anc_genome", "Fasta file containing ancestral genome.", cxxopts::value<std::string>())
    ("threads", "Optional: Number of threads used (only used to decide chunk size in DivideAncMut)", cxxopts::value<int>())
		("transversion", "Only use transversion for bl estimation.")
		("i,input", "Filename of input (excl file extension).", cxxopts::value<std::string>())
    ("o,output", "Filename of output (excl file extension).", cxxopts::value<std::string>());

  auto result = options.parse(argc, argv);

  std::string mode = result["mode"].as<std::string>();

  if(!mode.compare("AncToNewick")){
  
    GetTreeOfInterest(options, result);

  }else if(!mode.compare("SubTreesForSubpopulation")){
  
    CreateAncesTreeFileForSubpopulation(options, result);

  }else if(!mode.compare("AncMutForSubregion")){

    bool help = false;
    if(!result.count("anc") || !result.count("mut") || !result.count("first_bp") || !result.count("last_bp") || !result.count("output")){
      std::cout << "Not enough arguments supplied." << std::endl;
      std::cout << "Needed: anc, mut, first_bp, last_bp, output." << std::endl;
      help = true;
    }

    GetDistFromMut(options, result);
    AncMutForSubregion(options, result);

  }else if(!mode.compare("RemoveTreesWithFewMutations")){
  
    GetDistFromMut(options, result);
    RemoveTreesWithFewMutations(options, result);

  }else if(!mode.compare("ExtractDistFromMut")){
 
    GetDistFromMut(options, result);

  }else if(!mode.compare("DivideAncMut")){
  
    DivideAncMut(options, result);
  
  }else if(!mode.compare("CombineAncMut")){
  
    CombineAncMut(options, result);

	}else if(!mode.compare("ConvertNewickToTimeb")){

		ConvertNewickToTimeb(options, result);

	}else if(!mode.compare("MapMutations")){

		GetDistFromMut(options, result);
		MapMutation(options, result);

	}else if(!mode.compare("GenerateSNPAnnotationsUsingTree")){

		GenerateSNPAnnotationsUsingTree(options, result);

	}else if(!mode.compare("UnlinkTips")){

		UnlinkTips(options, result);

	}else if(!mode.compare("GetAllBranchesOfMut")){

		PropagateMutations(options, result);

	}else if(!mode.compare("CountMutonBranches")){

		PrintMutonBranches(options, result);

	}else{

    std::cout << "####### error #######" << std::endl;
    std::cout << "Invalid or missing mode." << std::endl;
    std::cout << "Options for --mode are:" << std::endl;
    std::cout << "AncToNewick, SubTreesForSubpopulation, RemoveTreesWithFewMutations, ExtractDistFromMut, DivideAncMut, CombineAncMut, AncMutForSubregion, ConvertNewickToTimeb, MapMutations, GenerateSNPAnnotationsUsingTree, GetAllBranchesOfMut, CountMutonBranches." << std::endl;
  
  }

  bool help = false;
  if(!result.count("mode")){
    std::cout << "Not enough arguments supplied." << std::endl;
    help = true;
  }
  if(result.count("help") || help){
    std::cout << options.help({""}) << std::endl;
    exit(0);
  }

  return 0;

}

