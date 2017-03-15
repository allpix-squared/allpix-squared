#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "core/AllPix.hpp"

#include "core/config/ConfigManager.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/module/StaticModuleManager.hpp"

#include "core/utils/exceptions.h"
#include "core/utils/log.h"

#include "core/module/UniqueModuleFactory.hpp"

// FIXME: should not be here
#include "modules/deposit_reader_test/TestDepositReaderModule.hpp"
#include "modules/deposition_simple/SimpleDepositionModule.hpp"
#include "modules/geometry_test/GeometryConstructionModule.hpp"
#include "modules/visualization_test/TestVisualizationModule.hpp"

using namespace allpix;

// FIXME: temporary generator function for as long we do not have dynamic loading
std::unique_ptr<ModuleFactory> generator(const std::string& str);
std::unique_ptr<ModuleFactory> generator(const std::string& str) {
    if(str == GeometryConstructionModule::name) {
        return std::make_unique<UniqueModuleFactory<GeometryConstructionModule>>();
    }
    if(str == SimpleDepositionModule::name) {
        return std::make_unique<UniqueModuleFactory<SimpleDepositionModule>>();
    }
    if(str == TestVisualizationModule::name) {
        return std::make_unique<UniqueModuleFactory<TestVisualizationModule>>();
    }
    if(str == TestDepositReaderModule::name) {
        return std::make_unique<UniqueModuleFactory<TestDepositReaderModule>>();
    }

    return nullptr;
}

int main(int argc, const char* argv[]) {
    std::string file_name = "etc/example.ini";

    // FIXME: temporary config pass until we have proper argument handling
    if(argc == 2) {
        file_name = argv[1];
    }

    try {
        // Set global log level:
        LogLevel log_level = Log::getLevelFromString("DEBUG");
        Log::setReportingLevel(log_level);
        LOG(INFO) << "Set log level: " << Log::getStringFromLevel(log_level);

        // Construct managers (FIXME: move some initialization to AllPix)
        std::unique_ptr<GeometryManager>     geo = std::make_unique<GeometryManager>();
        std::unique_ptr<StaticModuleManager> mod = std::make_unique<StaticModuleManager>(&generator);
        std::unique_ptr<ConfigManager>       conf = std::make_unique<ConfigManager>(file_name);

        // Construct main AllPix object
        std::unique_ptr<AllPix> apx = std::make_unique<AllPix>(std::move(conf), std::move(mod), std::move(geo));

        LOG(INFO) << "Initializing AllPix";
        apx->init();

        LOG(INFO) << "Running AllPix";
        apx->run();

        LOG(INFO) << "Finishing AllPix";
        apx->finalize();
    } catch(ConfigurationError& e) {
        LOG(CRITICAL) << "Error in the configuration file:" << std::endl
                      << "   " << e.what() << std::endl
                      << "The configuration file needs to be updated! Cannot continue...";
    } catch(RuntimeError& e) {
        LOG(CRITICAL) << "Error during execution of run:" << std::endl
                      << "   " << e.what() << std::endl
                      << "Please check your configuration and modules! Cannot continue...";
    } catch(LogicError& e) {
        LOG(CRITICAL) << "Error in the logic of module:" << std::endl
                      << "   " << e.what() << std::endl
                      << "Module has to be properly defined! Cannot continue...";
    } catch(std::exception& e) {
        LOG(CRITICAL) << "Fatal internal error" << std::endl << "   " << e.what() << std::endl << "Cannot continue...";
    }

    return 0;
}
