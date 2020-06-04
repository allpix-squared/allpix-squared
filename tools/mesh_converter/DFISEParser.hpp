#ifndef DFISE_READ_H
#define DFISE_READ_H

#include <iostream>
#include <map>
#include <vector>

namespace mesh_converter {

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

        DONOR_CONCENTRATION,
        DOPING_CONCENTRATION,
        ACCEPTOR_CONCENTRATION,
        ELECTRIC_FIELD,
        ELECTROSTATIC_POTENTIAL,
        VALUES
    };

    // Simple point class to store data
    class Point {
    public:
        Point() noexcept = default;
        Point(double px, double py, double pz = 0) noexcept : x(px), y(py), z(pz) {}

        double x{0}, y{0}, z{0};

        friend std::ostream& operator<<(std::ostream& out, const Point& pt) {
            out << "(" << pt.x << "," << pt.y << "," << pt.z << ")";
            return out;
        }
    };

    // Read the grid
    std::map<std::string, std::vector<Point>> read_grid(const std::string& file_name, bool mesh_tree);

    // Read the electric field
    std::map<std::string, std::map<std::string, std::vector<Point>>> read_electric_field(const std::string& file_name);
} // namespace mesh_converter

#endif
