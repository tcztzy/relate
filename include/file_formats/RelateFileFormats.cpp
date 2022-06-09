#include "FileFormats.cpp"
#include "ConvertToTreeSequence.cpp"

#include <cxxopts.hpp>
#include <string>

int main(int argc, char* argv[]){

  //////////////////////////////////
  //Program options  
  cxxopts::Options options("RelateFileFormats");
  options.add_options()
    ("help", "Print help.")
    ("mode", "Choose which part of the algorithm to run.", cxxopts::value<std::string>())
    ("haps", "Filename of haps file (Output file format of Shapeit).", cxxopts::value<std::string>())
    ("sample", "Filename of sample file (Output file format of Shapeit).", cxxopts::value<std::string>())
    ("map", "Filename of genetic map.", cxxopts::value<std::string>())
    ("mask", "Filename of file containing mask", cxxopts::value<std::string>())
    ("ancestor", "Filename of file containing human ancestor genome.", cxxopts::value<std::string>())
    ("poplabels", "Filename of file containing population labels.", cxxopts::value<std::string>()) 
    ("chr", "Chromosome index", cxxopts::value<int>())
    ("mut", "Filename of .mut file", cxxopts::value<std::string>())
		("flag", "Flag for different options in each mode", cxxopts::value<std::string>())
    ("i,input", "Filename of input.", cxxopts::value<std::string>())
    ("o,output", "Filename of output (excl file extension).", cxxopts::value<std::string>());

  auto result = options.parse(argc, argv);

  std::string mode = result["mode"].as<std::string>();

  if(!mode.compare("ConvertFromHapLegendSample")){
  
    ConvertFromHapLegendSample(options, result);
  
  }else if(!mode.compare("ConvertFromVcf")){
  
    ConvertFromVcf(options, result);
  
  }else if(!mode.compare("RemoveNonBiallelicSNPs")){
  
    RemoveNonBiallelicSNPs(options, result);
  
  }else if(!mode.compare("RemoveSamples")){
  
    RemoveSamples(options, result);
  
  }else if(!mode.compare("FilterHapsUsingMask")){
  
    FilterHapsUsingMask(options, result);
  
  }else if(!mode.compare("FlipHapsUsingAncestor")){
  
    FlipHapsUsingAncestor(options, result);

  }else if(!mode.compare("GenerateSNPAnnotations")){
  
    GenerateSNPAnnotations(options, result);
 
  }else if(!mode.compare("ConvertToTreeSequenceTxt")){
  
    ConvertToTreeSequenceTxt(options, result);
 
  }else if(!mode.compare("ConvertToTreeSequence")){
  
    ConvertToTreeSequence(options, result);
 
  }else{

    std::cout << "####### error #######" << std::endl;
    std::cout << "Invalid or missing mode." << std::endl;
    std::cout << "Options for --mode are:" << std::endl;
    std::cout << "ConvertFromHapLegendSample, ConvertFromVcf, RemoveNonBiallelicSNPs, RemoveSamples, FilterHapsUsingMask, FlipHapsUsingAncestor, GenerateSNPAnnotations, ConvertToTreeSequenceTxt, ConvertToTreeSequence." << std::endl;
  
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

