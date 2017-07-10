#include "dfise_converter.h"

#include <algorithm>
#include <cfloat>
#include <climits>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "Octree.hpp"
#include "read_dfise.h"

int main(int argc, char** argv) {
    auto region = "bulk";
    size_t xdiv = 100;
    size_t ydiv = 100;
    size_t zdiv = 100;

    if(argc < 2) {
        std::cerr << "filename of point cloud missing." << std::endl;
        return -1;
    }
    std::string filename = argv[1];

    std::cerr << "Reading grid" << std::endl;
    std::string grid_file = filename + ".grd";

    std::vector<Point> points;
    try {
        auto region_grid = read_grid(grid_file);
        points = region_grid[region];
    } catch(std::runtime_error& e) {
        std::cerr << "Failed to parse grid file " << grid_file << std::endl;
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }

    std::cerr << "Reading electric field" << std::endl;
    std::string data_file = filename + ".dat";
    std::vector<Point> field;
    try {
        auto region_fields = read_electric_field(data_file);
        field = region_fields[region];
    } catch(std::runtime_error& e) {
        std::cerr << "Failed to parse data file " << data_file << std::endl;
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }

    if(points.size() != field.size()) {
        std::cerr << "Field and grid do not match" << std::endl;
        return 1;
    }

    /*
     * ALERT swap coordinates to match ap2 system
     * FIXME has to be checked more carefully
     */
    for(size_t i = 0; i < points.size(); ++i) {
        std::swap(points[i].y, points[i].z);
        std::swap(field[i].y, field[i].z);
    }

    // Find minimum and maximum coordinate
    double minx = DBL_MAX, miny = DBL_MAX, minz = DBL_MAX;
    double maxx = DBL_MIN, maxy = DBL_MIN, maxz = DBL_MIN;
    for(auto& point : points) {
        minx = std::min(minx, point.x);
        miny = std::min(miny, point.y);
        minz = std::min(minz, point.z);

        maxx = std::max(maxx, point.x);
        maxy = std::max(maxy, point.y);
        maxz = std::max(maxz, point.z);
    }

    // Initializing the Octree with points from point cloud.
    unibn::Octree<Point> octree;
    octree.initialize(points);

    // Find approximate data
    std::vector<Point> pts;
    double xstep = (maxx - minx) / static_cast<double>(xdiv);
    double ystep = (maxy - miny) / static_cast<double>(ydiv);
    double zstep = (maxz - minz) / static_cast<double>(zdiv);

    double x = minx + xstep / 2.0;
    for(size_t i = 0; i < xdiv; ++i) {
        double y = miny + ystep / 2.0;
        for(size_t j = 0; j < ydiv; ++j) {
            double z = minz + zstep / 2.0;
            for(size_t k = 0; k < xdiv; ++k) {
                Point q(x, y, z);

                // Find nearest point
                std::vector<unsigned int> results;
                octree.radiusNeighbors<unibn::L2Distance<Point>>(q, 1.0f, results);
                std::sort(results.begin(), results.end(), [&](size_t a, size_t b) {
                    return unibn::L2Distance<Point>::compute(points[a], q) < unibn::L2Distance<Point>::compute(points[b], q);
                });

                if(results.empty()) {
                    std::cout << x << " " << y << " " << z << std::endl;
                    std::cerr << "distance not enough" << std::endl;
                    return 1;
                }

                pts.push_back(field[results[0]]);

                z += zstep;
            }
            y += ystep;
        }
        x += xstep;
    }

    // Write INIT file header
    std::cout << "tcad_octree_writer" << std::endl;                                    // NAME
    std::cout << "##SEED## ##EVENTS##" << std::endl;                                   // UNUSED
    std::cout << "##TURN## ##TILT## 1.0" << std::endl;                                 // UNUSED
    std::cout << "0.0 0.0 0.0" << std::endl;                                           // MAGNETIC FIELD (UNUSED)
    std::cout << (maxz - minz) << " " << (maxx - minx) << " " << (maxy - miny) << " "; // PIXEL DIMENSIONS
    std::cout << "0.0 0.0 0.0 0.0 ";                                                   // UNUSED
    std::cout << xdiv << " " << ydiv << " " << zdiv << " ";
    std::cout << "0.0" << std::endl; // UNUSED

    // Write INIT file data
    for(size_t i = 0; i < xdiv; ++i) {
        for(size_t j = 0; j < ydiv; ++j) {
            for(size_t k = 0; k < zdiv; ++k) {
                auto& point = pts[i * ydiv * zdiv + j * zdiv + k];
                std::cout << i + 1 << " " << j + 1 << " " << k + 1 << " " << point.x << " " << point.y << " " << point.z
                          << std::endl;
            }
        }
    }
}
