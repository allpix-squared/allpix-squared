#include <string>
#include <vector>

#include "Octree.hpp"
#include "read_dfise.h"

#include "core/utils/log.h"

namespace mesh_converter {
    void mesh_plotter(const std::string& grid_file,
                      double ss_radius,
                      double radius,
                      double x,
                      double y,
                      double z,
                      std::vector<mesh_converter::Point> points,
                      std::vector<unsigned int> results);
} // namespace mesh_converter
