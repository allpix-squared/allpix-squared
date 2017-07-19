#include "dfise_converter.h"

#include <algorithm>
#include <cfloat>
#include <climits>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <Eigen/Eigen>
#include "Octree.hpp"
#include "read_dfise.h"

Point barycentric_interpolation(Point query_point,
                                std::vector<Point> tetra_vertices,
                                std::vector<Point> tetra_vertices_field,
                                double tetra_volume) {
    /* DEBUGGING call with prints.
    Point barycentric_interpolation(Point query_point, std::vector<Point>&tetra_vertices,
                            std::vector<Point> tetra_vertices_field, std::vector<int> vertex_index,
                    std::vector<double> vertex_distance, double tetra_volume,
                    double search_radius, int number_of_vertices){
    //*/

    // Algorithm variables
    bool volume_signal;
    bool sub_1_signal, sub_2_signal, sub_3_signal, sub_4_signal;
    Eigen::Matrix4d matrix_sub1, matrix_sub2, matrix_sub3, matrix_sub4;
    double tetra_subvol_1, tetra_subvol_2, tetra_subvol_3, tetra_subvol_4;
    // Return variable. Point(interpolated electric field x, (...) y, (...) z)
    Point efield_int;

    // Function must have tetra_vertices.size() = 4
    if(tetra_vertices.size() != 4) {
        throw "Baricentric interpolation without only 4 vertices!";
    }

    if(tetra_volume > 0)
        volume_signal = true;
    if(tetra_volume < 0)
        volume_signal = false;

    // Jacobi Matrix for volume calculation
    matrix_sub1 << 1, 1, 1, 1, query_point.x, tetra_vertices[1].x, tetra_vertices[2].x, tetra_vertices[3].x, query_point.y,
        tetra_vertices[1].y, tetra_vertices[2].y, tetra_vertices[3].y, query_point.z, tetra_vertices[1].z,
        tetra_vertices[2].z, tetra_vertices[3].z;
    tetra_subvol_1 = (matrix_sub1.determinant()) / 6;
    if(tetra_subvol_1 > 0)
        sub_1_signal = true;
    if(tetra_subvol_1 < 0)
        sub_1_signal = false;
    if(tetra_subvol_1 == 0)
        sub_1_signal = volume_signal;

    matrix_sub2 << 1, 1, 1, 1, tetra_vertices[0].x, query_point.x, tetra_vertices[2].x, tetra_vertices[3].x,
        tetra_vertices[0].y, query_point.y, tetra_vertices[2].y, tetra_vertices[3].y, tetra_vertices[0].z, query_point.z,
        tetra_vertices[2].z, tetra_vertices[3].z;
    tetra_subvol_2 = (matrix_sub2.determinant()) / 6;
    if(tetra_subvol_2 > 0)
        sub_2_signal = true;
    if(tetra_subvol_2 < 0)
        sub_2_signal = false;
    if(tetra_subvol_2 == 0)
        sub_2_signal = volume_signal;

    matrix_sub3 << 1, 1, 1, 1, tetra_vertices[0].x, tetra_vertices[1].x, query_point.x, tetra_vertices[3].x,
        tetra_vertices[0].y, tetra_vertices[1].y, query_point.y, tetra_vertices[3].y, tetra_vertices[0].z,
        tetra_vertices[1].z, query_point.z, tetra_vertices[3].z;
    tetra_subvol_3 = (matrix_sub3.determinant()) / 6;
    if(tetra_subvol_3 > 0)
        sub_3_signal = true;
    if(tetra_subvol_3 < 0)
        sub_3_signal = false;
    if(tetra_subvol_3 == 0)
        sub_3_signal = volume_signal;

    matrix_sub1 << 1, 1, 1, 1, tetra_vertices[0].x, tetra_vertices[1].x, tetra_vertices[2].x, query_point.x,
        tetra_vertices[0].y, tetra_vertices[1].y, tetra_vertices[2].y, query_point.y, tetra_vertices[0].z,
        tetra_vertices[1].z, tetra_vertices[2].z, query_point.z;
    tetra_subvol_4 = (matrix_sub1.determinant()) / 6;
    if(tetra_subvol_4 > 0)
        sub_4_signal = true;
    if(tetra_subvol_4 < 0)
        sub_4_signal = false;
    if(tetra_subvol_4 == 0)
        sub_4_signal = volume_signal;

    // Electric field interpolation
    efield_int.x = (tetra_subvol_1 * tetra_vertices_field[0].x + tetra_subvol_2 * tetra_vertices_field[1].x +
                    tetra_subvol_3 * tetra_vertices_field[2].x + tetra_subvol_4 * tetra_vertices_field[3].x) /
                   tetra_volume;
    efield_int.y = (tetra_subvol_1 * tetra_vertices_field[0].y + tetra_subvol_2 * tetra_vertices_field[1].y +
                    tetra_subvol_3 * tetra_vertices_field[2].y + tetra_subvol_4 * tetra_vertices_field[3].y) /
                   tetra_volume;
    efield_int.z = (tetra_subvol_1 * tetra_vertices_field[0].z + tetra_subvol_2 * tetra_vertices_field[1].z +
                    tetra_subvol_3 * tetra_vertices_field[2].z + tetra_subvol_4 * tetra_vertices_field[3].z) /
                   tetra_volume;

    /* DEBUGGING prints (deactivate removing first slash)
    std::cout << " ===> Interpolating point q(" << query_point.x << "," << query_point.y << "," << query_point.z << ")" <<
    std::endl;
    std::cout << "Search radius:	" << search_radius << std::endl;
    std::cout << "Number of vertices:	" << number_of_vertices << std::endl;
    std::cout << "Neighbor vertices:	" << tetra_vertices.size() << std::endl;
    for (int i=0; i< tetra_vertices.size(); i++){
        std::cout << "Vertex index:	" << vertex_index[i] <<
            "	(" << tetra_vertices[i].x << ", " <<  tetra_vertices[i].y << ", " << tetra_vertices[i].z << ").	" <<
            "	Distance: " << 		    vertex_distance[i] <<
            "	Electric field:	(" <<
            tetra_vertices_field[i].x << ", " <<  tetra_vertices_field[i].y << ", " << tetra_vertices_field[i].z << ")." <<
    std::endl;
    }
    std::cout << "Tetra full volume:	 " << tetra_volume << std::endl;
    std::cout << "Tetra sub volume 1:	 " << tetra_subvol_1 << std::endl;
    std::cout << "Tetra sub volume 2:	 " << tetra_subvol_2 << std::endl;
    std::cout << "Tetra sub volume 3:	 " << tetra_subvol_3 << std::endl;
    std::cout << "Tetra sub volume 4:	 " << tetra_subvol_4 << std::endl;
    std::cout << "Volume difference:	 " << tetra_volume - (tetra_subvol_1+tetra_subvol_2+tetra_subvol_3+tetra_subvol_4) <<
    std::endl;
    std::cout << " ===> Electric field:	" << efield_int.x << " x, " << efield_int.y << " y, " << efield_int.z << " z" <<
    std::endl << std::endl;
    //*/

    // Check if query point is outside tetrahedron
    if(sub_1_signal != volume_signal || sub_2_signal != volume_signal || sub_3_signal != volume_signal ||
       sub_4_signal != volume_signal) {
        throw std::exception();
    }

    return efield_int;
}

int main(int argc, char** argv) {
    int xdiv = 100; // New mesh X pitch
    int ydiv = 100; // New mesh Y pitch
    int zdiv = 100; // New mesh Z pitch
    if(argc < 2) {
        std::cerr << "Usage: ./tcad_dfise_reader <data_file_prefix> [x_pitch y_pitch z_pitch]" << std::endl;
        std::cerr << "If the pitch is not defined, default value = 100 will be used" << std::endl;
        return -1;
    }
    std::string filename = argv[1];
    if(argc > 2) {
        xdiv = atoi(argv[2]); // New mesh X pitch
        ydiv = atoi(argv[3]); // New mesh Y pitch
        zdiv = atoi(argv[4]); // New mesh Z pitch
    }

    auto region = "bulk"; // Sensor bulk region name on DF-ISE file
    float radius = 1;     // Neighbour vertex search radius

    clock_t begin, end;
    begin = clock();

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

    /* ALERT fix coordinates */
    for(unsigned int i = 0; i < points.size(); ++i) {
        std::swap(points[i].y, points[i].z);
        std::swap(field[i].y, field[i].z);

        points[i].y -= 14;
    }

    // Find minimum and maximum from mesh coordinates
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

    end = clock();
    double search_time = (static_cast<double>(end - begin) / CLOCKS_PER_SEC);
    std::cerr << "Reading the files took " << search_time << " seconds." << std::endl;

    // Initializing the Octree with points from mesh cloud.
    unibn::Octree<Point> octree;
    octree.initialize(points);

    // Creating a new mesh points cloud with a regular pitch
    std::vector<Point> e_field_new_mesh;
    for(double x = minx + (maxx - minx) / (2.0 * xdiv); x <= maxx; x += (maxx - minx) / xdiv) {
        for(double y = miny + (maxy - miny) / (2.0 * ydiv); y <= maxy; y += (maxy - miny) / ydiv) {
            for(double z = minz + (maxz - minz) / (2.0 * zdiv); z <= maxz; z += (maxz - minz) / zdiv) {
                Point q(x, y, z); // New mesh vertex
                Point e(x, y, z); // Corresponding, to be interpolated, electric field

                // Finding the nearest neighbours points
                std::vector<long unsigned int> results_size;
                std::vector<unsigned int> results;
                unsigned int ball = 0;
                while(radius < 5.5f) {
                    // Calling octree neighbours search function and sorting the results list with the closest neighbours
                    // first
                    octree.radiusNeighbors<unibn::L2Distance<Point>>(q, radius, results);
                    std::sort(results.begin(), results.end(), [&](unsigned int a, unsigned int b) {
                        return unibn::L2Distance<Point>::compute(points[a], q) <
                               unibn::L2Distance<Point>::compute(points[b], q);
                    });

                    if(results.empty() || results.size() < 4) {
                        std::cerr << "At vertex (" << x << ", " << y << ", " << z << ")" << std::endl;
                        std::cerr << "Radius too Small. No neighbours found for radius " << radius << std::endl;
                        continue;
                    }

                    // If after a radius step no new neighbours are found, go to the next radius step
                    results_size.push_back(results.size());
                    if(results_size.size() > 1) {
                        if(results_size.at(ball) == results_size.at(ball - 1)) {
                            std::cerr << "No new neighbour after radius step. Going to next step." << std::endl;
                            ball++;
                            continue;
                        }
                    }
                    ball++;

                    // Finding tetrahedrons
                    Eigen::Matrix4d matrix;
                    double volume;
                    std::vector<Point> tetra_vertices;
                    std::vector<Point> tetra_vertices_field;
                    std::vector<unsigned int> vertex_index;
                    std::vector<double> vertex_distance;

                    // Loop over possible combinations of tetrahedron points
                    unsigned int i1 = 0, i2 = 0, i3 = 0, i4 = 0;
                    for(i1 = 0; i1 < results.size(); i1++) {
                        tetra_vertices.push_back(points[results[i1]]);
                        tetra_vertices_field.push_back(field[results[i1]]);
                        vertex_index.push_back(i1);
                        vertex_distance.push_back(unibn::L2Distance<Point>::compute(points[results[i1]], q));
                        for(i2 = 1; i2 < results.size(); i2++) {
                            tetra_vertices.push_back(points[results[i2]]);
                            tetra_vertices_field.push_back(field[results[i2]]);
                            vertex_index.push_back(i2);
                            vertex_distance.push_back(unibn::L2Distance<Point>::compute(points[results[i2]], q));
                            for(i3 = 2; i3 < results.size(); i3++) {
                                tetra_vertices.push_back(points[results[i3]]);
                                tetra_vertices_field.push_back(field[results[i3]]);
                                vertex_index.push_back(i3);
                                vertex_distance.push_back(unibn::L2Distance<Point>::compute(points[results[i3]], q));
                                for(i4 = 3; i4 < results.size(); i4++) {
                                    if(i1 == i2 || i1 == i3 || i1 == i4 || i2 == i3 || i2 == i4 || i3 == i4)
                                        continue;
                                    matrix << 1, 1, 1, 1, points[results[i1]].x, points[results[i2]].x,
                                        points[results[i3]].x, points[results[i4]].x, points[results[i1]].y,
                                        points[results[i2]].y, points[results[i3]].y, points[results[i4]].y,
                                        points[results[i1]].z, points[results[i2]].z, points[results[i3]].z,
                                        points[results[i4]].z;
                                    volume = (matrix.determinant()) / 6;
                                    if(std::abs(volume) > 0.00001) {
                                        tetra_vertices.push_back(points[results[i4]]);
                                        tetra_vertices_field.push_back(field[results[i4]]);
                                        vertex_index.push_back(i4);
                                        vertex_distance.push_back(unibn::L2Distance<Point>::compute(points[results[i4]], q));
                                        try {
                                            e = barycentric_interpolation(q, tetra_vertices, tetra_vertices_field, volume);
                                            // e = barycentric_interpolation(q, tetra_vertices, tetra_vertices_field,
                                            // vertex_index, vertex_distance, volume, radius, results.size());
                                        } catch(std::exception) {
                                            tetra_vertices.pop_back();
                                            tetra_vertices_field.pop_back();
                                            vertex_index.pop_back();
                                            vertex_distance.pop_back();
                                            continue;
                                        }
                                        break;
                                    }
                                } // end for 4
                                if(tetra_vertices.size() == 3) {
                                    tetra_vertices.pop_back();
                                    tetra_vertices_field.pop_back();
                                    vertex_index.pop_back();
                                    vertex_distance.pop_back();
                                }
                                if(tetra_vertices.size() == 4)
                                    break;
                            } // end for 3
                            if(tetra_vertices.size() == 2) {
                                tetra_vertices.pop_back();
                                tetra_vertices_field.pop_back();
                                vertex_index.pop_back();
                                vertex_distance.pop_back();
                            }
                            if(tetra_vertices.size() == 4)
                                break;
                        } // end for 2
                        if(tetra_vertices.size() == 1) {
                            tetra_vertices.pop_back();
                            tetra_vertices_field.pop_back();
                            vertex_index.pop_back();
                            vertex_distance.pop_back();
                        }
                        if(tetra_vertices.size() == 4)
                            break;
                    } // end for 1

                    if(tetra_vertices.size() == 4)
                        break;
                    radius = radius + 0.5f;
                } // end while

                e_field_new_mesh.push_back(e);
            }
        }
    }

    std::ofstream init_file;
    std::stringstream init_file_name;
    init_file_name << filename << "_" << xdiv << "x" << ydiv << "x" << zdiv << ".txt";
    init_file.open(init_file_name.str());
    // Write INIT file h"eader
    init_file << "tcad_octree_writer" << std::endl;                                    // NAME
    init_file << "##SEED## ##EVENTS##" << std::endl;                                   // UNUSED
    init_file << "##TURN## ##TILT## 1.0" << std::endl;                                 // UNUSED
    init_file << "0.0 0.0 0.0" << std::endl;                                           // MAGNETIC FIELD (UNUSED)
    init_file << (maxz - minz) << " " << (maxx - minx) << " " << (maxy - miny) << " "; // PIXEL DIMENSIONS
    init_file << "0.0 0.0 0.0 0.0 ";                                                   // UNUSED
    init_file << xdiv << " " << ydiv << " " << zdiv << " ";
    init_file << "0.0" << std::endl; // UNUSED

    // Write INIT file data
    for(int i = 0; i < xdiv; ++i) {
        for(int j = 0; j < ydiv; ++j) {
            for(int k = 0; k < zdiv; ++k) {
                auto& point = e_field_new_mesh[static_cast<unsigned int>(i * ydiv * zdiv + j * zdiv + k)];
                init_file << i + 1 << " " << j + 1 << " " << k + 1 << " " << point.x << " " << point.y << " " << point.z
                          << std::endl;
            }
        }
    }
    init_file.close();

    end = clock();
    search_time = (static_cast<double>(end - begin) / CLOCKS_PER_SEC);
    std::cerr << "Running everything took " << search_time << " seconds." << std::endl;
}
