#include <iostream>
#include <ostream>
#include <vector>
#include <memory>
#include <utility>
#include <string>

#include "../core/AllPix.hpp"

#include "../core/module/Module.hpp"
#include "../core/module/StaticModuleManager.hpp"

#include "../core/messenger/Messenger.hpp"

#include "../core/config/SimpleConfigManager.hpp"

#include "../core/geometry/Detector.hpp"
#include "../core/geometry/DefaultGeometryManager.hpp"

#include "../core/utils/exceptions.h"
#include "../core/utils/log.h"

#include "../tools/runge_kutta.h"

#include "examples.h"

using namespace allpix;

Eigen::Vector3d function(double time, Eigen::Vector3d pos) {
    return time*pos;  // Eigen::Vector3d::Zero();
}

int main(int, const char **) {
    std::string file_name = "etc/example.ini";
    
    /*constexpr double RK3[4][3] = {{0, 0, 0},
        {1/2, 0, 0},
        {-1, 2, 0},
        {1/6, 2/3, 1/6}};*/
    
    //RungeKutta<double, 3, 3> runge_kutta(tableau::RK3, function, 1e-9, Eigen::Vector3d());
    auto runge_kutta = make_runge_kutta(tableau::RK5, function, 1e-9, Eigen::Vector3d(1, 0, 0));
    
    for (int i = 0; i < 5; ++i) {
        auto step = runge_kutta.step(10000);
        std::cout << step.value << std::endl;
        std::cout << step.error << std::endl;
        //runge_kutta.step();
        std::cout << std::endl;
    }
    std::cout << runge_kutta.getResult() << std::endl;
    
    return 0;
    
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
    } catch (ConfigurationError &e) {
        LOG(CRITICAL) << "Error in the configuration file:";
        LOG(CRITICAL) << "   " << e.what(); 
        LOG(CRITICAL) << "The configuration file needs to be updated! Cannot continue...";
    } catch (RuntimeError &e) {
        LOG(CRITICAL) << "Error during execution of run:";
        LOG(CRITICAL) << "   " << e.what(); 
        LOG(CRITICAL) << "Please check your configuration and modules! Cannot continue...";
    } catch (LogicError &e) {
        LOG(CRITICAL) << "Error in the logic of module:";
        LOG(CRITICAL) << "   " << e.what(); 
        LOG(CRITICAL) << "Module has to be properly defined! Cannot continue...";
    } catch(std::exception &e) { 
        LOG(CRITICAL) << "Fatal internal error";
        LOG(CRITICAL) << "   " << e.what(); 
        LOG(CRITICAL) << "Cannot continue...";
    }
        
    return 0;
}
