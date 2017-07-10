#ifndef DFISE_READ_H
#define DFISE_READ_H

#include <map>
#include <vector>

// Sections to read in DF-ISE file
enum class DFSection {
    NONE = 0,
    IGNORED,
    HEADER,
    INFO,
    REGION,
    COORDINATES,
    VERTICES,
    EDGES,
    FACES,
    ELEMENTS,

    ELECTRIC_FIELD,
    VALUES
};

// Simple point class to store data
class Point {
public:
    Point() : x(0), y(0), z(0) {}
    Point(double px, double py, double pz) : x(px), y(py), z(pz) {}

    double x, y, z;
};

// Read the grid
std::map<std::string, std::vector<Point>> read_grid(std::string file_name);

// Read the electric field
std::map<std::string, std::vector<Point>> read_electric_field(std::string file_name);

#endif
