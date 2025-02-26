/**
 * @file
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

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

#include <Math/Vector3D.h>
#include <TTree.h>

#include <Eigen/Eigen>

#include "core/config/ConfigReader.hpp"
#include "core/config/Configuration.hpp"
#include "core/config/exceptions.h"
#include "core/module/ThreadPool.hpp"
#include "core/utils/log.h"
#include "core/utils/unit.h"
#include "tools/ROOT.h"
#include "tools/field_parser.h"
#include "tools/units.h"

#include "MeshElement.hpp"
#include "MeshParser.hpp"
#include "combinations/combinations.h"
#include "octree/Octree.hpp"

using namespace mesh_converter;
using namespace ROOT::Math;
using allpix::Log;
using allpix::ThreadPool;
using allpix::Units;

void interrupt_handler(int);

/**
 * @brief Handle termination request (CTRL+C)
 */
void interrupt_handler(int) {
    LOG(STATUS) << "Interrupted! Aborting conversion...";
    Log::finish();
    std::exit(0);
}

int main(int argc, char** argv) {
    using XYZVectorUInt = DisplacementVector3D<Cartesian3D<unsigned int>>;
    using XYVectorUInt = DisplacementVector2D<Cartesian2D<unsigned int>>;
    using FileType = allpix::FileType;
    using FieldQuantity = allpix::FieldQuantity;

    int return_code = 0;

    // Register the default set of units with this executable:
    allpix::register_units();

    // Add stream and set default logging level
    Log::addStream(std::cout);

    // Install abort handler (CTRL+\) and interrupt handler (CTRL+C)
    std::signal(SIGQUIT, interrupt_handler);
    std::signal(SIGINT, interrupt_handler);

    // If no arguments are provided, print the help:
    bool print_help = false;
    if(argc == 1) {
        print_help = true;
        return_code = 1;
    }

    std::string file_prefix;
    std::string init_file_prefix;
    std::string log_file_name;

    std::string conf_file_name;

    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-h") == 0) {
            print_help = true;
        } else if(strcmp(argv[i], "-v") == 0 && (i + 1 < argc)) {
            try {
                auto log_level = Log::getLevelFromString(std::string(argv[++i]));
                Log::setReportingLevel(log_level);
            } catch(std::invalid_argument& e) {
                LOG(ERROR) << "Invalid verbosity level \"" << std::string(argv[i]) << "\", ignoring overwrite";
                return_code = 1;
            }
        } else if(strcmp(argv[i], "-f") == 0 && (i + 1 < argc)) {
            file_prefix = std::string(argv[++i]);

            // Pre-fill config file name if not set yet:
            if(conf_file_name.empty()) {
                conf_file_name = file_prefix + ".conf";
            }
        } else if(strcmp(argv[i], "-c") == 0 && (i + 1 < argc)) {
            conf_file_name = std::string(argv[++i]);
        } else if(strcmp(argv[i], "-o") == 0 && (i + 1 < argc)) {
            init_file_prefix = std::string(argv[++i]);
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
        std::cerr << "Usage: mesh_converter -f <file_name> [<options>]" << std::endl;
        std::cout << "Required parameters:" << std::endl;
        std::cout << "\t -f <file_prefix>  common prefix of DF-ISE grid (.grd) and data (.dat) files" << std::endl;
        std::cout << "Optional parameters:" << std::endl;
        std::cout << "\t -c <config_file>  configuration file name" << std::endl;
        std::cout << "\t -h                display this help text" << std::endl;
        std::cout << "\t -l <file>         file to log to besides standard output (disabled by default)" << std::endl;
        std::cout
            << "\t -o <file_prefix>  output file prefix without .init extension (defaults to file name of <file_prefix>)"
            << std::endl;
        std::cout << "\t -v <level>        verbosity level (default reporiting level is INFO)" << std::endl;

        Log::finish();
        return return_code;
    }

    // NOTE: this stream should be available for the duration of the logging
    std::ofstream log_file;
    if(!log_file_name.empty()) {
        log_file.open(log_file_name, std::ios_base::out | std::ios_base::trunc);
        if(!log_file.good()) {
            LOG(FATAL) << "Cannot write to provided log file! Check if permissions are sufficient.";
            Log::finish();
            return 1;
        }
        Log::addStream(log_file);
    }

    try {
        std::ifstream file(conf_file_name);
        allpix::ConfigReader reader(file, conf_file_name);
        allpix::Configuration config = reader.getHeaderConfiguration();

        auto log_level = Log::getReportingLevel();
        if(log_level == allpix::LogLevel::NONE) {
            auto log_level_string = config.get<std::string>("log_level", "INFO");
            std::transform(log_level_string.begin(), log_level_string.end(), log_level_string.begin(), ::toupper);
            try {
                log_level = Log::getLevelFromString(log_level_string);
                Log::setReportingLevel(log_level);
            } catch(std::invalid_argument& e) {
                LOG(ERROR) << "Log level \"" << log_level_string
                           << "\" specified in the configuration is invalid, defaulting to INFO instead";
                Log::setReportingLevel(allpix::LogLevel::INFO);
            }
        }

        LOG(STATUS) << "Welcome to the Mesh Converter Tool of Allpix^2 " << ALLPIX_PROJECT_VERSION;
        LOG(STATUS) << "Using " << conf_file_name << " configuration file";

        // Output file format:
        auto format = config.get<std::string>("model", "apf");
        std::transform(format.begin(), format.end(), format.begin(), ::tolower);
        FileType file_type = (format == "init" ? FileType::INIT : format == "apf" ? FileType::APF : FileType::UNKNOWN);
        if(file_type == FileType::UNKNOWN) {
            throw allpix::InvalidValueError(config, "model", "only models 'apf' and 'init' are currently supported");
        }

        // Input file parser:
        auto parser = MeshParser::factory(config);

        // Region, observable and binning of output field
        auto regions = config.getArray<std::string>("region");
        auto observable = config.get<std::string>("observable");
        const auto units = config.get<std::string>("observable_units");
        // Test if this unit is valid:
        try {
            (void)Units::get(units);
        } catch(std::invalid_argument& e) {
            throw allpix::InvalidValueError(config, "observable_units", e.what());
        }

        const auto vector_field = config.get<bool>("vector_field", (observable == "ElectricField"));

        const auto interpolate = config.get<bool>("interpolate", true);
        const auto allow_decay = config.get<bool>("allow_coplanar_interpolation", false);
        const auto radius_step = config.get<double>("radius_step", 0.5);
        const auto volume_cut = config.get<double>("volume_cut", 10e-9);

        // Swapping elements
        auto rot = config.getArray<std::string>("xyz", {"x", "y", "z"});
        if(rot.size() != 3) {
            throw allpix::InvalidValueError(config, "xyz", "Three entries required");
        }

        auto start = std::chrono::system_clock::now();

        std::string grid_file = file_prefix + ".grd";
        std::vector<Point> points = parser->getMesh(grid_file, regions);

        // Obtain number of mesh dimensions from mesh point:
        XYZVectorUInt divisions;
        const auto dimension = points.front().dim;
        LOG(STATUS) << "Determined dimensionality of input mesh to be " << dimension << "D";
        if(dimension == 2) {
            auto divisions_yz = config.get<XYVectorUInt>("divisions", XYVectorUInt(100, 100));
            divisions = XYZVectorUInt(1, divisions_yz.x(), divisions_yz.y());
        } else if(dimension == 3) {
            divisions = config.get<XYZVectorUInt>("divisions", XYZVectorUInt(100, 100, 100));
        }

        // If we are looking at a 2D mesh, we force x to be the coordinate out of range:
        if(dimension == 2 && rot.at(0) != "-x" && rot.at(0) != "x") {
            throw allpix::InvalidValueError(config, "xyz", "For 2D meshes the first coordinate has to remain 'x'");
        }

        std::string data_file = file_prefix + ".dat";
        std::vector<Point> field = parser->getField(data_file, observable, regions);

        if(points.size() != field.size()) {
            throw std::runtime_error("Field and grid file do not match, found " + std::to_string(points.size()) + " and " +
                                     std::to_string(field.size()) + " data points, respectively.");
        }
        auto points_temp = points;
        auto field_temp = field;
        if(rot.at(0) == "-y" || rot.at(0) == "y") {
            for(size_t i = 0; i < points.size(); ++i) {
                points_temp[i].x = points[i].y;
                field_temp[i].x = field[i].y;
            }
        }
        if(rot.at(0) == "-z" || rot.at(0) == "z") {
            for(size_t i = 0; i < points.size(); ++i) {
                points_temp[i].x = points[i].z;
                field_temp[i].x = field[i].z;
            }
        }
        if(rot.at(1) == "-x" || rot.at(1) == "x") {
            for(size_t i = 0; i < points.size(); ++i) {
                points_temp[i].y = points[i].x;
                field_temp[i].y = field[i].x;
            }
        }
        if(rot.at(1) == "-z" || rot.at(1) == "z") {
            for(size_t i = 0; i < points.size(); ++i) {
                points_temp[i].y = points[i].z;
                field_temp[i].y = field[i].z;
            }
        }
        if(rot.at(2) == "-x" || rot.at(2) == "x") {
            for(size_t i = 0; i < points.size(); ++i) {
                points_temp[i].z = points[i].x;
                field_temp[i].z = field[i].x;
            }
        }
        if(rot.at(2) == "-y" || rot.at(2) == "y") {
            for(size_t i = 0; i < points.size(); ++i) {
                points_temp[i].z = points[i].y;
                field_temp[i].z = field[i].y;
            }
        }
        points = points_temp;
        field = field_temp;

        // Find minimum and maximum from mesh coordinates
        auto maxx =
            (dimension == 2 ? 1
                            : std::max_element(points.begin(), points.end(), [](auto& a, auto& b) { return a.x < b.x; })->x);
        auto minx = std::min_element(points.begin(), points.end(), [](auto& a, auto& b) { return a.x < b.x; })->x;
        auto maxy = std::max_element(points.begin(), points.end(), [](auto& a, auto& b) { return a.y < b.y; })->y;
        auto miny = std::min_element(points.begin(), points.end(), [](auto& a, auto& b) { return a.y < b.y; })->y;
        auto maxz = std::max_element(points.begin(), points.end(), [](auto& a, auto& b) { return a.z < b.z; })->z;
        auto minz = std::min_element(points.begin(), points.end(), [](auto& a, auto& b) { return a.z < b.z; })->z;

        // Creating a new mesh points cloud with a regular pitch
        const double xstep = (maxx - minx) / static_cast<double>(divisions.x());
        const double ystep = (maxy - miny) / static_cast<double>(divisions.y());
        const double zstep = (maxz - minz) / static_cast<double>(divisions.z());
        const double cell_volume = xstep * ystep * zstep;

        // Using the minimal cell dimension as initial search radius for the point cloud:
        const auto initial_radius = config.get<double>("initial_radius", std::min({xstep, ystep, zstep}));
        const auto max_radius = config.get<double>("max_radius", std::max(initial_radius, 50.));
        LOG(INFO) << "Using initial neighbor search radius of " << initial_radius << " and maximum search radius of "
                  << max_radius;
        const auto allow_failed_interpolation = config.get<bool>("allow_failure", false);

        if(rot.at(0) != "x" || rot.at(1) != "y" || rot.at(2) != "z") {
            LOG(STATUS) << "TCAD mesh (x,y,z) coords. transformation into: (" << rot.at(0) << "," << rot.at(1) << ","
                        << rot.at(2) << ")";
        }

        const auto mesh_points_total = divisions.x() * divisions.y() * divisions.z();
        LOG(STATUS) << "Mesh dimensions: " << maxx - minx << " x " << maxy - miny << " x " << maxz - minz << std::endl
                    << "New mesh element dimension: " << xstep << " x " << ystep << " x " << zstep << std::endl
                    << "Volume: " << cell_volume << std::endl
                    << "New mesh grid points: " << static_cast<ROOT::Math::XYZVector>(divisions) << " (" << mesh_points_total
                    << " total)";

        if(rot.at(0).find('-') != std::string::npos) {
            LOG(WARNING) << "Inverting coordinate X. This might change the right-handness of the coordinate system!";
            for(size_t i = 0; i < points.size(); ++i) {
                points[i].x = maxx - (points[i].x - minx);
                field[i].x = -field[i].x;
            }
        }
        if(rot.at(1).find('-') != std::string::npos) {
            LOG(WARNING) << "Inverting coordinate Y. This might change the right-handness of the coordinate system!";
            for(size_t i = 0; i < points.size(); ++i) {
                points[i].y = maxy - (points[i].y - miny);
                field[i].y = -field[i].y;
            }
        }
        if(rot.at(2).find('-') != std::string::npos) {
            LOG(WARNING) << "Inverting coordinate Z. This might change the right-handness of the coordinate system!";
            for(size_t i = 0; i < points.size(); ++i) {
                points[i].z = maxz - (points[i].z - minz);
                field[i].z = -field[i].z;
            }
        }

        auto end = std::chrono::system_clock::now();
        auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
        LOG(INFO) << "Reading the files took " << elapsed_seconds << " seconds.";

        // Initializing the Octree with points from mesh cloud.
        unibn::Octree<Point> octree;
        octree.initialize(points);

        unsigned int mesh_points_done = 0;
        auto mesh_section = [&](double x, double y) {
            Log::setReportingLevel(log_level);

            // New mesh slice
            std::vector<Point> new_mesh;

            double z = minz + zstep / 2.0;
            for(unsigned int k = 0; k < divisions.z(); ++k) {
                // New mesh vertex and field
                auto q = (dimension == 2 ? Point(y, z) : Point(x, y, z));
                Point e;

                // No interpolation requested, return nearest neighbor:
                if(!interpolate) {
                    auto idx = static_cast<size_t>(octree.findNeighbor<unibn::L2Distance<Point>>(q));
                    new_mesh.push_back(field.at(idx));
                    z += zstep;
                    continue;
                }

                bool valid = false;
                bool allow_zero_volume = false;
                size_t prev_neighbours = 0;
                double radius = initial_radius;

                while(radius <= max_radius) {
                    LOG(DEBUG) << "Search radius: " << radius;
                    // Calling octree neighbours search and sorting the results list with the closest neighbours first
                    std::vector<unsigned int> results;
                    octree.radiusNeighbors<unibn::L2Distance<Point>>(q, radius, results);
                    LOG(DEBUG) << "Number of vertices found: " << results.size();

                    if(radius == initial_radius && results.size() > 100) {
                        LOG(WARNING) << "Found " << results.size() << " mesh vertices within initial search radius of "
                                     << initial_radius << "um." << std::endl
                                     << "This might indicate that the output mesh granularity is too low and field features "
                                        "might be missed."
                                     << std::endl
                                     << "Consider increasing output mesh granularity.";
                    }

                    // If after a radius step no new neighbours are found, go to the next radius step
                    if(results.size() <= prev_neighbours || results.empty()) {
                        prev_neighbours = results.size();
                        LOG(DEBUG) << "No (new) neighbour found with radius " << radius << ". Increasing search radius.";
                        radius = radius + radius_step;
                        continue;
                    }

                    // If we have less than N close neighbors, no full mesh element can be formed. Increase radius.
                    if(results.size() < (dimension == 3 ? 4 : 3)) {
                        LOG(DEBUG) << "Incomplete mesh element found for radius " << radius << ", increasing radius";
                        radius = radius + radius_step;
                        continue;
                    }

                    // If we have too many neighbors, we could decay to using lower-dimension interpolation:
                    if(allow_decay && radius > initial_radius && results.size() > 100) {
                        LOG_ONCE(WARNING) << "Large number of neighbors found, this hints to a quasi-co"
                                          << (dimension == 3 ? "planar" : "linear") << " situation" << std::endl
                                          << "Decaying to interpolation in " << (dimension == 3 ? "planar" : "linear")
                                          << " space for affected points";
                        allow_zero_volume = true;
                    }

                    // Sort by lowest distance first, this drastically reduces the number of permutations required to find a
                    // valid mesh element and also ensures that this is the one with the smallest volume.
                    std::sort(results.begin(), results.end(), [&](unsigned int a, unsigned int b) {
                        return unibn::L2Distance<Point>::compute(points[a], q) <
                               unibn::L2Distance<Point>::compute(points[b], q);
                    });

                    // Finding tetrahedrons by checking all combinations of N elements, starting with closest to reference
                    // point
                    auto res = for_each_combination(results.begin(),
                                                    results.begin() + (dimension == 3 ? 4 : 3),
                                                    results.end(),
                                                    Combination(&points, &field, q, allow_zero_volume ? 0 : volume_cut));
                    valid = res.valid();
                    if(valid) {
                        e = res.result();
                        break;
                    }

                    radius = radius + radius_step;
                    LOG(DEBUG) << "All combinations tried. Increasing search radius to " << radius;
                }

                if(!valid) {
                    if(!allow_failed_interpolation) {
                        throw std::runtime_error(
                            "Could not find valid volume element. Consider to increase max_radius to include "
                            "more mesh points in the search or to allow failed interpolation with allow_failure "
                            "to set the element to zero.");
                    }

                    LOG(DEBUG) << "Failed to interpolate, setting element to zero";
                    e = {};
                }

                new_mesh.push_back(e);
                z += zstep;
            }

            mesh_points_done += divisions.z();
            LOG_PROGRESS(STATUS, "m") << (interpolate ? "Interpolating" : "Generating") << " new mesh: " << mesh_points_done
                                      << " of " << mesh_points_total << ", "
                                      << (mesh_points_done / (mesh_points_total / 100)) << "%";

            return new_mesh;
        };

        // Start the interpolation on many threads:
        auto num_threads = config.get<unsigned int>("workers", std::max(std::thread::hardware_concurrency(), 1u));
        ThreadPool::registerThreadCount(num_threads);
        LOG(STATUS) << "Starting regular grid interpolation with " << num_threads << " threads.";
        std::vector<Point> e_field_new_mesh;

        // clang-format off
        auto init_function = [log_level = Log::getReportingLevel(), log_format = Log::getFormat()]() {
            // clang-format on
            // Initialize the threads to the same log level and format as the master setting
            Log::setReportingLevel(log_level);
            Log::setFormat(log_format);
        };

        ThreadPool pool(num_threads, num_threads * 1024, init_function);
        std::vector<std::shared_future<std::vector<Point>>> mesh_futures;
        // Set starting point
        double x = minx + xstep / 2.0;
        // Loop over x coordinate, add tasks for each coordinate to the queue
        for(unsigned int i = 0; i < divisions.x(); ++i) {
            double y = miny + ystep / 2.0;
            for(unsigned int j = 0; j < divisions.y(); ++j) {
                mesh_futures.push_back(pool.submit(mesh_section, x, y));
                y += ystep;
            }
            x += xstep;
        }

        // Merge the result vectors:
        for(auto& mesh_future : mesh_futures) {
            auto mesh_slice = mesh_future.get();
            e_field_new_mesh.insert(e_field_new_mesh.end(), mesh_slice.begin(), mesh_slice.end());
        }
        pool.destroy();

        end = std::chrono::system_clock::now();
        elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
        LOG(INFO) << "New mesh created in " << elapsed_seconds << " seconds.";

        // Prepare header and auxiliary information:
        std::string header =
            "Allpix Squared " + std::string(ALLPIX_PROJECT_VERSION) + " TCAD Mesh Converter, observable: " + observable;
        std::array<double, 3> size{
            {Units::get(maxx - minx, "um"), Units::get(maxy - miny, "um"), Units::get(maxz - minz, "um")}};
        std::array<size_t, 3> gridsize{
            {static_cast<size_t>(divisions.x()), static_cast<size_t>(divisions.y()), static_cast<size_t>(divisions.z())}};

        FieldQuantity quantity = (vector_field ? FieldQuantity::VECTOR : FieldQuantity::SCALAR);
        // Prepare data:
        LOG(INFO) << "Preparing data for storage...";
        auto data = std::make_shared<std::vector<double>>();
        for(unsigned int i = 0; i < divisions.x(); ++i) {
            for(unsigned int j = 0; j < divisions.y(); ++j) {
                for(unsigned int k = 0; k < divisions.z(); ++k) {
                    auto& point = e_field_new_mesh[i * divisions.y() * divisions.z() + j * divisions.z() + k];
                    LOG(DEBUG) << "Values of data point (" << i << ", " << j << ", " << k << "): " << point;
                    // For a vector field, we push three values:
                    if(quantity == FieldQuantity::VECTOR) {
                        // We need to convert to framework-internal units:
                        data->push_back(Units::get(point.x, units));
                        data->push_back(Units::get(point.y, units));
                        data->push_back(Units::get(point.z, units));
                    } else {
                        // For a scalar field, we need to push only one value, but which one depends on the field rotation.
                        // We need the original x-position, as that is the only filled one in the parsed field
                        if(rot.at(1) == "-x" || rot.at(1) == "x") {
                            data->push_back(Units::get(point.y, units));
                        } else if(rot.at(2) == "-x" || rot.at(2) == "x") {
                            data->push_back(Units::get(point.z, units));
                        } else {
                            data->push_back(Units::get(point.x, units));
                        }
                    }
                }
            }
        }

        allpix::FieldData<double> field_data(header, gridsize, size, data);
        std::string init_file_name = init_file_prefix + "_" + observable + (file_type == FileType::INIT ? ".init" : ".apf");

        allpix::FieldWriter<double> field_writer(quantity);
        field_writer.writeFile(field_data, init_file_name, file_type, (file_type == FileType::INIT ? units : ""));
        LOG(STATUS) << "New mesh written to file \"" << init_file_name << "\"";

        end = std::chrono::system_clock::now();
        elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
        LOG(STATUS) << "Interpolation and conversion completed in " << elapsed_seconds << " seconds.";

    } catch(allpix::ConfigurationError& e) {
        LOG(FATAL) << "Error in the configuration:" << std::endl
                   << e.what() << std::endl
                   << "The configuration needs to be updated. Cannot continue.";
        return_code = 1;
    } catch(std::exception& e) {
        LOG(FATAL) << "Fatal internal error" << std::endl << e.what() << std::endl << "Cannot continue.";
        return_code = 127;
    }

    // Finish the logging
    Log::finish();
    return return_code;
}
