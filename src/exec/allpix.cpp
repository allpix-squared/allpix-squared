/**
 * @file
 * @brief Executable running the framework
 *
 * @copyright Copyright (c) 2016-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include <atomic>
#if defined(__linux__) || (defined(__APPLE__) && !defined(__arm64__))
#include <cpuid.h>
#endif

#include <csignal>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <string>
#include <utility>

#include <boost/version.hpp>

#include "core/Allpix.hpp"
#include "core/config/ConfigManager.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/utils/exceptions.h"
#include "core/utils/log.h"

#ifdef ALLPIX_GEANT4_AVAILABLE
#include <G4Version.hh>
#endif

using namespace allpix;

void clean();
void abort_handler(int);
void interrupt_handler(int);

std::unique_ptr<Allpix> apx;
std::atomic<bool> apx_ready{false};

/**
 * @brief Handle user abort (CTRL+\) which should stop the framework immediately
 * @note This handler is actually not fully reliable (but otherwise crashing is okay...)
 */
void abort_handler(int) {
    // Output interrupt message and clean
    LOG(FATAL) << "Aborting!";
    clean();

    // Ignore any segmentation fault that may arise after this
    std::signal(SIGSEGV, SIG_IGN); // NOLINT
    std::abort();
}

/**
 * @brief Handle termination request (CTRL+C) as soon as possible while keeping the program flow
 */
void interrupt_handler(int) {
    // Stop the framework if it is loaded
    if(apx_ready) {
        LOG(STATUS) << "Interrupted! Finishing up active events...";
        apx->terminate();
    }
}

/**
 * @brief Clean the environment when closing application
 */
void clean() {
    Log::finish();
    if(apx_ready) {
        apx.reset();
    }
}

/**
 * @brief Main function running the application
 */
int main(int argc, const char* argv[]) {
    // Add cout as the default logging stream
    Log::addStream(std::cout);

    // Install abort handler (CTRL+\)
    std::signal(SIGQUIT, abort_handler);
    std::signal(SIGABRT, abort_handler);

    // Install interrupt handler (CTRL+C)
    std::signal(SIGINT, interrupt_handler);

    // Install termination handler (e.g. from "kill"). Gracefully exit, finish last event and quit
    std::signal(SIGTERM, interrupt_handler);

    // If no arguments are provided, print the help:
    bool print_help = false;
    int return_code = 0;
    if(argc == 1) {
        print_help = true;
        return_code = 1;
    }

    // Parse arguments
    std::string config_file_name;
    std::string log_file_name;
    std::vector<std::string> module_options;
    std::vector<std::string> detector_options;

    for(int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if(arg == "-h") {
            print_help = true;
        } else if(arg == "--version") {
            std::cout << "Allpix Squared version " << ALLPIX_PROJECT_VERSION;
#ifdef ALLPIX_BUILD_ENV
            std::cout << " (" << ALLPIX_BUILD_ENV << ")";
#endif
            std::cout << std::endl;
            std::cout << "               built on " << ALLPIX_BUILD_TIME << std::endl;
            std::cout << "               using Boost.Random " << BOOST_VERSION / 100000 << "." // major version
                      << BOOST_VERSION / 100 % 1000 << "."                                     // minor version
                      << BOOST_VERSION % 100                                                   // patch level
                      << std::endl;
            std::cout << "                     ROOT " << ROOT_RELEASE << std::endl;
#ifdef ALLPIX_GEANT4_AVAILABLE
            std::cout << "                     Geant4 " << G4VERSION_NUMBER / 100 << "." << G4VERSION_NUMBER / 10 % 10 << "."
                      << G4VERSION_NUMBER % 10 << std::endl;
#endif

            // The new apple m silicon does not include cpuid
#if defined(__linux__) || (defined(__APPLE__) && !defined(__arm64__))
            std::array<char, 0x40> cpu_string;
            std::array<unsigned int, 4> cpu_info = {0, 0, 0, 0};
            __cpuid(0x80000000, cpu_info[0], cpu_info[1], cpu_info[2], cpu_info[3]);
            unsigned int n_ex_ids = cpu_info[0];
            memset(cpu_string.data(), 0, sizeof(cpu_string));
            for(unsigned int j = 0x80000000; j <= n_ex_ids; ++j) {
                __cpuid(j, cpu_info[0], cpu_info[1], cpu_info[2], cpu_info[3]);
                if(j == 0x80000002) {
                    memcpy(cpu_string.data(), cpu_info.data(), sizeof(cpu_info));
                } else if(j == 0x80000003) {
                    memcpy(cpu_string.data() + 16, cpu_info.data(), sizeof(cpu_info));
                } else if(j == 0x80000004) {
                    memcpy(cpu_string.data() + 32, cpu_info.data(), sizeof(cpu_info));
                }
            }
            std::cout << "               running on " << std::thread::hardware_concurrency() << "x " << cpu_string.data()
                      << std::endl;

            std::cout << std::endl;
#endif

            std::cout << "Copyright (c) 2016-2023 CERN and the Allpix Squared authors." << std::endl << std::endl;
            std::cout << "This software is distributed under the terms of the MIT License." << std::endl;
            std::cout << "In applying this license, CERN does not waive the privileges and immunities" << std::endl;
            std::cout << "granted to it by virtue of its status as an Intergovernmental Organization" << std::endl;
            std::cout << "or submit itself to any jurisdiction." << std::endl;
            clean();
            return 0;
        } else if(arg == "-v" && (i + 1 < argc)) {
            try {
                LogLevel log_level = Log::getLevelFromString(std::string(argv[++i]));
                Log::setReportingLevel(log_level);
            } catch(std::invalid_argument& e) {
                LOG(ERROR) << "Invalid verbosity level \"" << std::string(argv[i]) << "\", ignoring overwrite";
            }
        } else if(arg == "-c" && (i + 1 < argc)) {
            config_file_name = std::string(argv[++i]);
        } else if(arg == "-l" && (i + 1 < argc)) {
            log_file_name = std::string(argv[++i]);
        } else if(arg == "-o" && (i + 1 < argc)) {
            module_options.emplace_back(argv[++i]);
        } else if(arg.find("-j") == 0) {
            module_options.emplace_back("multithreading=true");
            if(arg == "-j" && (i + 1 < argc)) {
                module_options.emplace_back("workers=" + std::string(argv[++i]));
            } else {
                module_options.emplace_back("workers=" + arg.substr(2));
            }
        } else if(arg == "-g" && (i + 1 < argc)) {
            detector_options.emplace_back(argv[++i]);
        } else {
            LOG(ERROR) << "Unrecognized command line argument \"" << argv[i] << "\"";
            print_help = true;
            return_code = 1;
        }
    }

    // Print help if requested or no arguments given
    if(print_help) {
        std::cout << "Allpix Squared" << std::endl;
        std::cout << "Generic Pixel Detector Simulation Framework" << std::endl;
        std::cout << std::endl;
        std::cout << "Usage: allpix -c <file> [OPTIONS]" << std::endl;
        std::cout << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  -c <file>    configuration file to be used" << std::endl;
        std::cout << "  -l <file>    file to log to besides standard output" << std::endl;
        std::cout << "  -o <option>  extra module configuration option(s) to pass" << std::endl;
        std::cout << "  -g <option>  extra detector configuration options(s) to pass" << std::endl;
        std::cout << "  -v <level>   verbosity level, overwriting the global level" << std::endl;
        std::cout << "  -j <workers> number of worker threads, equivalent to" << std::endl;
        std::cout << "               -o multithreading=true -o workers=<workers>" << std::endl;
        std::cout << "  --version    print version information and quit" << std::endl;
        std::cout << std::endl;
        std::cout << "For more help, please see <https://cern.ch/allpix-squared>" << std::endl;
        clean();
        return return_code;
    }

    // Check if we have a configuration file
    if(config_file_name.empty()) {
        LOG(FATAL) << "No configuration file provided! See usage info with \"allpix -h\"";
        clean();
        return 1;
    }

    // Add an extra file to log too if possible
    // NOTE: this stream should be available for the duration of the logging
    std::ofstream log_file;
    if(!log_file_name.empty()) {
        log_file.open(log_file_name, std::ios_base::out | std::ios_base::trunc);
        if(!log_file.good()) {
            LOG(FATAL) << "Cannot write to provided log file! Check if permissions are sufficient.";
            clean();
            return 1;
        }

        Log::addStream(log_file);
    }

    try {
        // Construct main Allpix object
        apx = std::make_unique<Allpix>(config_file_name, module_options, detector_options);
        apx_ready = true;

        // Load modules
        apx->load();

        // Initialize modules (pre-run)
        apx->initialize();

        // Run modules and event-loop
        apx->run();

        // Finalize modules (post-run)
        apx->finalize();
    } catch(ConfigurationError& e) {
        LOG(FATAL) << "Error in the configuration:" << std::endl
                   << e.what() << std::endl
                   << "The configuration needs to be updated. Cannot continue.";
        return_code = 1;
    } catch(RuntimeError& e) {
        LOG(FATAL) << "Error during execution of run:" << std::endl
                   << e.what() << std::endl
                   << "Please check your configuration and modules. Cannot continue.";
        return_code = 1;
    } catch(LogicError& e) {
        LOG(FATAL) << "Error in the logic of module:" << std::endl
                   << e.what() << std::endl
                   << "Module has to be properly defined. Cannot continue.";
        return_code = 1;
    } catch(std::exception& e) {
        LOG(FATAL) << "Fatal internal error" << std::endl << e.what() << std::endl << "Cannot continue.";
        return_code = 127;
    }

    // Finish the logging
    clean();

    return return_code;
}
