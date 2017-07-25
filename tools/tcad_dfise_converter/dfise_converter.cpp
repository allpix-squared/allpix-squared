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
#include <utility>

#include <Eigen/Eigen>
#include "../../src/core/utils/log.h"
#include "Octree.hpp"
#include "read_dfise.h"

std::pair<Point, bool> barycentric_interpolation(Point query_point,
                                                 std::vector<Point> tetra_vertices,
                                                 std::vector<Point> tetra_vertices_field,
                                                 double tetra_volume) {

    // Algorithm variables
    bool volume_signal;
    bool sub_1_signal, sub_2_signal, sub_3_signal, sub_4_signal;
    Eigen::Matrix4d matrix_sub1, matrix_sub2, matrix_sub3, matrix_sub4;
    double tetra_subvol_1, tetra_subvol_2, tetra_subvol_3, tetra_subvol_4;
    // Return variable. Point(interpolated electric field x, (...) y, (...) z)
    Point efield_int;
    bool flag = true;
    std::pair<Point, bool> efield_valid;

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
    LOG(DEBUG) << "Tetra full volume:	 " << tetra_volume << std::endl
               << "Tetra sub volume 1:	 " << tetra_subvol_1 << std::endl
               << "Tetra sub volume 2:	 " << tetra_subvol_2 << std::endl
               << "Tetra sub volume 3:	 " << tetra_subvol_3 << std::endl
               << "Tetra sub volume 4:	 " << tetra_subvol_4 << std::endl
               << "Volume difference:	 "
               << tetra_volume - (tetra_subvol_1 + tetra_subvol_2 + tetra_subvol_3 + tetra_subvol_4);

    // Check if query point is outside tetrahedron
    if(sub_1_signal != volume_signal || sub_2_signal != volume_signal || sub_3_signal != volume_signal ||
       sub_4_signal != volume_signal) {
        flag = false;
        LOG(WARNING) << "Warning: Point outside tetrahedron";
        efield_valid = std::make_pair(efield_int, flag);
        return efield_valid;
    }

    LOG(INFO) << "Interpolated electric field:	" << efield_int.x << " x, " << efield_int.y << " y, " << efield_int.z << " z"
              << std::endl;
    efield_valid = std::make_pair(efield_int, flag);
    return efield_valid;
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

    std::string file_prefix;
    std::string init_file_prefix;
    std::string log_file_name;
    std::string region = "bulk";                            // Sensor bulk region name on DF-ISE file
    double volume_cut = std::numeric_limits<double>::min(); // Neighbour vertex search radius
    size_t index_cut = std::numeric_limits<size_t>::max();
    float radius_step = 0.5;    // Neighbour vertex search radius
    float max_radius = 10;      // Neighbour vertex search radius
    float initial_radius = 0.5; // Neighbour vertex search radius
    int xdiv = 100;             // New mesh X pitch
    int ydiv = 100;             // New mesh Y pitch
    int zdiv = 100;             // New mesh Z pitch

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
            init_file_prefix = file_prefix;
        } else if(strcmp(argv[i], "-o") == 0 && (i + 1 < argc)) {
            init_file_prefix = std::string(argv[++i]);
        } else if(strcmp(argv[i], "-R") == 0 && (i + 1 < argc)) {
            region = std::string(argv[++i]); // Region to be meshed
        } else if(strcmp(argv[i], "-r") == 0 && (i + 1 < argc)) {
            initial_radius = static_cast<float>(strtod(argv[++i], nullptr)); // New mesh X pitch
        } else if(strcmp(argv[i], "-s") == 0 && (i + 1 < argc)) {
            radius_step = static_cast<float>(strtod(argv[++i], nullptr)); // New mesh X pitch
        } else if(strcmp(argv[i], "-m") == 0 && (i + 1 < argc)) {
            max_radius = static_cast<float>(strtod(argv[++i], nullptr)); // New mesh X pitch
        } else if(strcmp(argv[i], "-i") == 0 && (i + 1 < argc)) {
            index_cut = static_cast<size_t>(strtod(argv[++i], nullptr)); // New mesh X pitch
        } else if(strcmp(argv[i], "-c") == 0 && (i + 1 < argc)) {
            volume_cut = static_cast<float>(strtod(argv[++i], nullptr)); // New mesh X pitch
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
        std::cout << "\t -f <file_prefix>		DF-ISE files prefix" << std::endl;
        std::cout << "\t -o <output_file_prefix>	Init output file prefix" << std::endl;
        std::cout << "\t -R <region>			region name to be meshed" << std::endl;
        std::cout << "\t -r <radius>			initial node neighbors search radius" << std::endl;
        std::cout << "\t -r <radius_step>		radius step if no neighbor is found" << std::endl;
        std::cout << "\t -m <max_radius>		maximum search radius" << std::endl;
        std::cout << "\t -i <index_cut>			index cut during permutation on vertex neighbours" << std::endl;
        std::cout << "\t -c <volume_cut>		minimum volume for tetrahedron (non-coplanar vertices)" << std::endl;
        std::cout << "\t -x <mesh x_pitch>		new regular mesh X pitch" << std::endl;
        std::cout << "\t -y <mesh_y_pitch>		new regular mesh Y pitch" << std::endl;
        std::cout << "\t -z <mesh_z_pitch>		new regular mesh Z pitch" << std::endl;
        std::cout << "\t -l <file>    			file to log to besides standard output" << std::endl;
        std::cout << "\t -v <level>			verbosity level overwrites global log level,\n"
                  << "\t  			        but not the per-module configuration." << std::endl;

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
    LOG(TRACE) << "Reading the files took " << elapsed_seconds << " seconds.";

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
            for(int k = 0; k < zdiv; ++k) {
                Point q(x, y, z); // New mesh vertex
                Point e(x, y, z); // Corresponding, to be interpolated, electric field
                bool flag = false;
                std::pair<Point, bool> return_interpolation;

                LOG(INFO) << "===> Interpolating point " << i + 1 << " " << j + 1 << " " << k + 1 << "	(" << q.x << ","
                          << q.y << "," << q.z << ")";

                size_t prev_neighbours = 0;
                float radius = initial_radius;
                while(radius < max_radius) {
                    LOG(DEBUG) << "Search radius:	" << radius;
                    // Calling octree neighbours search function and sorting the results list with the closest neighbours
                    // first
                    std::vector<unsigned int> results;
                    octree.radiusNeighbors<unibn::L2Distance<Point>>(q, radius, results);
                    std::sort(results.begin(), results.end(), [&](unsigned int a, unsigned int b) {
                        return unibn::L2Distance<Point>::compute(points[a], q) <
                               unibn::L2Distance<Point>::compute(points[b], q);
                    });
                    LOG(DEBUG) << "Number of vertices found:	" << results.size();

                    if(results.empty()) {
                        LOG(WARNING) << "At vertex (" << x << ", " << y << ", " << z << ")" << std::endl
                                     << "Radius too Small. No neighbours found for radius " << radius << std::endl
                                     << "Increasing the readius";
                        radius = radius + radius_step;
                        continue;
                    }

                    if(results.size() < 4) {
                        LOG(WARNING) << "At vertex (" << x << ", " << y << ", " << z << ")" << std::endl
                                     << "Incomplete mesh element found for radius " << radius << std::endl
                                     << "Increasing the readius";
                        radius = radius + radius_step;
                        continue;
                    }

                    // If after a radius step no new neighbours are found, go to the next radius step
                    if(results.size() > prev_neighbours) {
                        prev_neighbours = results.size();
                    } else {
                        LOG(WARNING) << "At vertex (" << x << ", " << y << ", " << z << ")" << std::endl
                                     << "No new neighbour after radius step. Going to next step.";
                        radius = radius + radius_step;
                        continue;
                    }

                    // Finding tetrahedrons
                    double volume;
                    Eigen::Matrix4d matrix;
                    size_t num_nodes_element = 4;
                    std::vector<Point> tetra_vertices;
                    std::vector<Point> tetra_vertices_field;

                    std::vector<int> bitmask(num_nodes_element, 1);
                    bitmask.resize(results.size(), 0);
                    std::vector<size_t> index;

                    do {
                        index.clear();
                        tetra_vertices.clear();
                        tetra_vertices_field.clear();
                        // print integers and permute bitmask
                        for(size_t idk = 0; idk < results.size(); ++idk) {
                            if(bitmask[idk]) {
                                index.push_back(idk);
                                tetra_vertices.push_back(points[results[idk]]);
                                tetra_vertices_field.push_back(field[results[idk]]);
                            }
                            if(index.size() == 4)
                                break;
                        }
                        if(index[0] > index_cut || index[1] > index_cut || index[2] > index_cut || index[3] > index_cut) {
                            LOG(WARNING) << "Applying index cut on the " << index_cut << "-nth neighbour on a total of "
                                         << results.size();
                            continue;
                        }
                        LOG(DEBUG) << "Parsing neighbors [index]:	" << index[0] << ", " << index[1] << ", " << index[2]
                                   << ", " << index[3];

                        matrix << 1, 1, 1, 1, points[results[index[0]]].x, points[results[index[1]]].x,
                            points[results[index[2]]].x, points[results[index[3]]].x, points[results[index[0]]].y,
                            points[results[index[1]]].y, points[results[index[2]]].y, points[results[index[3]]].y,
                            points[results[index[0]]].z, points[results[index[1]]].z, points[results[index[2]]].z,
                            points[results[index[3]]].z;
                        volume = (matrix.determinant()) / 6;

                        if(abs(volume) <= volume_cut) {
                            LOG(WARNING) << "Coplanar vertices. Going to the next vertex combination.";
                            continue;
                        } else {
                            try {
                                return_interpolation =
                                    barycentric_interpolation(q, tetra_vertices, tetra_vertices_field, volume);
                                e = return_interpolation.first;
                                flag = return_interpolation.second;
                            } catch(std::invalid_argument& exception) {
                                LOG(WARNING) << "Failed to interpolate point: " << exception.what();
                                continue;
                            }
                            if(flag == false) {
                                continue;
                            }
                            break;
                        }
                    } while(std::prev_permutation(bitmask.begin(), bitmask.end()));

                    if(tetra_vertices.size() == 4 && flag == true) {
                        break;
                    }

                    LOG(WARNING) << "All combinations tried. Increasing the radius.";
                    radius = radius + radius_step;
                }

                if(flag == false) {
                    LOG(FATAL) << "Couldn't interpolate new mesh point. For now, let's stop the program and think a bit.";
                    return -1;
                }

                e_field_new_mesh.push_back(e);
                z += zstep;
            }
            y += ystep;
        }
        x += xstep;
    }

    end = std::chrono::system_clock::now();
    elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
    LOG(TRACE) << "New mesh created in " << elapsed_seconds << " seconds.";

    std::ofstream init_file;
    std::stringstream init_file_name;
    init_file_name << init_file_prefix << "_" << xdiv << "x" << ydiv << "x" << zdiv << ".init";
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
