#include <csignal>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>

#include "core/AllPix.hpp"
#include "core/config/ConfigManager.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/utils/exceptions.h"
#include "core/utils/log.h"

using namespace allpix;

void clean();
void interrupt_handler(int);

// Output interrupt message and clean
void interrupt_handler(int) {
    // NOTE: this is actually not totally reliable (otherwise crashing is fine...)
    LOG(FATAL) << "Interrupted!";
    // ignore any segmentation fault that may arise after this
    std::signal(SIGSEGV, SIG_IGN);
    clean();
    std::exit(1);
}

// Clean environment
void clean() {
    Log::finish();
}

int main(int argc, const char* argv[]) {
    // Add cout as the default logging stream
    Log::addStream(std::cout);

    // Install interrupt inter
    std::signal(SIGINT, interrupt_handler);
    std::signal(SIGTERM, interrupt_handler);

    // If no arguments are provided, print the help:
    bool print_help = false;
    int return_code = 0;
    if(argc == 1) {
        print_help = true;
        return_code = 1;
    }

    std::string config_file_name;
    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-h") == 0) {
            print_help = true;
        } else if(strcmp(argv[i], "-v") == 0 && (i + 1 < argc)) {
            try {
                LogLevel log_level = Log::getLevelFromString(std::string(argv[++i]));
                Log::setReportingLevel(log_level);
            } catch(std::invalid_argument& e) {
                LOG(ERROR) << "Invalid verbosity level \"" << std::string(argv[i]) << "\"";
            }
        } else if(strcmp(argv[i], "-c") == 0 && (i + 1 < argc)) {
            config_file_name = std::string(argv[++i]);
        } else {
            LOG(ERROR) << "Unrecognized command line argument \"" << argv[i] << "\"";
        }
    }

    if(print_help) {
        std::cout << "Usage: allpix -c <config> [-v <level>]" << std::endl;
        std::cout << "Generic simulation framework for pixel detectors" << std::endl;
        std::cout << "\t -v <level>   verbosity level, default INFO, can be overwritten \n"
                  << "\t              by global or per-module configuration." << std::endl;
        std::cout << "\t -c <config>  configuration file to be used" << std::endl;
        clean();
        return return_code;
    }

    if(config_file_name.empty()) {
        LOG(FATAL) << "No configuration file provided! See usage info with \"allpix -h\"";
        clean();
        return 1;
    }

    try {
        // Construct main AllPix object
        std::unique_ptr<AllPix> apx = std::make_unique<AllPix>(config_file_name);

        // Load modules
        apx->load();

        // Initialize modules (pre-run)
        apx->init();

        // Run modules and event-loop
        apx->run();

        // Finalize modules (post-run)
        apx->finalize();
    } catch(ConfigurationError& e) {
        LOG(FATAL) << "Error in the configuration file:" << std::endl
                   << "   " << e.what() << std::endl
                   << "The configuration file needs to be updated! Cannot continue...";
        return_code = 1;
    } catch(RuntimeError& e) {
        LOG(FATAL) << "Error during execution of run:" << std::endl
                   << "   " << e.what() << std::endl
                   << "Please check your configuration and modules! Cannot continue...";
        return_code = 1;
    } catch(LogicError& e) {
        LOG(FATAL) << "Error in the logic of module:" << std::endl
                   << "   " << e.what() << std::endl
                   << "Module has to be properly defined! Cannot continue...";
        return_code = 1;
    } catch(std::exception& e) {
        LOG(FATAL) << "Fatal internal error" << std::endl << "   " << e.what() << std::endl << "Cannot continue...";
        return_code = 127;
    }

    // Finish the logging
    clean();

    return return_code;
}
