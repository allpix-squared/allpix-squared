#include "dfise_converter.h"

#include <algorithm>
#include <cfloat>
#include <chrono>
#include <climits>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

#include <Eigen/Eigen>

#include "Octree.hpp"
#include "read_dfise.h"
#include "utils/log.h"

using namespace mesh_converter;

void interrupt_handler(int) {
    LOG(STATUS) << "Interrupted! Aborting conversion...";
    allpix::Log::finish();
    std::exit(0);
}

void MeshElement::setVertices(std::vector<Point>& new_vertices) {
    if(vertices.size() != new_vertices.size()) {
        LOG(ERROR) << "Invalid vertices vector";
        return;
    }
    for(size_t index = 0; index < new_vertices.size(); index++) {
        vertices[index] = new_vertices[index];
    }
}

void MeshElement::setVertex(size_t index, Point& new_vertice) {
    vertices[index] = new_vertice;
}

Point MeshElement::getVertex(size_t index) {
    return vertices[index];
}

void MeshElement::setVerticesField(std::vector<Point>& new_observable) {
    if(vertices.size() != new_observable.size()) {
        LOG(ERROR) << "Invalid field vector";
        return;
    }
    for(size_t index = 0; index < 4; index++) {
        e_field[index] = new_observable[index];
    }
}

void MeshElement::setVertexField(size_t index, Point& new_observable) {
    e_field[index] = new_observable;
}

Point MeshElement::getVertexProperty(size_t index) {
    return e_field[index];
}

void MeshElement::setDimension(int dimension) {
    _dimension = dimension;
}

int MeshElement::getDimension() {
    return _dimension;
}

double MeshElement::getVolume() {
    double volume = 0;
    if(this->getDimension() == 3) {
        Eigen::Matrix4d element_matrix;
        element_matrix << 1, 1, 1, 1, vertices[0].x, vertices[1].x, vertices[2].x, vertices[3].x, vertices[0].y,
            vertices[1].y, vertices[2].y, vertices[3].y, vertices[0].z, vertices[1].z, vertices[2].z, vertices[3].z;
        volume = (element_matrix.determinant()) / 6;
    }
    if(this->getDimension() == 2) {
        Eigen::Matrix3d element_matrix;
        element_matrix << 1, 1, 1, vertices[0].y, vertices[1].y, vertices[2].y, vertices[0].z, vertices[1].z, vertices[2].z;
        volume = (element_matrix.determinant()) / 2;
    }
    return volume;
}

double MeshElement::getDistance(size_t index, Point& qp) {
    return unibn::L2Distance<Point>::compute(vertices[index], qp);
}

bool MeshElement::validElement(double volume_cut, Point& qp) {
    if(this->getVolume() == 0) {
        LOG(TRACE) << "Invalid tetrahedron with coplanar(3D)/colinear(2D) vertices.";
        return false;
    }
    if(std::abs(this->getVolume()) <= volume_cut) {
        LOG(TRACE) << "Tetrahedron volume smaller than volume cut.";
        return false;
    }

    Eigen::Matrix4d sub_tetra_matrix;
    for(size_t i = 0; i < static_cast<size_t>(this->getDimension()) + 1; i++) {
        std::vector<Point> sub_vertices = vertices;
        sub_vertices[i] = qp;
        MeshElement sub_tetrahedron(sub_vertices);
        sub_tetrahedron.setDimension(this->getDimension());
        double tetra_volume = sub_tetrahedron.getVolume();
        if(this->getVolume() * tetra_volume >= 0) {
            continue;
        }
        if(this->getVolume() * tetra_volume < 0) {
            LOG(TRACE) << "New mesh Point outside found element.";
            return false;
        }
    }
    return true;
}

Point MeshElement::getObservable(Point& qp) {
    Point new_observable;
    Eigen::Matrix4d sub_tetra_matrix;
    for(size_t index = 0; index < static_cast<size_t>(this->getDimension()) + 1; index++) {
        auto sub_vertices = vertices;
        sub_vertices[index] = qp;
        MeshElement sub_tetrahedron(sub_vertices);
        sub_tetrahedron.setDimension(this->getDimension());
        double sub_volume = sub_tetrahedron.getVolume();
        LOG(DEBUG) << "Sub volume " << index << ": " << sub_volume;
        new_observable.x = new_observable.x + (sub_volume * e_field[index].x) / this->getVolume();
        new_observable.y = new_observable.y + (sub_volume * e_field[index].y) / this->getVolume();
        new_observable.z = new_observable.z + (sub_volume * e_field[index].z) / this->getVolume();
    }
    LOG(DEBUG) << "Interpolated electric field: (" << new_observable.x << "," << new_observable.y << "," << new_observable.z
               << ")" << std::endl;
    return new_observable;
}

void MeshElement::printElement(Point& qp) {
    for(size_t index = 0; index < static_cast<size_t>(this->getDimension()) + 1; index++) {
        LOG(DEBUG) << "Tetrahedron vertex " << index_vec[index] << " (" << vertices[index].x << ", " << vertices[index].y
                   << ", " << vertices[index].z << ") - "
                   << " Distance: " << this->getDistance(index, qp) << " - Electric field: (" << e_field[index].x << ", "
                   << e_field[index].y << ", " << e_field[index].z << ")";
    }
    LOG(DEBUG) << "Volume: " << this->getVolume();
}

int main(int argc, char** argv) {
    // If no arguments are provided, print the help:
    bool print_help = false;
    int return_code = 0;
    if(argc == 1) {
        print_help = true;
        return_code = 1;
    }

    // Add stream and set default logging level
    allpix::Log::addStream(std::cout);
    allpix::Log::setReportingLevel(allpix::LogLevel::INFO);

    // Install abort handler (CTRL+\) and interrupt handler (CTRL+C)
    std::signal(SIGQUIT, interrupt_handler);
    std::signal(SIGINT, interrupt_handler);

    std::string file_prefix;
    std::string init_file_prefix;
    std::string log_file_name;
    std::string region = "bulk";              // Sensor bulk region name on DF-ISE file
    std::string observable = "ElectricField"; // Sensor bulk region name on DF-ISE file
    double volume_cut = 1e-9;                 // Enclosing tetrahedron should have volume != 0
    size_t index_cut = 10000000;              // Permutation index initial cut
    bool index_cut_flag = false;
    double initial_radius = 1;   // Neighbour vertex search radius
    double radius_threshold = 0; // Neighbour vertex search radius
    bool threshold_flag = false;
    double radius_step = 0.5; // Search radius increment
    double max_radius = 10;   // Maximum search radiuss
    int dimension = 3;
    int xdiv = 100; // New mesh X pitch
    int ydiv = 100; // New mesh Y pitch
    int zdiv = 100; // New mesh Z pitch

    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-h") == 0) {
            print_help = true;
        } else if(strcmp(argv[i], "-v") == 0 && (i + 1 < argc)) {
            try {
                allpix::LogLevel log_level = allpix::Log::getLevelFromString(std::string(argv[++i]));
                allpix::Log::setReportingLevel(log_level);
            } catch(std::invalid_argument& e) {
                LOG(ERROR) << "Invalid verbosity level \"" << std::string(argv[i]) << "\", ignoring overwrite";
                return_code = 1;
            }
        } else if(strcmp(argv[i], "-f") == 0 && (i + 1 < argc)) {
            file_prefix = std::string(argv[++i]);
        } else if(strcmp(argv[i], "-o") == 0 && (i + 1 < argc)) {
            init_file_prefix = std::string(argv[++i]);
        } else if(strcmp(argv[i], "-R") == 0 && (i + 1 < argc)) {
            region = std::string(argv[++i]);
        } else if(strcmp(argv[i], "-O") == 0 && (i + 1 < argc)) {
            observable = std::string(argv[++i]);
        } else if(strcmp(argv[i], "-r") == 0 && (i + 1 < argc)) {
            initial_radius = strtod(argv[++i], nullptr);
        } else if(strcmp(argv[i], "-t") == 0 && (i + 1 < argc)) {
            radius_threshold = strtod(argv[++i], nullptr);
            threshold_flag = true;
        } else if(strcmp(argv[i], "-s") == 0 && (i + 1 < argc)) {
            radius_step = strtod(argv[++i], nullptr);
        } else if(strcmp(argv[i], "-m") == 0 && (i + 1 < argc)) {
            max_radius = strtod(argv[++i], nullptr);
        } else if(strcmp(argv[i], "-i") == 0 && (i + 1 < argc)) {
            index_cut = static_cast<size_t>(strtol(argv[++i], nullptr, 10));
            index_cut_flag = true;
        } else if(strcmp(argv[i], "-c") == 0 && (i + 1 < argc)) {
            volume_cut = strtod(argv[++i], nullptr);
        } else if(strcmp(argv[i], "-x") == 0 && (i + 1 < argc)) {
            xdiv = static_cast<int>(strtol(argv[++i], nullptr, 10));
        } else if(strcmp(argv[i], "-y") == 0 && (i + 1 < argc)) {
            ydiv = static_cast<int>(strtol(argv[++i], nullptr, 10));
        } else if(strcmp(argv[i], "-z") == 0 && (i + 1 < argc)) {
            zdiv = static_cast<int>(strtol(argv[++i], nullptr, 10));
        } else if(strcmp(argv[i], "-d") == 0 && (i + 1 < argc)) {
            dimension = static_cast<int>(strtol(argv[++i], nullptr, 10));
            xdiv = 1;
        } else if(strcmp(argv[i], "-l") == 0 && (i + 1 < argc)) {
            log_file_name = std::string(argv[++i]);
        } else {
            LOG(ERROR) << "Unrecognized command line argument or missing value \"" << argv[i] << "\"";
            print_help = true;
            return_code = 1;
        }
    }

    if(file_prefix.empty()) {
        print_help = true;
        return_code = 1;
    }

    if(init_file_prefix.empty()) {
        init_file_prefix = file_prefix;
        auto sep_idx = init_file_prefix.find_last_of('/');
        if(sep_idx != std::string::npos) {
            init_file_prefix = init_file_prefix.substr(sep_idx + 1);
        }
    }

    // Print help if requested or no arguments given
    if(print_help) {
        std::cerr << "Usage: ./tcad_dfise_reader -f <file_name> [<options>]" << std::endl;
        std::cout << "\t -f <file_prefix>       common prefix of DF-ISE grid (.grd) and data (.dat) files" << std::endl;
        std::cout << "\t -o <init_file_prefix>  output file prefix without .init (defaults to file name of <file_prefix>)"
                  << std::endl;
        std::cout << "\t -R <region>            region name to be meshed (defaults to 'bulk')" << std::endl;
        std::cout << "\t -O <observable>        observable to be interpolated (defaults Electric Field)" << std::endl;
        std::cout << "\t -r <radius>            initial node neighbors search radius in um (defaults to 1 um)" << std::endl;
        std::cout << "\t -t <radius_threshold>  minimum distance from node to new mesh point (defaults to 0 um)"
                  << std::endl;
        std::cout << "\t -s <radius_step>       radius step if no neighbor is found (defaults to 0.5 um)" << std::endl;
        std::cout << "\t -m <max_radius>        maximum search radius (default is 10 um)" << std::endl;
        std::cout << "\t -i <index_cut>         index cut during permutation on vertex neighbours (disabled by default)"
                  << std::endl;
        std::cout << "\t -c <volume_cut>        minimum volume for tetrahedron for non-coplanar vertices (defaults to "
                     "minimum double value)"
                  << std::endl;
        std::cout << "\t -x <mesh x_pitch>      new regular mesh X pitch (defaults to 100)" << std::endl;
        std::cout << "\t -y <mesh_y_pitch>      new regular mesh Y pitch (defaults to 100)" << std::endl;
        std::cout << "\t -z <mesh_z_pitch>      new regular mesh Z pitch (defaults to 100)" << std::endl;
        std::cout << "\t -d <mesh_dimension>    specify mesh dimensionality (defaults to 3)" << std::endl;
        std::cout << "\t -l <file>              file to log to besides standard output (disabled by default)" << std::endl;
        std::cout << "\t -v <level>             verbosity level (default reporiting level is INFO)" << std::endl;

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

    LOG(STATUS) << "Reading mesh grid from grid file";
    std::string grid_file = file_prefix + ".grd";

    std::vector<Point> points;
    try {
        auto region_grid = read_grid(grid_file);
        points = region_grid[region];
    } catch(std::runtime_error& e) {
        LOG(FATAL) << "Failed to parse grid file " << grid_file;
        LOG(FATAL) << " " << e.what();
        allpix::Log::finish();
        return 1;
    }

    LOG(STATUS) << "Reading electric field from data file";
    std::string data_file = file_prefix + ".dat";
    std::vector<Point> field;
    try {
        auto region_fields = read_electric_field(data_file);
        field = region_fields[region][observable];
    } catch(std::runtime_error& e) {
        LOG(FATAL) << "Failed to parse data file " << data_file;
        LOG(FATAL) << " " << e.what();
        allpix::Log::finish();
        return 1;
    }

    if(points.size() != field.size()) {
        LOG(FATAL) << "Field and grid file do not match";
        allpix::Log::finish();
        return 1;
    }

    /* ALERT fix coordinates */
    if(dimension == 3) {
        for(unsigned int i = 0; i < points.size(); ++i) {
            std::swap(points[i].y, points[i].z);
            std::swap(field[i].y, field[i].z);
        }
    }

    // Find minimum and maximum from mesh coordinates
    double minx = DBL_MAX, miny = DBL_MAX, minz = DBL_MAX;
    double maxx = DBL_MIN, maxy = DBL_MIN, maxz = DBL_MIN;
    for(auto& point : points) {
        if(dimension == 2) {
            maxx = 1;
            minx = 0;
            maxy = std::max(maxy, point.y);
            maxz = std::max(maxz, point.z);
            miny = std::min(miny, point.y);
            minz = std::min(minz, point.z);
        }

        if(dimension == 3) {
            maxx = std::max(maxx, point.x);
            maxy = std::max(maxy, point.y);
            maxz = std::max(maxz, point.z);
            minx = std::min(minx, point.x);
            miny = std::min(miny, point.y);
            minz = std::min(minz, point.z);
        }
    }

    // Creating a new mesh points cloud with a regular pitch
    double xstep = (maxx - minx) / static_cast<double>(xdiv);
    double ystep = (maxy - miny) / static_cast<double>(ydiv);
    double zstep = (maxz - minz) / static_cast<double>(zdiv);
    double cell_volume = xstep * ystep * zstep;

    LOG(STATUS) << "Mesh dimensions: " << maxx - minx << " x " << maxy - miny << " x " << maxz - minz << std::endl
                << "New mesh element dimension: " << xstep << " x " << ystep << " x " << zstep
                << " ==>  Volume = " << cell_volume;

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

    LOG(STATUS) << "Starting regular grid interpolation";
    // Initializing the Octree with points from mesh cloud.
    unibn::Octree<Point> octree;
    octree.initialize(points);
    std::vector<Point> e_field_new_mesh;

    double x = minx + xstep / 2.0;
    for(int i = 0; i < xdiv; ++i) {
        double y = miny + ystep / 2.0;
        for(int j = 0; j < ydiv; ++j) {
            double z = minz + zstep / 2.0;
            for(int k = 0; k < zdiv; ++k) {
                Point q, e;
                if(dimension == 2) {
                    q.x = -1;
                    q.y = y;
                    q.z = z; // New mesh vertex
                    e.x = -1;
                    e.y = y;
                    e.z = z;
                } else {
                    q.x = x;
                    q.y = y;
                    q.z = z; // New mesh vertex
                    e.x = x;
                    e.y = y;
                    e.z = z; // Corresponding, to be interpolated, electric field
                }
                bool valid = false;

                LOG_PROGRESS(INFO, "POINT") << "Interpolating point X=" << i + 1 << " Y=" << j + 1 << " Z=" << k + 1 << " ("
                                            << q.x << "," << q.y << "," << q.z << ")";

                size_t prev_neighbours = 0;
                double radius = initial_radius;
                size_t index_cut_up;
                while(radius < max_radius) {
                    LOG(DEBUG) << "Search radius: " << radius;
                    // Calling octree neighbours search and sorting the results list with the closest neighbours first
                    std::vector<unsigned int> results;
                    std::vector<unsigned int> results_high;
                    octree.radiusNeighbors<unibn::L2Distance<Point>>(q, radius, results_high);
                    std::sort(results_high.begin(), results_high.end(), [&](unsigned int a, unsigned int b) {
                        return unibn::L2Distance<Point>::compute(points[a], q) <
                               unibn::L2Distance<Point>::compute(points[b], q);
                    });

                    if(threshold_flag) {
                        size_t results_size = results_high.size();
                        int count = 0;
                        for(size_t index = 0; index < results_size; index++) {
                            if(unibn::L2Distance<Point>::compute(points[results_high[index]], q) < radius_threshold) {
                                count++;
                                continue;
                            }
                            results.push_back(results_high[index]);
                        }
                        LOG(DEBUG) << "Applying radius threshold of " << radius_threshold << std::endl
                                   << "Removing " << count << " of " << results_size;
                    } else {
                        results = results_high;
                    }

                    // If after a radius step no new neighbours are found, go to the next radius step
                    if(results.size() <= prev_neighbours || results.empty()) {
                        prev_neighbours = results.size();
                        LOG(WARNING) << "No (new) neighbour found with radius " << radius << ". Increasing search radius."
                                     << std::endl;
                        radius = radius + radius_step;
                        continue;
                    }

                    if(results.size() < 4) {
                        LOG(WARNING) << "Incomplete mesh element found for radius " << radius << std::endl
                                     << "Increasing the readius (setting a higher initial radius may help)";
                        radius = radius + radius_step;
                        continue;
                    }

                    LOG(DEBUG) << "Number of vertices found: " << results.size();

                    // Finding tetrahedrons
                    Eigen::Matrix4d matrix;
                    size_t num_nodes_element = 0;
                    if(dimension == 3) {
                        num_nodes_element = 4;
                    }
                    if(dimension == 2) {
                        num_nodes_element = 3;
                    }
                    std::vector<Point> element_vertices;
                    std::vector<Point> element_vertices_field;

                    std::vector<int> bitmask(num_nodes_element, 1);
                    bitmask.resize(results.size(), 0);
                    std::vector<size_t> index;

                    if(!index_cut_flag) {
                        index_cut = results.size();
                    }
                    index_cut_up = index_cut;
                    while(index_cut_up <= results.size()) {
                        do {
                            valid = false;
                            index.clear();
                            element_vertices.clear();
                            element_vertices_field.clear();
                            // print integers and permute bitmask
                            for(size_t idk = 0; idk < results.size(); ++idk) {
                                if(bitmask[idk] != 0) {
                                    index.push_back(idk);
                                    element_vertices.push_back(points[results[idk]]);
                                    element_vertices_field.push_back(field[results[idk]]);
                                }
                                if(index.size() == num_nodes_element) {
                                    break;
                                }
                            }

                            bool index_flag = false;
                            for(size_t ttt = 0; ttt < num_nodes_element; ttt++) {
                                if(index[ttt] > index_cut_up) {
                                    index_flag = true;
                                    break;
                                }
                            }
                            if(index_flag) {
                                continue;
                            }

                            if(dimension == 3) {
                                LOG(TRACE) << "Parsing neighbors [index]: " << index[0] << ", " << index[1] << ", "
                                           << index[2] << ", " << index[3];
                            }
                            if(dimension == 2) {
                                LOG(TRACE)
                                    << "Parsing neighbors [index]: " << index[0] << ", " << index[1] << ", " << index[2];
                            }

                            MeshElement element(dimension, index, element_vertices, element_vertices_field);
                            valid = element.validElement(volume_cut, q);
                            if(!valid) {
                                continue;
                            }
                            element.printElement(q);
                            e = element.getObservable(q);
                            break;
                        } while(std::prev_permutation(bitmask.begin(), bitmask.end()));

                        if(valid) {
                            break;
                        }

                        LOG(DEBUG) << "All combinations tried up to index " << index_cut_up
                                   << " done. Increasing the index cut.";
                        index_cut_up = index_cut_up + index_cut;
                    }

                    if(valid) {
                        break;
                    }

                    LOG(DEBUG) << "All combinations tried. Increasing the radius.";
                    radius = radius + radius_step;
                }

                if(!valid) {
                    LOG(FATAL) << "Couldn't interpolate new mesh point, probably the grid is too irregular";
                    return 1;
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
    LOG(INFO) << "New mesh created in " << elapsed_seconds << " seconds.";

    LOG(STATUS) << "Writing INIT file";

    std::ofstream init_file;
    std::stringstream init_file_name;
    init_file_name << init_file_prefix << "_" << observable << ".init";
    init_file.open(init_file_name.str());

    // Write INIT file h"eader
    init_file << "tcad_dfise_converter" << std::endl;                                  // NAME
    init_file << "#Observable: " << observable << std::endl;                           // OBSERVABLE INTERPOLATED
    init_file << "##SEED## ##EVENTS##" << std::endl;                                   // UNUSED
    init_file << "##TURN## ##TILT## 1.0" << std::endl;                                 // UNUSED
    init_file << "0.0 0.0 0.0" << std::endl;                                           // MAGNETIC FIELD (UNUSED)
    init_file << (maxz - minz) << " " << (maxx - minx) << " " << (maxy - miny) << " "; // PIXEL DIMENSIONS
    init_file << "0.0 0.0 0.0 0.0 ";                                                   // UNUSED
    init_file << xdiv << " " << ydiv << " " << zdiv << " ";                            // GRID SIZE
    init_file << "0.0" << std::endl;                                                   // UNUSED

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
    LOG(STATUS) << "Conversion completed in " << elapsed_seconds << " seconds.";

    allpix::Log::finish();
    return 1;
}
