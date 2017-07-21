#include "dfise_converter.h"

#include <algorithm>
#include <cfloat>
#include <chrono>
#include <climits>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <Eigen/Eigen>
#include "../../src/core/utils/log.h"
#include "Octree.hpp"
#include "read_dfise.h"

Point barycentric_interpolation(Point query_point,
                                std::vector<Point> tetra_vertices,
                                std::vector<Point> tetra_vertices_field,
                                double tetra_volume) {

    allpix::Log::addStream(std::cout);

    // Algorithm variables
    bool volume_signal;
    bool sub_1_signal, sub_2_signal, sub_3_signal, sub_4_signal;
    Eigen::Matrix4d matrix_sub1, matrix_sub2, matrix_sub3, matrix_sub4;
    double tetra_subvol_1, tetra_subvol_2, tetra_subvol_3, tetra_subvol_4;
    // Return variable. Point(interpolated electric field x, (...) y, (...) z)
    Point efield_int;

    // Function must have tetra_vertices.size() = 4
    if(tetra_vertices.size() != 4) {
        throw std::invalid_argument("Baricentric interpolation without only 4 vertices!");
    }

    if(tetra_volume > 0) {
        volume_signal = true;
    }
    if(tetra_volume < 0) {
        volume_signal = false;
    }

    // Jacobi Matrix for volume calculation
    matrix_sub1 << 1, 1, 1, 1, query_point.x, tetra_vertices[1].x, tetra_vertices[2].x, tetra_vertices[3].x, query_point.y,
        tetra_vertices[1].y, tetra_vertices[2].y, tetra_vertices[3].y, query_point.z, tetra_vertices[1].z,
        tetra_vertices[2].z, tetra_vertices[3].z;
    tetra_subvol_1 = (matrix_sub1.determinant()) / 6;
    if(tetra_subvol_1 > 0) {
        sub_1_signal = true;
    }
    if(tetra_subvol_1 < 0) {
        sub_1_signal = false;
    }
    if(tetra_subvol_1 == 0) {
        sub_1_signal = volume_signal;
    }

    matrix_sub2 << 1, 1, 1, 1, tetra_vertices[0].x, query_point.x, tetra_vertices[2].x, tetra_vertices[3].x,
        tetra_vertices[0].y, query_point.y, tetra_vertices[2].y, tetra_vertices[3].y, tetra_vertices[0].z, query_point.z,
        tetra_vertices[2].z, tetra_vertices[3].z;
    tetra_subvol_2 = (matrix_sub2.determinant()) / 6;
    if(tetra_subvol_2 > 0) {
        sub_2_signal = true;
    }
    if(tetra_subvol_2 < 0) {
        sub_2_signal = false;
    }
    if(tetra_subvol_2 == 0) {
        sub_2_signal = volume_signal;
    }

    matrix_sub3 << 1, 1, 1, 1, tetra_vertices[0].x, tetra_vertices[1].x, query_point.x, tetra_vertices[3].x,
        tetra_vertices[0].y, tetra_vertices[1].y, query_point.y, tetra_vertices[3].y, tetra_vertices[0].z,
        tetra_vertices[1].z, query_point.z, tetra_vertices[3].z;
    tetra_subvol_3 = (matrix_sub3.determinant()) / 6;
    if(tetra_subvol_3 > 0) {
        sub_3_signal = true;
    }
    if(tetra_subvol_3 < 0) {
        sub_3_signal = false;
    }
    if(tetra_subvol_3 == 0) {
        sub_3_signal = volume_signal;
    }

    matrix_sub1 << 1, 1, 1, 1, tetra_vertices[0].x, tetra_vertices[1].x, tetra_vertices[2].x, query_point.x,
        tetra_vertices[0].y, tetra_vertices[1].y, tetra_vertices[2].y, query_point.y, tetra_vertices[0].z,
        tetra_vertices[1].z, tetra_vertices[2].z, query_point.z;
    tetra_subvol_4 = (matrix_sub1.determinant()) / 6;
    if(tetra_subvol_4 > 0) {
        sub_4_signal = true;
    }
    if(tetra_subvol_4 < 0) {
        sub_4_signal = false;
    }
    if(tetra_subvol_4 == 0) {
        sub_4_signal = volume_signal;
    }

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

    for(size_t i = 0; i < tetra_vertices.size(); i++) {
        auto distance = unibn::L2Distance<Point>::compute(tetra_vertices[i], query_point);
        LOG(DEBUG) << "Tetrahedron vertex "
                   << "	(" << tetra_vertices[i].x << ", " << tetra_vertices[i].y << ", " << tetra_vertices[i].z << ").	"
                   << "	Distance: " << distance << "	Electric field:	(" << tetra_vertices_field[i].x << ", "
                   << tetra_vertices_field[i].y << ", " << tetra_vertices_field[i].z << ").";
    }
    LOG(DEBUG) << "Tetra full volume:	 " << tetra_volume;
    LOG(DEBUG) << "Tetra sub volume 1:	 " << tetra_subvol_1;
    LOG(DEBUG) << "Tetra sub volume 2:	 " << tetra_subvol_2;
    LOG(DEBUG) << "Tetra sub volume 3:	 " << tetra_subvol_3;
    LOG(DEBUG) << "Tetra sub volume 4:	 " << tetra_subvol_4;
    LOG(DEBUG) << "Volume difference:	 "
               << tetra_volume - (tetra_subvol_1 + tetra_subvol_2 + tetra_subvol_3 + tetra_subvol_4);
    LOG(DEBUG) << " ===> Electric field:	" << efield_int.x << " x, " << efield_int.y << " y, " << efield_int.z << " z";

    // Check if query point is outside tetrahedron
    if(sub_1_signal != volume_signal || sub_2_signal != volume_signal || sub_3_signal != volume_signal ||
       sub_4_signal != volume_signal) {
        throw std::invalid_argument("Point outside tetrahedron");
    }

    allpix::Log::finish();
    return efield_int;
}

int main(int argc, char** argv) {
    // If no arguments are provided, print the help:
    bool print_help = false;
    int return_code = 0;
    if(argc == 1) {
        print_help = true;
        return_code = 1;
    }

    allpix::Log::addStream(std::cout);

    std::string file_prefix = "example_pixel";
    std::string log_file_name;
    std::string region = "bulk"; // Sensor bulk region name on DF-ISE file
    float radius_step = 0.5;     // Neighbour vertex search radius
    float max_radius = 10;       // Neighbour vertex search radius
    float radius = 1;            // Neighbour vertex search radius
    int xdiv = 100;              // New mesh X pitch
    int ydiv = 100;              // New mesh Y pitch
    int zdiv = 100;              // New mesh Z pitch

    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-h") == 0) {
            print_help = true;
        } else if(strcmp(argv[i], "-v") == 0 && (i + 1 < argc)) {
            try {
                allpix::LogLevel log_level = allpix::Log::getLevelFromString(std::string(argv[++i]));
                allpix::Log::setReportingLevel(log_level);
            } catch(std::invalid_argument& e) {
                LOG(ERROR) << "Invalid verbosity level \"" << std::string(argv[i]) << "\", ignoring overwrite";
            }
        } else if(strcmp(argv[i], "-f") == 0 && (i + 1 < argc)) {
            file_prefix = std::string(argv[++i]);
        } else if(strcmp(argv[i], "-R") == 0 && (i + 1 < argc)) {
            region = std::string(argv[++i]); // Region to be meshed
        } else if(strcmp(argv[i], "-r") == 0 && (i + 1 < argc)) {
            radius = static_cast<float>(strtod(argv[++i], nullptr)); // New mesh X pitch
        } else if(strcmp(argv[i], "-s") == 0 && (i + 1 < argc)) {
            radius_step = static_cast<float>(strtod(argv[++i], nullptr)); // New mesh X pitch
        } else if(strcmp(argv[i], "-m") == 0 && (i + 1 < argc)) {
            max_radius = static_cast<float>(strtod(argv[++i], nullptr)); // New mesh X pitch
        } else if(strcmp(argv[i], "-x") == 0 && (i + 1 < argc)) {
            xdiv = static_cast<int>(strtod(argv[++i], nullptr)); // New mesh X pitch
        } else if(strcmp(argv[i], "-y") == 0 && (i + 1 < argc)) {
            ydiv = static_cast<int>(strtod(argv[++i], nullptr)); // New mesh X pitch
        } else if(strcmp(argv[i], "-z") == 0 && (i + 1 < argc)) {
            zdiv = static_cast<int>(strtod(argv[++i], nullptr)); // New mesh X pitch
        } else if(strcmp(argv[i], "-l") == 0 && (i + 1 < argc)) {
            log_file_name = std::string(argv[++i]);
        } else {
            LOG(ERROR) << "Unrecognized command line argument \"" << argv[i] << "\"";
        }
    }

    // Print help if requested or no arguments given
    if(print_help) {
        std::cerr << "Usage: ./tcad_dfise_reader -f <data_file_prefix> [<options>]" << std::endl;
        std::cout << "\t -f <file_prefix>	DF-ISE files prefix" << std::endl;
        std::cout << "\t -R <region>		region name to be meshed" << std::endl;
        std::cout << "\t -r <radius>		initial node neighbors search radius" << std::endl;
        std::cout << "\t -r <radius_step>	radius step if no neighbor is found" << std::endl;
        std::cout << "\t -m <max_radius>	maximum search radius" << std::endl;
        std::cout << "\t -x <mesh x_pitch>	new regular mesh X pitch" << std::endl;
        std::cout << "\t -y <mesh_y_pitch>	new regular mesh Y pitch" << std::endl;
        std::cout << "\t -z <mesh_z_pitch>	new regular mesh Z pitch" << std::endl;
        std::cout << "\t -l <file>    		file to log to besides standard output" << std::endl;
        allpix::Log::finish();
        return return_code;
    }

    // NOTE: this stream should be available for the duration of the logging
    std::ofstream log_file;
    if(!log_file_name.empty()) {
        log_file.open(log_file_name, std::ios_base::out | std::ios_base::trunc);
        if(!log_file.good()) {
            LOG(FATAL) << "Cannot write to provided log file! Check if permissions are sufficient.";
            allpix::Log::finish();
            return 1;
        }
        allpix::Log::addStream(log_file);
    }

    auto start = std::chrono::system_clock::now();

    LOG(TRACE) << "Reading grid";
    std::string grid_file = file_prefix + ".grd";

    std::vector<Point> points;
    try {
        auto region_grid = read_grid(grid_file);
        points = region_grid[region];
    } catch(std::runtime_error& e) {
        LOG(ERROR) << "Failed to parse grid file " << grid_file;
        LOG(ERROR) << "ERROR: " << e.what();
        return 1;
    }

    LOG(TRACE) << "Reading electric field";
    std::string data_file = file_prefix + ".dat";
    std::vector<Point> field;
    try {
        auto region_fields = read_electric_field(data_file);
        field = region_fields[region];
    } catch(std::runtime_error& e) {
        LOG(ERROR) << "Failed to parse data file " << data_file;
        LOG(ERROR) << "ERROR: " << e.what();
        return 1;
    }

    if(points.size() != field.size()) {
        LOG(ERROR) << "Field and grid do not match";
        return 1;
    }

    /* ALERT fix coordinates */
    for(unsigned int i = 0; i < points.size(); ++i) {
        std::swap(points[i].y, points[i].z);
        std::swap(field[i].y, field[i].z);
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

    /*
     * ALERT invert the z-axis to match the ap2 system
     * WARNING this will remove the right-handedness of the coordinate system!
     */
    for(size_t i = 0; i < points.size(); ++i) {
        points[i].z = maxz - (points[i].z - minz);
        field[i].z = -field[i].z;
    }

    auto end = std::chrono::system_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
    LOG(INFO) << "Reading the files took " << elapsed_seconds << " seconds.";

    LOG(TRACE) << "Starting meshing";
    // Initializing the Octree with points from mesh cloud.
    unibn::Octree<Point> octree;
    octree.initialize(points);

    // Creating a new mesh points cloud with a regular pitch
    std::vector<Point> e_field_new_mesh;
    double xstep = (maxx - minx) / static_cast<double>(xdiv);
    double ystep = (maxy - miny) / static_cast<double>(ydiv);
    double zstep = (maxz - minz) / static_cast<double>(zdiv);

    double x = minx + xstep / 2.0;
    for(int i = 0; i < xdiv; ++i) {
        double y = miny + ystep / 2.0;
        for(int j = 0; j < ydiv; ++j) {
            double z = minz + zstep / 2.0;
            for(int k = 0; k < xdiv; ++k) {
                Point q(x, y, z); // New mesh vertex
                Point e(x, y, z); // Corresponding, to be interpolated, electric field

                // Finding the nearest neighbours points
                std::vector<long unsigned int> results_size;
                std::vector<unsigned int> results;
                unsigned int ball = 0;
                while(radius < max_radius) {
                    // Calling octree neighbours search function and sorting the results list with the closest neighbours
                    // first
                    octree.radiusNeighbors<unibn::L2Distance<Point>>(q, radius, results);
                    std::sort(results.begin(), results.end(), [&](unsigned int a, unsigned int b) {
                        return unibn::L2Distance<Point>::compute(points[a], q) <
                               unibn::L2Distance<Point>::compute(points[b], q);
                    });

                    if(results.empty()) {
                        LOG(WARNING) << "At vertex (" << x << ", " << y << ", " << z << ")";
                        LOG(WARNING) << "Radius too Small. No neighbours found for radius " << radius;
                        radius = radius + radius_step;
                        continue;
                    }

                    if(results.size() < 4) {
                        LOG(WARNING) << "At vertex (" << x << ", " << y << ", " << z << ")";
                        LOG(WARNING) << "Incomplete mesh element found for radius " << radius;
                        radius = radius + radius_step;
                        continue;
                    }

                    // If after a radius step no new neighbours are found, go to the next radius step
                    results_size.push_back(results.size());
                    if(results_size.size() > 1) {
                        if(results_size.at(ball) == results_size.at(ball - 1)) {
                            LOG(WARNING) << "No new neighbour after radius step. Going to next step.";
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
                                    if(i1 == i2 || i1 == i3 || i1 == i4 || i2 == i3 || i2 == i4 || i3 == i4) {
                                        continue;
                                    }
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
                                        LOG(DEBUG)
                                            << " ===> Interpolating point (" << q.x << "," << q.y << "," << q.z << ")";
                                        LOG(DEBUG) << "Search radius:	" << radius;
                                        LOG(DEBUG) << "Number of vertices found:	" << results.size();
                                        LOG(DEBUG) << "Parsing neighbors [index]:	" << i1 << ", " << i2 << ", " << i3
                                                   << ", " << i4;
                                        try {
                                            e = barycentric_interpolation(q, tetra_vertices, tetra_vertices_field, volume);
                                        } catch(std::invalid_argument& exception) {
                                            LOG(WARNING) << "Failed to interpolate point";
                                            LOG(WARNING) << "ERROR: " << exception.what();
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
                                if(tetra_vertices.size() == 4) {
                                    break;
                                }
                            } // end for 3
                            if(tetra_vertices.size() == 2) {
                                tetra_vertices.pop_back();
                                tetra_vertices_field.pop_back();
                                vertex_index.pop_back();
                                vertex_distance.pop_back();
                            }
                            if(tetra_vertices.size() == 4) {
                                break;
                            }
                        } // end for 2
                        if(tetra_vertices.size() == 1) {
                            tetra_vertices.pop_back();
                            tetra_vertices_field.pop_back();
                            vertex_index.pop_back();
                            vertex_distance.pop_back();
                        }
                        if(tetra_vertices.size() == 4) {
                            break;
                        }
                    } // end for 1

                    if(tetra_vertices.size() == 4) {
                        break;
                    }
                    radius = radius + radius_step;
                } // end while

                e_field_new_mesh.push_back(e);
                z += zstep;
            }
            y += ystep;
        }
        x += xstep;
    }

    std::ofstream init_file;
    std::stringstream init_file_name;
    init_file_name << file_prefix << "_" << xdiv << "x" << ydiv << "x" << zdiv << ".txt";
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

    end = std::chrono::system_clock::now();
    elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
    LOG(INFO) << "Running everything took " << elapsed_seconds << " seconds.";

    allpix::Log::finish();
    return 1;
}
