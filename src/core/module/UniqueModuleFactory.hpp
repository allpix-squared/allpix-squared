/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_SINGLE_MODULE_FACTORY_H
#define ALLPIX_SINGLE_MODULE_FACTORY_H

#include <memory>
#include <vector>

#include "Module.hpp"
#include "ModuleFactory.hpp"
#include "ModuleIdentifier.hpp"
#include "core/config/Configuration.hpp"

// FIXME: ensure proper dynamic deletion

namespace allpix {

    template <typename T> class UniqueModuleFactory : public ModuleFactory {
    public:
        // create a module
        std::vector<std::pair<ModuleIdentifier, std::unique_ptr<Module>>> create() override {
            std::vector<std::pair<ModuleIdentifier, std::unique_ptr<Module>>> mod_list;

            // create a unique instance of the module
            ModuleIdentifier identifier(T::name, 0);
            mod_list.emplace_back(identifier, std::make_unique<T>(getConfiguration(), getMessenger(), getGeometryManager()));

            return mod_list;
        }
    };
}

#endif /* ALLPIX_SINGLE_MODULE_FACTORY_H */
