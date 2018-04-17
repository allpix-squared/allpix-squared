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

#include "TCanvas.h"
#include "TFile.h"
#include "TGraph2D.h"
#include "TH1.h"
#include "TTree.h"

#include <Eigen/Eigen>

#include "core/config/ConfigReader.hpp"
#include "core/config/Configuration.hpp"
#include "core/config/exceptions.h"
#include "core/utils/log.h"

#include "MeshElement.h"
#include "Octree.hpp"
#include "read_dfise.h"

using namespace mesh_converter;

void interrupt_handler(int);

/**
 * @brief Handle termination request (CTRL+C)
 */
void interrupt_handler(int) {
    LOG(STATUS) << "Interrupted! Aborting conversion...";
    allpix::Log::finish();
    std::exit(0);
}

void mesh_converter::mesh_plotter(const std::string& grid_file,
                                  double ss_radius,
                                  double radius,
                                  double x,
                                  double y,
                                  double z,
                                  std::vector<mesh_converter::Point> points,
                                  std::vector<unsigned int> results) {
    auto tg = new TGraph2D();
    tg->SetMarkerStyle(20);
    tg->SetMarkerSize(0.5);
    tg->SetMarkerColor(kBlack);

    for(auto& point : points) {
        if(ss_radius != -1) {
            if((fabs(point.x - x) < radius * ss_radius) && (fabs(point.y - y) < radius * ss_radius) &&
               (fabs(point.z - z) < radius * ss_radius)) {
                tg->SetPoint(tg->GetN(), point.x, point.y, point.z);
            }
        } else {
            tg->SetPoint(tg->GetN(), point.x, point.y, point.z);
        }
    }

    auto tg1 = new TGraph2D();
    auto tg2 = new TGraph2D();
    tg1->SetMarkerStyle(20);
    tg1->SetMarkerSize(1);
    tg1->SetMarkerColor(kBlue);
    tg2->SetMarkerStyle(34);
    tg2->SetMarkerSize(1);
    tg2->SetMarkerColor(kRed);
    tg2->SetPoint(0, x, y, z);
    for(auto& elem : results) {
        tg1->SetPoint(tg1->GetN(), points[elem].x, points[elem].y, points[elem].z);
    }

    std::string root_file_name_out = grid_file + "_INTERPOLATION_POINT_SCREEN_SHOT.root";
    auto root_output = new TFile(root_file_name_out.c_str(), "RECREATE");
    auto c = new TCanvas();
    tg->Draw("p");
    tg1->Draw("p same");
    tg2->Draw("p same");
    c->Write("canvas");
    root_output->Close();

    LOG(STATUS) << "Mesh screen-shot created. Closing the program.";
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

    std::string conf_file_name;

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
            conf_file_name = file_prefix + ".conf";
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
        std::cerr << "Usage: ./tcad_dfise_reader -f <file_name> [<options>]" << std::endl;
        std::cout << "\t -f <file_prefix>       common prefix of DF-ISE grid (.grd) and data (.dat) files" << std::endl;
        std::cout << "\t -c <config_file>       configuration file name" << std::endl;
        std::cout << "\t -o <init_file_prefix>  output file prefix without .init (defaults to file name of <file_prefix>)"
                  << std::endl;
        std::cout << "\t -l <file>              file to log to besides standard output (disabled by default)" << std::endl;
        std::cout << "\t -v <level>             verbosity level (default reporiting level is INFO)" << std::endl;

        allpix::Log::finish();
        return return_code;
    }

    LOG(STATUS) << "Using " << conf_file_name << " configuration file";
    std::ifstream file(conf_file_name);
    allpix::ConfigReader reader(file, conf_file_name);
    allpix::Configuration config = reader.getHeaderConfiguration();

    std::string region = config.get<std::string>("region", "bulk");
    std::string observable = config.get<std::string>("observable", "ElectricField");

    auto initial_radius = config.get<double>("initial_radius", 1);
    auto radius_step = config.get<double>("radius_step", 0.5);
    auto max_radius = config.get<double>("max_radius", 10);

    bool threshold_flag = false;
    auto radius_threshold = config.get<double>("radius_threshold", -1);
    if(radius_threshold != -1) {
        threshold_flag = true;
    }

    auto volume_cut = config.get<double>("volume_cut", 10e-9);

    bool index_cut_flag = false;
    auto index_cut = config.get<size_t>("index_cut", 999999);
    if(index_cut != 999999) {
        index_cut_flag = true;
    }

    auto xdiv = config.get<int>("xdiv", 100);
    auto ydiv = config.get<int>("ydiv", 100);
    auto zdiv = config.get<int>("zdiv", 100);
    auto dimension = config.get<int>("dimension", 3);
    if(dimension == 2) {
        xdiv = 1;
    }

    std::vector<std::string> rot = {"x", "y", "z"};
    if(config.has("xyz")) {
        rot = config.getArray<std::string>("xyz");
    }
    if(rot.size() != 3) {
        LOG(FATAL) << "Configuration keyword xyz must have 3 entries.";
        allpix::Log::finish();
        return 1;
    }

    auto mesh_tree = config.get<bool>("mesh_tree", false);

    bool ss_flag = false;
    auto ss_radius = config.get<double>("ss_radius", -1);
    std::vector<int> ss_point = {-1, -1, -1};
    if(config.has("screen_shot")) {
        ss_point = config.getArray<int>("screen_shot");
    }
    if(ss_point[0] != -1 && ss_point[1] != -1 && ss_point[2] != -1 && dimension == 3) {
        ss_flag = true;
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
        auto region_grid = read_grid(grid_file, mesh_tree);
        points = region_grid[region];
        LOG(DEBUG) << "Grid with " << points.size() << " points";
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
        LOG(DEBUG) << "Field with " << field.size() << " points";
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
    double xstep = (maxx - minx) / static_cast<double>(xdiv);
    double ystep = (maxy - miny) / static_cast<double>(ydiv);
    double zstep = (maxz - minz) / static_cast<double>(zdiv);
    double cell_volume = xstep * ystep * zstep;

    if(rot.at(0) != "x" || rot.at(1) != "y" || rot.at(2) != "z") {
        LOG(STATUS) << "TCAD mesh (x,y,z) coords. transformation into: (" << rot.at(0) << "," << rot.at(1) << ","
                    << rot.at(2) << ")";
    }
    LOG(STATUS) << "Mesh dimensions: " << maxx - minx << " x " << maxy - miny << " x " << maxz - minz << std::endl
                << "New mesh element dimension: " << xstep << " x " << ystep << " x " << zstep
                << " ==>  Volume = " << cell_volume;

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
                if(ss_flag) {
                    std::map<std::string, int> map;
                    map.emplace("x", ss_point[0]);
                    map.emplace("y", ss_point[1]);
                    map.emplace("z", ss_point[2]);
                    x = map.find(rot.at(0))->second;
                    y = map.find(rot.at(1))->second;
                    z = map.find(rot.at(2))->second;
                }
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

                    if(ss_flag) {
                        mesh_plotter(grid_file, ss_radius, radius, x, y, z, points, results);
                        return 1;
                    }

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
    init_file << "tcad_dfise_converter, ";                                             // NAME
    init_file << "observable: " << observable << std::endl;                            // OBSERVABLE INTERPOLATED
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
