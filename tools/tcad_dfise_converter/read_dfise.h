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
    ELECTROSTATIC_POTENTIAL,
    VALUES
};

// Simple point class to store data
class Point {
public:
    Point() = default;
    Point(double px, double py) : x(px), y(py) {}
    Point(double px, double py, double pz) : x(px), y(py), z(pz) {}

    double x{0}, y{0}, z{0};
};

// Read the grid
std::map<std::string, std::vector<Point>> read_grid(const std::string& file_name);

// Read the electric field
std::map<std::string, std::map<std::string, std::vector<Point>>> read_electric_field(const std::string& file_name);

#endif
