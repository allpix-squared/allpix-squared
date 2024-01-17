/**
 * @file
 *
 * @copyright Copyright (c) 2019-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include <csignal>
#include <filesystem>
#include <fstream>

#include "core/config/ConfigReader.hpp"
#include "core/config/Configuration.hpp"
#include "core/config/exceptions.h"
#include "core/geometry/DetectorModel.hpp"
#include "core/module/ThreadPool.hpp"
#include "core/utils/log.h"
#include "tools/ROOT.h"
#include "tools/field_parser.h"
#include "tools/units.h"

#include <Math/Point2D.h>
#include <Math/Point3D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

using allpix::ThreadPool;

void interrupt_handler(int);

bool ROOT::Math::operator<(const ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>>& lhs,
                           const ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>>& rhs) {
    if(lhs.x() == rhs.x()) {
        return lhs.y() < rhs.y();
    }
    return lhs.x() < rhs.x();
}

/**
 * @brief Handle termination request (CTRL+C)
 */
void interrupt_handler(int) {
    LOG(STATUS) << "Interrupted! Aborting generation...";
    allpix::Log::finish();
    std::exit(0);
}

int main(int argc, char** argv) {
    using XYZVectorInt = ROOT::Math::DisplacementVector3D<ROOT::Math::Cartesian3D<size_t>>;
    using XYVectorInt = ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<size_t>>;

    // If no arguments are provided, print the help:
    bool print_help = false;
    int return_code = 0;
    if(argc == 1) {
        print_help = true;
        return_code = 1;
    }

    try {

        // Add stream and set default logging level
        allpix::Log::addStream(std::cout);
        allpix::register_units();

        // Install abort handler (CTRL+\) and interrupt handler (CTRL+C)
        std::signal(SIGQUIT, interrupt_handler);
        std::signal(SIGINT, interrupt_handler);

        std::string output_file_prefix = "model";

        std::string model_path;
        allpix::LogLevel log_level = allpix::LogLevel::INFO;
        XYVectorInt matrix(3, 3);
        XYZVectorInt binning;
        auto file_type = allpix::FileType::APF;

        for(int i = 1; i < argc; i++) {
            if(strcmp(argv[i], "-h") == 0) {
                print_help = true;
            } else if(strcmp(argv[i], "--init") == 0) {
                file_type = allpix::FileType::INIT;
            } else if(strcmp(argv[i], "--binning") == 0 && (i + 1 < argc)) {
                binning = allpix::from_string<XYZVectorInt>(std::string(argv[++i]));
            } else if(strcmp(argv[i], "--matrix") == 0 && (i + 1 < argc)) {
                matrix = allpix::from_string<XYVectorInt>(std::string(argv[++i]));
            } else if(strcmp(argv[i], "--model") == 0 && (i + 1 < argc)) {
                model_path = std::filesystem::canonical(std::string(argv[++i]));
            } else if(strcmp(argv[i], "--output") == 0 && (i + 1 < argc)) {
                output_file_prefix = std::string(argv[++i]);
            } else if(strcmp(argv[i], "-v") == 0 && (i + 1 < argc)) {
                try {
                    log_level = allpix::Log::getLevelFromString(std::string(argv[++i]));
                } catch(std::invalid_argument& e) {
                    LOG(ERROR) << "Invalid verbosity level \"" << std::string(argv[i]) << "\", ignoring overwrite";
                    return_code = 1;
                }
            } else {
                LOG(ERROR) << "Unrecognized command line argument or missing value \"" << argv[i] << "\"";
                print_help = true;
                return_code = 1;
            }
        }

        // Set log level:
        allpix::Log::setReportingLevel(log_level);

        // Print help if requested or no arguments given
        if(print_help) {
            std::cerr << "Usage: generate_potential --model <file_name> [<options>]" << std::endl;
            std::cout << "Required parameters:" << std::endl;
            std::cout << "\t --model <file>          Canonical path to the model file the potential should be generated for"
                      << std::endl;
            std::cout << "Optional parameters:" << std::endl;
            std::cout << "\t --binning <int vector>  3D vector with the number of bins in each coordinate x, y, z"
                      << std::endl;
            std::cout
                << "\t --matrix  <int vector>  2D vector with size of the pixel array in x and y the potential should be "
                   "calculated for"
                << std::endl;
            std::cout << "\t --output  <file name>   Name of the file the potential should be stored in" << std::endl;
            std::cout
                << "\t --init                  Switch to enable writing the potential in the INIT format instead of APF"
                << std::endl;
            std::cout << "\t -v <level>              verbosity level (default reporiting level is INFO)" << std::endl;
            std::cout << "\t -h                      print this help text" << std::endl;

            allpix::Log::finish();
            return return_code;
        }

        LOG(STATUS) << "Welcome to the Weighting Potential Generator Tool of Allpix^2 " << ALLPIX_PROJECT_VERSION;
        LOG(INFO) << "Using detector model file \"" << model_path << "\"";

        // Parsing detector model to generate potential for:
        std::ifstream file(model_path);
        allpix::ConfigReader reader(file, model_path);
        auto model = allpix::DetectorModel::factory(model_path, reader);

        // Get pixel implant size from the detector model:
        auto implants = model->getImplants();
        if(implants.size() > 1) {
            throw std::invalid_argument("Detector model contains more than one implant, not supported for pad potential");
        }

        auto implant = (implants.empty() ? ROOT::Math::XYZVector(model->getPixelSize().x(), model->getPixelSize().y(), 0)
                                         : implants.front().getSize());
        // This module currently only works with pad definition, i.e. 2D implant deinition:
        if(implant.z() > std::numeric_limits<double>::epsilon()) {
            throw std::invalid_argument("Generator can only be used with 2D implants, but non-zero thickness found");
        }

        // Calculate thickness domain
        auto sensor_max_z = model->getSensorCenter().z() + model->getSensorSize().z() / 2.0;
        auto thickness_domain = std::make_pair(-sensor_max_z, sensor_max_z);

        // Calculate field size from induction matrix size and pixel pitch:
        auto pixel_pitch = model->getPixelSize();
        auto fieldsize = ROOT::Math::XYZVector(pixel_pitch.x() * static_cast<double>(matrix.x()),
                                               pixel_pitch.y() * static_cast<double>(matrix.y()),
                                               model->getSensorSize().z());

        // Binning: default to 1 bin per um:
        if(binning.mag2() == 0) {
            binning = XYZVectorInt(static_cast<size_t>(std::round(allpix::Units::convert(fieldsize.x(), "um"))),
                                   static_cast<size_t>(std::round(allpix::Units::convert(fieldsize.y(), "um"))),
                                   static_cast<size_t>(std::round(allpix::Units::convert(fieldsize.z(), "um"))));
        }

        // Output file path:
        std::string output_file_name =
            output_file_prefix + "_weightingpotential" + (file_type == allpix::FileType::INIT ? ".init" : ".apf");

        LOG(INFO) << "Field size: " << allpix::Units::display(fieldsize, {"um", "mm"});
        LOG(INFO) << "Binning: " << binning.x() << " " << binning.y() << " " << binning.z();
        LOG(INFO) << "Output file: " << output_file_name;
        auto start = std::chrono::system_clock::now();

        // Start potential generation on many threads:
        auto num_threads = std::max(std::thread::hardware_concurrency(), 1u);
        ThreadPool::registerThreadCount(num_threads);
        LOG(STATUS) << "Starting weighting potential generation with " << num_threads << " threads.";
        auto weighting_potential = std::make_shared<std::vector<double>>();

        auto generate_section = [&](size_t index_x) {
            allpix::Log::setReportingLevel(log_level);

            auto potential = [implant, thickness_domain](const ROOT::Math::XYZPoint& pos) {
                // Calculate values of the "f" function
                auto f = [implant](double x, double y, double u) {
                    // Calculate arctan fractions
                    auto arctan = [](double a, double b, double c) {
                        return std::atan(a * b / c / std::sqrt(a * a + b * b + c * c));
                    };

                    // Shift the x and y coordinates by plus/minus half the implant size:
                    double x1 = x - implant.x() / 2;
                    double x2 = x + implant.x() / 2;
                    double y1 = y - implant.y() / 2;
                    double y2 = y + implant.y() / 2;

                    // Calculate arctan sum and return
                    return arctan(x1, y1, u) + arctan(x2, y2, u) - arctan(x1, y2, u) - arctan(x2, y1, u);
                };

                // Transform into coordinate system with sensor between d/2 < z < -d/2:
                auto d = thickness_domain.second - thickness_domain.first;
                auto local_z = -pos.z() + thickness_domain.second;

                // Calculate the series expansion
                double sum = 0;
                for(int n = 1; n <= 100; n++) {
                    sum += f(pos.x(), pos.y(), 2 * n * d - local_z) - f(pos.x(), pos.y(), 2 * n * d + local_z);
                }

                return (1 / (2 * M_PI) * (f(pos.x(), pos.y(), local_z) - sum));
            };

            std::vector<double> slice;
            for(size_t index_y = 1; index_y <= binning.y(); index_y++) {
                for(size_t index_z = 1; index_z <= binning.z(); index_z++) {
                    auto pos = ROOT::Math::XYZPoint(
                        fieldsize.x() / static_cast<double>(binning.x()) * static_cast<double>(index_x) - fieldsize.x() / 2,
                        fieldsize.y() / static_cast<double>(binning.y()) * static_cast<double>(index_y) - fieldsize.y() / 2,
                        fieldsize.z() / static_cast<double>(binning.z()) * static_cast<double>(index_z) - fieldsize.z() / 2);
                    slice.push_back(potential(pos));
                }
            }
            return slice;
        };

        // clang-format off
        auto init_function = [log_level = allpix::Log::getReportingLevel(), log_format = allpix::Log::getFormat()]() {
            // clang-format on
            // Initialize the threads to the same log level and format as the master setting
            allpix::Log::setReportingLevel(log_level);
            allpix::Log::setFormat(log_format);
        };

        ThreadPool pool(num_threads, num_threads * 1024, init_function);
        std::vector<std::shared_future<std::vector<double>>> wp_futures;

        // Loop over x coordinate, add tasks for each coordinate to the queue
        for(size_t x = 1; x <= binning.x(); x++) {
            wp_futures.push_back(pool.submit(generate_section, x));
        }

        // Merge the result vectors:
        unsigned int slices_done = 0;
        for(auto& wp_future : wp_futures) {
            auto slice = wp_future.get();
            weighting_potential->insert(weighting_potential->end(), slice.begin(), slice.end());
            LOG_PROGRESS(INFO, "generation") << "Generating potential: " << (100 * slices_done / wp_futures.size()) << "%";
            slices_done++;
        }
        LOG_PROGRESS(INFO, "generation") << "Generating potential: 100%";
        pool.destroy();

        auto end = std::chrono::system_clock::now();
        auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
        LOG(INFO) << "Weighting potential generated in " << elapsed_seconds << " seconds.";

        // Prepare header and auxiliary information:
        std::string header = "Allpix Squared " + std::string(ALLPIX_PROJECT_VERSION) + " Weighting Potential Generator";
        std::array<double, 3> size{{fieldsize.x(), fieldsize.y(), fieldsize.z()}};
        std::array<size_t, 3> gridsize{{binning.x(), binning.y(), binning.z()}};

        allpix::FieldData<double> field_data(header, gridsize, size, weighting_potential);
        allpix::FieldWriter<double> field_writer(allpix::FieldQuantity::SCALAR);
        field_writer.writeFile(field_data, output_file_name, file_type);

        end = std::chrono::system_clock::now();
        elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
        LOG(STATUS) << "Generation completed in " << elapsed_seconds << " seconds.";

    } catch(std::exception& e) {
        LOG(FATAL) << "Failed to generate weighting potential: " << e.what();
        allpix::Log::finish();
        return 1;
    }
    allpix::Log::finish();
    return 0;
}
