#ifndef EMP_COLOR_H
#define EMP_COLOR_H

#include <iomanip>
#include <string>
#include <sstream>

#include "Color.h"

namespace emp {

  class ColorMap {
  private:
    std::vector<Color> color_map;
  public:
    ColorMap(int size, double autocolor=0) : color_map(size) {
      //  The value in autocolor determines the max degree of the hue.
      if (autocolor) {
        const double step = autocolor / (double) size;
        for (int i = 0; i < size; i++) {
          const double hue = 330 + step * (double) i;
          std::stringstream stream;
          stream << "hsl(" << hue << ",100%,50%)";
          color_map[i].Set(stream.str());
        }
      }
    }
    ~ColorMap() { ; }

    int GetSize() const { return (int) color_map.size(); }

    Color & operator[](int id) { return color_map[id]; }
    const Color & operator[](int id) const { return color_map[id]; }
  };

}

#endif

