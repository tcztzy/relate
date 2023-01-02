//All against all painting.
#ifndef PAINTING
#define PAINTING

#include <filesystem>
#include <iostream>
#include <cxxopts.hpp>

#include "data.hpp"
#include "fast_painting.hpp"
#include "usage.hpp"

namespace fs = std::filesystem;

int Paint(cxxopts::ParseResult& result, int chunk_index){

  const fs::path file_out{result["output"].as<fs::path>()};

  int N, L, num_windows;
  std::vector<int> window_boundaries;
  FILE* fp = fopen((file_out / ("parameters_c" + std::to_string(chunk_index) + ".bin")).c_str(), "r");
  assert(fp != NULL);
  fread(&N, sizeof(int), 1, fp);
  fread(&L, sizeof(int), 1, fp);
  fread(&num_windows, sizeof(int), 1, fp);
  window_boundaries.resize(num_windows);
  fread(&window_boundaries[0], sizeof(int), num_windows, fp);
  fclose(fp);
  num_windows--;

  fs::path basename = file_out / ("chunk_" + std::to_string(chunk_index));
	Data data(basename.replace_extension("hap").c_str(), basename.replace_extension("bp").c_str(), basename.replace_extension("dist").c_str(), basename.replace_extension("r").c_str(), basename.replace_extension("rpos").c_str(), basename.replace_extension("state").c_str()); //struct data is defined in data.hpp 
  data.name = (file_out / ("chunk_" + std::to_string(chunk_index)) / "paint" / "relate");

  if(result.count("painting")){

	  std::string val;
		std::string painting = result["painting"].as<std::string>();
		int i = 0;
		for(;i < painting.size(); i++){
      if(painting[i] == ',') break;
			val += painting[i];
		}
		data.theta = std::stof(val);
		data.ntheta = 1.0 - data.theta;

		i++;
		val.clear();
		for(;i < painting.size(); i++){
			if(painting[i] == ',') break;
			val += painting[i];
		}
    double rho = std::stof(val);
		for(int l = 0; l < (int)data.r.size(); l++){
			data.r[l] *= rho;
		}

	}

  std::cerr << "---------------------------------------------------------" << std::endl;
  std::cerr << "Painting sequences..." << std::endl;

  //create directory called paint/ if not existent
  const fs::path chunk_dir = file_out / ("chunk_" + std::to_string(chunk_index));
  fs::create_directory(chunk_dir, file_out);
  fs::create_directory(chunk_dir / "painting", chunk_dir);

  //////////////////////////////////////////// Paint sequence ////////////////////////////

  char filename[1024];
  std::vector<FILE*> pfiles(num_windows);
  for(int w = 0; w < num_windows; w++){
    snprintf(filename, sizeof(char) * 1024, "%s_%i.bin", data.name.c_str(), w);
    pfiles[w] = fopen(filename, "wb");
    assert(pfiles[w] != NULL);
  }

  for(int hap = 0; hap < data.N; hap++){
    //std::cerr << hap << std::endl;
    FastPainting painter(data);
    painter.PaintSteppingStones(data, window_boundaries, pfiles, hap);
  }

  for(int w = 0; w < num_windows; w++){  
    fclose(pfiles[w]);
  }

  ResourceUsage();

  return 0;
}

#endif //PAINTING
