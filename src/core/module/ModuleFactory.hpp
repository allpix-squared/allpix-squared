/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 *  @author Daniel Hynds <daniel.hynds@cern.ch>
 */

#ifndef ALLPIX_MODULE_FACTORY_H
#define ALLPIX_MODULE_FACTORY_H

#include <memory>
#include <utility>
#include <vector>

#include "Module.hpp"
#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "exceptions.h"

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
        std::vector<std::pair<ModuleIdentifier, Module*>> createModules(std::string, void*);
        std::vector<std::pair<ModuleIdentifier, Module*>> createModulesPerDetector(std::string, void*);
        inline void check_module_detector(const std::string& module_name, Module* module, const Detector* detector) {
            if(module->getDetector().get() != detector) {
                throw InvalidModuleStateException(
                    "Module " + module_name +
                    " does not call the correct base Module constructor: the provided detector should be forwarded");
            }
        }

    private:
        Configuration config_;

        Messenger* messenger_;
        GeometryManager* geometry_manager_;
    };

} // namespace allpix

#endif /* ALLPIX_SINGLE_MODULE_FACTORY_H */
