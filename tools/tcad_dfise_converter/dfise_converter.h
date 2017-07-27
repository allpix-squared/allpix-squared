#include <utility>
#include <vector>

#include "read_dfise.h"

// Interrupt handler
void interrupt_handler(int);

// Main function to execute converter
int main(int argc, char** argv);

// Interpolation of a single vertex
std::pair<Point, bool> barycentric_interpolation(Point query_point,
                                                 std::vector<Point> tetra_vertices,
                                                 std::vector<Point> tetra_vertices_field,
                                                 double tetra_volume);
