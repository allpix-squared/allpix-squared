#include "read_dfise.h"
// Main function to execute converter
int main(int argc, char** argv);
std::pair<Point, bool> barycentric_interpolation(Point query_point,
                                                 std::vector<Point> tetra_vertices,
                                                 std::vector<Point> tetra_vertices_field,
                                                 double tetra_volume);
