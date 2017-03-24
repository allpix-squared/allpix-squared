/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MODULE_FACTORY_H
#define ALLPIX_MODULE_FACTORY_H

#include <memory>
#include <vector>

#include "Module.hpp"
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
}

#endif /* ALLPIX_SINGLE_MODULE_FACTORY_H */
