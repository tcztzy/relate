#ifndef FAST_PAINTING_HPP
#define FAST_PAINTING_HPP

#include <ctgmath>
#include <vector>

#include "data.hpp"
#include "collapsed_matrix.hpp"

class FastPainting{

  private:

    double lower_rescaling_threshold, upper_rescaling_threshold;
    double Nminusone, prior_theta, prior_ntheta, theta_ratio, log_ntheta, log_small;
    double normalizing_constant, distance_rescaling_constant;  

  public:
    FastPainting(size_t N, double theta = 0.001) {
      lower_rescaling_threshold = 1e-10;
      upper_rescaling_threshold = 1.0/lower_rescaling_threshold;

      assert(theta < 1.0);
      double ntheta = 1.0 - theta;
      Nminusone = (double)N - 1.0;
      prior_theta  = theta/Nminusone - ntheta/Nminusone;
      prior_ntheta = ntheta/Nminusone;
      theta_ratio = theta/ntheta - 1.0;
      log_ntheta = log(ntheta);
      log_small  = log(0.01);
    }
    FastPainting(Data& data){

      lower_rescaling_threshold = 1e-10;
      upper_rescaling_threshold = 1.0/lower_rescaling_threshold;

      assert(data.theta < 1.0);
      Nminusone = data.N - 1.0;
      prior_theta  = data.theta/Nminusone - data.ntheta/Nminusone;
      prior_ntheta = data.ntheta/Nminusone;
      theta_ratio = data.theta/(1.0 - data.theta) - 1.0;
      log_ntheta = log(data.ntheta);
      log_small  = log(0.01);
 
    }
  
    void PaintSteppingStones(const Data& data, std::vector<int>& window_boundaries, std::vector<FILE*> pfiles, const int k);
    void PaintSteppingStones(const Data& data, const char* basename, size_t num_windows, const int *window_boundaries, const int k) const;
    void RePaintSection(const Data& data, CollapsedMatrix<float>& topology, std::vector<float>& logscales, CollapsedMatrix<float>& alpha_begin, CollapsedMatrix<float>& beta_end, int boundarySNP_begin, int boundarySNP_end, float logscale_alpha, float logscale_beta, const int k);

};

#endif //FAST_PAINTING_HPP
