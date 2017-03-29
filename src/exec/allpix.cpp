#include <memory>
#include <string>
#include <utility>

#include "core/AllPix.hpp"
#include "core/config/ConfigManager.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/module/StaticModuleManager.hpp"
#include "core/utils/exceptions.h"
#include "core/utils/log.h"

#include "core/module/ModuleFactory.hpp"

// FIXME: should not be here
#include "modules/GeometryBuilderTGeo/TGeoBuilderModule.hpp"
#include "modules/deposit_reader_test/TestDepositReaderModule.hpp"
#include "modules/detector_histogrammer_test/DetectorHistogrammerTestModule.hpp"

#include "modules/DepositionGeant4/DepositionGeant4Module.hpp"
#include "modules/Example/ExampleModule.hpp"
#include "modules/GeometryBuilderGeant4/GeometryBuilderGeant4Module.hpp"
#include "modules/VisualizationGeant4/VisualizationGeant4Module.hpp"

using namespace allpix;

// FIXME: temporary generator function for as long we do not have dynamic loading
std::unique_ptr<ModuleFactory> generator(const std::string& str);
std::unique_ptr<ModuleFactory> generator(const std::string& str) {
    if(str == GeometryBuilderGeant4Module::name) {
        return std::make_unique<UniqueModuleFactory<GeometryBuilderGeant4Module>>();
    }
    if(str == DepositionGeant4Module::name) {
        return std::make_unique<UniqueModuleFactory<DepositionGeant4Module>>();
    }
    if(str == VisualizationGeant4Module::name) {
        return std::make_unique<UniqueModuleFactory<VisualizationGeant4Module>>();
    }
    if(str == ExampleModule::name) {
        return std::make_unique<UniqueModuleFactory<ExampleModule>>();
    }

    if(str == DetectorHistogrammerModule::name) {
        return std::make_unique<DetectorModuleFactory<DetectorHistogrammerModule>>();
    }
    if(str == TGeoBuilderModule::name) {
        return std::make_unique<UniqueModuleFactory<TGeoBuilderModule>>();
    }
    if(str == TestDepositReaderModule::name) {
        return std::make_unique<UniqueModuleFactory<TestDepositReaderModule>>();
    }

    return nullptr;
}

int main(int argc, const char* argv[]) {
    // FIXME: have standard config and pass replacement as single argument until we have proper argument handling
    std::string config_file_name = "etc/example_config.ini";
    if(argc == 2) {
        config_file_name = argv[1];
    }

    try {
        // Set global log level:
        LogLevel log_level = Log::getLevelFromString("DEBUG");
        Log::setReportingLevel(log_level);
        LOG(INFO) << "Set log level: " << Log::getStringFromLevel(log_level);

        // Construct managers
        // FIXME: move module manager initialization to AllPix as soon we have dynamic loading
        std::unique_ptr<StaticModuleManager> mod = std::make_unique<StaticModuleManager>(&generator);

        // Construct main AllPix object
        std::unique_ptr<AllPix> apx = std::make_unique<AllPix>(config_file_name, std::move(mod));

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
        return 1;
    } catch(RuntimeError& e) {
        LOG(CRITICAL) << "Error during execution of run:" << std::endl
                      << "   " << e.what() << std::endl
                      << "Please check your configuration and modules! Cannot continue...";
        return 1;
    } catch(LogicError& e) {
        LOG(CRITICAL) << "Error in the logic of module:" << std::endl
                      << "   " << e.what() << std::endl
                      << "Module has to be properly defined! Cannot continue...";
        return 1;
    } catch(std::exception& e) {
        LOG(CRITICAL) << "Fatal internal error" << std::endl << "   " << e.what() << std::endl << "Cannot continue...";
        return 127;
    }

    return 0;
}
