#include <memory>
#include <string>
#include <utility>

#include "core/AllPix.hpp"
#include "core/config/ConfigManager.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/utils/exceptions.h"
#include "core/utils/log.h"

using namespace allpix;

int main(int argc, const char* argv[]) {
    // FIXME: implement proper argument handling
    std::string config_file_name;
    if(argc == 2) {
        config_file_name = argv[1];
    } else {
        LOG(FATAL) << "No configuration file provided! Rerun with configuration file as first argument.";
        return 1;
    }

    try {
        // Construct managers
        // FIXME: move module manager initialization to AllPix as soon we have dynamic loading
        std::unique_ptr<ModuleManager> mod = std::make_unique<ModuleManager>();

        // Construct main AllPix object
        std::unique_ptr<AllPix> apx = std::make_unique<AllPix>(config_file_name, std::move(mod));

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
        return 1;
    } catch(RuntimeError& e) {
        LOG(FATAL) << "Error during execution of run:" << std::endl
                   << "   " << e.what() << std::endl
                   << "Please check your configuration and modules! Cannot continue...";
        return 1;
    } catch(LogicError& e) {
        LOG(FATAL) << "Error in the logic of module:" << std::endl
                   << "   " << e.what() << std::endl
                   << "Module has to be properly defined! Cannot continue...";
        return 1;
    } catch(std::exception& e) {
        LOG(FATAL) << "Fatal internal error" << std::endl << "   " << e.what() << std::endl << "Cannot continue...";
        return 127;
    }

    return 0;
}
