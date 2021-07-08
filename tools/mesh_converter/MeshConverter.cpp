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
using allpix::ThreadPool;

void interrupt_handler(int);

/**
 * @brief Handle termination request (CTRL+C)
 */
void interrupt_handler(int) {
    LOG(STATUS) << "Interrupted! Aborting conversion...";
    allpix::Log::finish();
    std::exit(0);
}

int main(int argc, char** argv) {
    using XYZVectorInt = DisplacementVector3D<Cartesian3D<int>>;
    using XYVectorInt = DisplacementVector2D<Cartesian2D<int>>;
    using FileType = allpix::FileType;
    using FieldQuantity = allpix::FieldQuantity;

    int return_code = 0;

    try {

        // Register the default set of units with this executable:
        allpix::register_units();

        // If no arguments are provided, print the help:
        bool print_help = false;
        if(argc == 1) {
            print_help = true;
            return_code = 1;
        }

        // Add stream and set default logging level
        allpix::Log::addStream(std::cout);

        // Install abort handler (CTRL+\) and interrupt handler (CTRL+C)
        std::signal(SIGQUIT, interrupt_handler);
        std::signal(SIGINT, interrupt_handler);

        std::string file_prefix;
        std::string init_file_prefix;
        std::string log_file_name;

        std::string conf_file_name;
        allpix::LogLevel log_level = allpix::LogLevel::INFO;

        for(int i = 1; i < argc; i++) {
            if(strcmp(argv[i], "-h") == 0) {
                print_help = true;
            } else if(strcmp(argv[i], "-v") == 0 && (i + 1 < argc)) {
                try {
                    log_level = allpix::Log::getLevelFromString(std::string(argv[++i]));
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

        // Set log level:
        allpix::Log::setReportingLevel(log_level);

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

        LOG(STATUS) << "Welcome to the Mesh Converter Tool of Allpix^2 " << ALLPIX_PROJECT_VERSION;
        LOG(STATUS) << "Using " << conf_file_name << " configuration file";
        std::ifstream file(conf_file_name);
        allpix::ConfigReader reader(file, conf_file_name);
        allpix::Configuration config = reader.getHeaderConfiguration();

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
        auto regions = config.getArray<std::string>("region", {"bulk"});
        auto observable = config.get<std::string>("observable", "ElectricField");

        const auto radius_step = config.get<double>("radius_step", 0.5);
        const auto max_radius = config.get<double>("max_radius", 50);
        const auto volume_cut = config.get<double>("volume_cut", 10e-9);
        const auto units = config.get<std::string>("observable_units", "V/cm");
        const auto vector_field = config.get<bool>("vector_field", true);

        XYZVectorInt divisions;
        const auto dimension = config.get<size_t>("dimension", 3);
        if(dimension == 2) {
            auto divisions_yz = config.get<XYVectorInt>("divisions", XYVectorInt(100, 100));
            divisions = XYZVectorInt(1, divisions_yz.x(), divisions_yz.y());
        } else if(dimension == 3) {
            divisions = config.get<XYZVectorInt>("divisions", XYZVectorInt(100, 100, 100));
        } else {
            throw allpix::InvalidValueError(config, "dimension", "only two or three dimensional fields are supported");
        }

        // Swapping elements
        auto rot = config.getArray<std::string>("xyz", {"x", "y", "z"});
        if(rot.size() != 3) {
            throw allpix::InvalidValueError(config, "xyz", "three entries required");
        }

        auto start = std::chrono::system_clock::now();

        std::string grid_file = file_prefix + ".grd";
        std::vector<Point> points = parser->getMesh(grid_file, regions);

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
        const double xstep = (maxx - minx) / static_cast<double>(divisions.x());
        const double ystep = (maxy - miny) / static_cast<double>(divisions.y());
        const double zstep = (maxz - minz) / static_cast<double>(divisions.z());
        const double cell_volume = xstep * ystep * zstep;

        // Using the minimal cell dimension as initial search radius for the point cloud:
        const auto initial_radius = config.get<double>("initial_radius", std::min({xstep, ystep, zstep}));
        LOG(INFO) << "Using initial neighbor search radius of " << initial_radius;

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

        rot.at(0).erase(std::remove(rot.at(0).begin(), rot.at(0).end(), '-'), rot.at(0).end());
        rot.at(1).erase(std::remove(rot.at(1).begin(), rot.at(1).end(), '-'), rot.at(1).end());
        rot.at(2).erase(std::remove(rot.at(2).begin(), rot.at(2).end(), '-'), rot.at(2).end());

        auto end = std::chrono::system_clock::now();
        auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
        LOG(INFO) << "Reading the files took " << elapsed_seconds << " seconds.";

        // Initializing the Octree with points from mesh cloud.
        unibn::Octree<Point> octree;
        octree.initialize(points);

        int mesh_points_done = 0;
        auto mesh_section = [&](double x, double y) {
            allpix::Log::setReportingLevel(log_level);

            // New mesh slice
            std::vector<Point> new_mesh;

            double z = minz + zstep / 2.0;
            for(int k = 0; k < divisions.z(); ++k) {
                // New mesh vertex and field
                Point q(dimension == 2 ? -1 : x, y, z), e;
                bool valid = false;

                size_t prev_neighbours = 0;
                double radius = initial_radius;

                while(radius < max_radius) {
                    LOG(DEBUG) << "Search radius: " << radius;
                    // Calling octree neighbours search and sorting the results list with the closest neighbours first
                    std::vector<unsigned int> results;
                    octree.radiusNeighbors<unibn::L2Distance<Point>>(q, radius, results);
                    LOG(DEBUG) << "Number of vertices found: " << results.size();

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
                                                    Combination(&points, &field, q, volume_cut));
                    valid = res.valid();
                    if(valid) {
                        e = res.result();
                        break;
                    }

                    radius = radius + radius_step;
                    LOG(DEBUG) << "All combinations tried. Increasing search radius to " << radius;
                }

                if(!valid) {
                    throw std::runtime_error(
                        "Could not find valid volume element. Consider to increase max_radius to include "
                        "more mesh points in the search");
                }

                new_mesh.push_back(e);
                z += zstep;
            }

            mesh_points_done += divisions.z();
            LOG_PROGRESS(INFO, "m") << "Interpolating new mesh: " << mesh_points_done << " of " << mesh_points_total << ", "
                                    << (100 * mesh_points_done / mesh_points_total) << "%";

            return new_mesh;
        };

        // Start the interpolation on many threads:
        auto num_threads = config.get<unsigned int>("workers", std::max(std::thread::hardware_concurrency(), 1u));
        ThreadPool::registerThreadCount(num_threads);
        LOG(STATUS) << "Starting regular grid interpolation with " << num_threads << " threads.";
        std::vector<Point> e_field_new_mesh;

        // clang-format off
        auto init_function = [log_level = allpix::Log::getReportingLevel(), log_format = allpix::Log::getFormat()]() {
            // clang-format on
            // Initialize the threads to the same log level and format as the master setting
            allpix::Log::setReportingLevel(log_level);
            allpix::Log::setFormat(log_format);
        };

        ThreadPool pool(num_threads, num_threads * 1024, init_function);
        std::vector<std::shared_future<std::vector<Point>>> mesh_futures;
        // Set starting point
        double x = minx + xstep / 2.0;
        // Loop over x coordinate, add tasks for each coordinate to the queue
        for(int i = 0; i < divisions.x(); ++i) {
            double y = miny + ystep / 2.0;
            for(int j = 0; j < divisions.y(); ++j) {
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
        std::array<double, 3> size{{allpix::Units::get(maxx - minx, "um"),
                                    allpix::Units::get(maxy - miny, "um"),
                                    allpix::Units::get(maxz - minz, "um")}};
        std::array<size_t, 3> gridsize{
            {static_cast<size_t>(divisions.x()), static_cast<size_t>(divisions.y()), static_cast<size_t>(divisions.z())}};

        FieldQuantity quantity = (vector_field ? FieldQuantity::VECTOR : FieldQuantity::SCALAR);
        // Prepare data:
        LOG(INFO) << "Preparing data for storage...";
        auto data = std::make_shared<std::vector<double>>();
        for(int i = 0; i < divisions.x(); ++i) {
            for(int j = 0; j < divisions.y(); ++j) {
                for(int k = 0; k < divisions.z(); ++k) {
                    auto& point = e_field_new_mesh[static_cast<unsigned int>(i * divisions.y() * divisions.z() +
                                                                             j * divisions.z() + k)];
                    // We need to convert to framework-internal units:
                    data->push_back(allpix::Units::get(point.x, units));
                    // For a vector field, we push three values:
                    if(quantity == FieldQuantity::VECTOR) {
                        data->push_back(allpix::Units::get(point.y, units));
                        data->push_back(allpix::Units::get(point.z, units));
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
    allpix::Log::finish();
    return return_code;
}
