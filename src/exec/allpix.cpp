/**
 * @file
 * @brief Executable running the framework
 *
 * @copyright Copyright (c) 2016-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include <array>
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <ostream>
#include <signal.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#if defined(__linux__) || (defined(__APPLE__) && !defined(__arm64__))
#include <cpuid.h>
#endif

#include <ROOT/RVersion.hxx>
#include <boost/version.hpp>

#ifdef ALLPIX_GEANT4_AVAILABLE
#include <G4Version.hh>
#endif

#include "core/Allpix.hpp"
#include "core/config/exceptions.h"
#include "core/utils/exceptions.h"
#include "core/utils/log.h"

using namespace allpix;

void clean();
void abort_handler(int /*unused*/);
void interrupt_handler(int /*unused*/);

std::unique_ptr<Allpix> apx;
std::atomic<bool> apx_ready{false};

/**
 * @brief Handle user abort (CTRL+\) which should stop the framework immediately
 * @note This handler is actually not fully reliable (but otherwise crashing is okay...)
 */
void abort_handler(int /*unused*/) {
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
void interrupt_handler(int /*unused*/) {
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
        std::string const arg = argv[i];

        if(arg == "-h") {
            print_help = true;
        } else if(arg == "--version") {
            std::cout << "Allpix Squared version " << ALLPIX_PROJECT_VERSION;
#ifdef ALLPIX_BUILD_ENV
            std::cout << " (" << ALLPIX_BUILD_ENV << ")";
#endif
            std::cout << '\n';
            std::cout << "               built on " << ALLPIX_BUILD_TIME << '\n';
            std::cout << "               using Boost.Random " << BOOST_VERSION / 100000 << "." // major version
                      << BOOST_VERSION / 100 % 1000 << "."                                     // minor version
                      << BOOST_VERSION % 100                                                   // patch level
                      << '\n';
            std::cout << "                     ROOT " << ROOT_RELEASE << '\n';
#ifdef ALLPIX_GEANT4_AVAILABLE
            std::cout << "                     Geant4 " << G4VERSION_NUMBER / 100 << "." << G4VERSION_NUMBER / 10 % 10 << "."
                      << G4VERSION_NUMBER % 10 << '\n';
#endif

            // The new apple m silicon does not include cpuid
#if defined(__linux__) || (defined(__APPLE__) && !defined(__arm64__))
            std::array<char, 0x40> cpu_string{};
            std::array<unsigned int, 4> cpu_info = {0, 0, 0, 0};
            __cpuid(0x80000000, cpu_info[0], cpu_info[1], cpu_info[2], cpu_info[3]);
            unsigned int const n_ex_ids = cpu_info[0];
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
                      << '\n';

            std::cout << '\n';
#endif

            std::cout << "Copyright (c) 2016-2025 CERN and the Allpix Squared authors." << '\n' << '\n';
            std::cout << "This software is distributed under the terms of the MIT License." << '\n';
            std::cout << "In applying this license, CERN does not waive the privileges and immunities" << '\n';
            std::cout << "granted to it by virtue of its status as an Intergovernmental Organization" << '\n';
            std::cout << "or submit itself to any jurisdiction." << '\n';
            clean();
            return 0;
        } else if(arg == "-v" && (i + 1 < argc)) {
            try {
                LogLevel const log_level = Log::getLevelFromString(std::string(argv[++i]));
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
        } else if(arg.starts_with("-j")) {
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
        std::cout << "Allpix Squared" << '\n';
        std::cout << "Generic Pixel Detector Simulation Framework" << '\n';
        std::cout << '\n';
        std::cout << "Usage: allpix -c <file> [OPTIONS]" << '\n';
        std::cout << '\n';
        std::cout << "Options:" << '\n';
        std::cout << "  -c <file>    configuration file to be used" << '\n';
        std::cout << "  -l <file>    file to log to besides standard output" << '\n';
        std::cout << "  -o <option>  extra module configuration option(s) to pass" << '\n';
        std::cout << "  -g <option>  extra detector configuration options(s) to pass" << '\n';
        std::cout << "  -v <level>   verbosity level, overwriting the global level" << '\n';
        std::cout << "  -j <workers> number of worker threads, equivalent to" << '\n';
        std::cout << "               -o multithreading=true -o workers=<workers>" << '\n';
        std::cout << "  --version    print version information and quit" << '\n';
        std::cout << '\n';
        std::cout << "For more help, please see <https://cern.ch/allpix-squared>" << '\n';
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
        LOG(FATAL) << "Error in the configuration:" << '\n'
                   << e.what() << '\n'
                   << "The configuration needs to be updated. Cannot continue.";
        return_code = 1;
    } catch(RuntimeError& e) {
        LOG(FATAL) << "Error during execution of run:" << '\n'
                   << e.what() << '\n'
                   << "Please check your configuration and modules. Cannot continue.";
        return_code = 1;
    } catch(LogicError& e) {
        LOG(FATAL) << "Error in the logic of module:" << '\n'
                   << e.what() << '\n'
                   << "Module has to be properly defined. Cannot continue.";
        return_code = 1;
    } catch(std::exception& e) {
        LOG(FATAL) << "Fatal internal error" << '\n' << e.what() << '\n' << "Cannot continue.";
        return_code = 127;
    }

    // Finish the logging
    clean();

    return return_code;
}
