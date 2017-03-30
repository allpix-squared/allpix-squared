/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MODULE_FACTORY_H
#define ALLPIX_MODULE_FACTORY_H

#include <memory>
#include <utility>
#include <vector>

#include "Module.hpp"
#include "exceptions.h"
#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"

namespace allpix {

    class AllPix;

    class ModuleFactory {
    public:
        // Constructor and destructor
        ModuleFactory();
        virtual ~ModuleFactory();

        // Disallow copy of a factory
        ModuleFactory(const ModuleFactory&) = delete;
        ModuleFactory& operator=(const ModuleFactory&) = delete;

        // set configuration
        void setConfiguration(Configuration conf);
        Configuration& getConfiguration();

        // set messenger
        void setMessenger(Messenger*);
        Messenger* getMessenger();

        // set geometry manager
        void setGeometryManager(GeometryManager*);
        GeometryManager* getGeometryManager();

        // create a module
        virtual std::vector<std::pair<ModuleIdentifier, std::unique_ptr<Module>>> create() = 0;

    private:
        Configuration config_;

        Messenger* messenger_;
        GeometryManager* geometry_manager_;
    };
  
  template <typename T> class UniqueModuleFactory : public ModuleFactory {
  public:
    // create a module
    std::vector<std::pair<ModuleIdentifier, std::unique_ptr<Module>>> create() override {
      std::vector<std::pair<ModuleIdentifier, std::unique_ptr<Module>>> mod_list;
      
      // create a unique instance of the module
      ModuleIdentifier identifier(T::name, "", 0);
      mod_list.emplace_back(identifier, std::make_unique<T>(getConfiguration(), getMessenger(), getGeometryManager()));
      
      return mod_list;
    }
  };
  
  template <typename T> class DetectorModuleFactory : public ModuleFactory {
  public:
    // create a module
    std::vector<std::pair<ModuleIdentifier, std::unique_ptr<Module>>> create() override {
      std::set<std::string> all_names;
      std::vector<std::pair<ModuleIdentifier, std::unique_ptr<Module>>> mod_list;
      
      Configuration conf = getConfiguration();
      
      // FIXME: lot of overlap here...!
      // FIXME: check empty config arrays
      
      // instantiate all names first with highest priority
      if(conf.has("name")) {
        std::vector<std::string> names = conf.getArray<std::string>("name");
        for(auto& name : names) {
          auto det = getGeometryManager()->getDetector(name);
          
          // create with detector name and priority
          ModuleIdentifier identifier(T::name, det->getName(), 1);
          mod_list.emplace_back(identifier, std::make_unique<T>(conf, getMessenger(), det));
          // check if the module called the correct base class constructor
          check_module_detector(identifier.getName(), mod_list.back().second.get(), det.get());
          // save the name (to not override it later)
          all_names.insert(name);
        }
      }
      
      // then instantiate all types that are not yet name instantiated
      if(conf.has("type")) {
        std::vector<std::string> types = conf.getArray<std::string>("type");
        for(auto& type : types) {
          auto detectors = getGeometryManager()->getDetectorsByType(type);
          
          for(auto& det : detectors) {
            // skip all that were already added by name
            if(all_names.find(det->getName()) != all_names.end()) {
              continue;
            }
            
            // create with detector name and priority
            ModuleIdentifier identifier(T::name, det->getName(), 2);
            mod_list.emplace_back(identifier, std::make_unique<T>(conf, getMessenger(), det));
            // check if the module called the correct base class constructor
            check_module_detector(identifier.getName(), mod_list.back().second.get(), det.get());
          }
        }
      }
      
      // instantiate for all detectors if no name / type provided
      if(!conf.has("type") && !conf.has("name")) {
        auto detectors = getGeometryManager()->getDetectors();
        
        for(auto& det : detectors) {
          // create with detector name and priority
          ModuleIdentifier identifier(T::name, det->getName(), 0);
          mod_list.emplace_back(identifier, std::make_unique<T>(conf, getMessenger(), det));
          // check if the module called the correct base class constructor
          check_module_detector(identifier.getName(), mod_list.back().second.get(), det.get());
        }
      }
      
      return mod_list;
    }
    
  private:
    inline void check_module_detector(const std::string& module_name, Module* module, const Detector* detector) {
      if(module->getDetector().get() != detector) {
        throw InvalidModuleStateException(
                                          "Module " + module_name +
                                          " does not call the correct base Module constructor: the provided detector should be forwarded");
      }
    }
  };
  
  
}

#endif /* ALLPIX_SINGLE_MODULE_FACTORY_H */
