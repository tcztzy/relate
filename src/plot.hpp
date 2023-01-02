#ifndef PLOT_HPP
#define PLOT_HPP

#include <vector>

class plot{


  private:

    int width, height;

  public:

    plot(int width, int height): width(width), height(height){}
    void draw(const std::vector<float>& x, const std::vector<double>&y);

};

#endif //PLOT_HPP
