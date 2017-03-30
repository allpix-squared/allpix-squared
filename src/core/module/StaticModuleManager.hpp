/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_STATIC_MODULE_MANAGER_H
#define ALLPIX_STATIC_MODULE_MANAGER_H

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "Module.hpp"
#include "ModuleFactory.hpp"
#include "ModuleManager.hpp"

namespace allpix {

    class StaticModuleManager : public ModuleManager {
    public:
        using GeneratorFunction = std::function<std::unique_ptr<ModuleFactory>(std::string)>;

        // Constructor and destructors
        explicit StaticModuleManager(GeneratorFunction);

        // Load modules
        void load(Messenger* messenger, ConfigManager* conf_manager, GeometryManager* geo_manager) override;

    private:
        std::unique_ptr<ModuleFactory> get_factory(const std::string& name);

        std::map<std::string, int> _instantiations_map;

        GeneratorFunction generator_func_;
    };
}

#endif /* ALLPIX_STATIC_MODULE_MANAGER_H */
