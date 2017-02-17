#include <iostream>
#include <ostream>
#include <vector>
#include <memory>

#include "../core/AllPix.hpp"

#include "../core/module/Module.hpp"
#include "../core/module/StaticModuleManager.hpp"

#include "../core/messenger/Messenger.hpp"

#include "../core/config/SimpleConfigManager.hpp"

#include "../core/geometry/Detector.hpp"
#include "../core/geometry/DefaultGeometryManager.hpp"

#include "../core/utils/exceptions.h"
#include "../core/utils/log.h"

#include "examples.h"

using namespace allpix;

int main(int, const char **) {
    std::string file_name = "etc/example.ini";
    
    try {
        // Set global log level:
        LogLevel log_level = Log::getLevelFromString("DEBUG");
        Log::setReportingLevel(log_level);
        
        LOG(INFO) << "Set log level: " << Log::getStringFromLevel(log_level);
                
        std::unique_ptr<Messenger> msg = std::make_unique<Messenger>();
        std::unique_ptr<DefaultGeometryManager> geo = std::make_unique<DefaultGeometryManager>();
        std::unique_ptr<StaticModuleManager> mod = std::make_unique<StaticModuleManager>(&generator);
        std::unique_ptr<SimpleConfigManager> conf = std::make_unique<SimpleConfigManager>(file_name);
        
        /*auto configs = conf->getConfigurations();
        for(auto &config : configs){
            std::cout << "[" << config.getName() << "]" << std::endl;
            config.print();
            std::cout << std::endl;
        }*/
        
        Detector det1("name1", "type1");
        Detector det2("name2", "type1");
        Detector det3("name3", "type2");
        
        geo->addDetector(det1);
        geo->addDetector(det2);
        geo->addDetector(det3);
        
        std::unique_ptr<AllPix> apx = std::make_unique<AllPix>(std::move(conf), std::move(mod), std::move(msg), std::move(geo));
        
        LOG(INFO) << "Initializing AllPix";
        apx->init();
        LOG(INFO) << "Running AllPix";
        apx->run();
        LOG(INFO) << "Finishing AllPix";
        apx->finalize();
        
    } catch(Exception &e) { 
        LOG(CRITICAL) << e.what(); 
    }
        
    return 0;
}
