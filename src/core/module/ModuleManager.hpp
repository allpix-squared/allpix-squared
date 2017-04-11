/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MODULE_MANAGER_H
#define ALLPIX_MODULE_MANAGER_H

#include <list>
#include <map>
#include <memory>
#include <queue>

#include "Module.hpp"

namespace allpix {

    class ConfigManager;
    class Messenger;
    class GeometryManager;

    class ModuleManager {
    public:
        // Constructor and destructors
        ModuleManager();
        virtual ~ModuleManager();

        // Disallow copy
        ModuleManager(const ModuleManager&) = delete;
        ModuleManager& operator=(const ModuleManager&) = delete;

        // Load modules
        virtual void load(Messenger* messenger, ConfigManager* conf_manager, GeometryManager* geo_manager) = 0;

        // Initialize modules (pre-run)
        virtual void init();

        // Run modules
        virtual void run();

        // Finalize modules (post-run)
        virtual void finalize();

    protected:
        using ModuleList = std::list<std::unique_ptr<Module>>;
        using IdentifierToModuleMap = std::map<ModuleIdentifier, ModuleList::iterator>;
        using ModuleToIdentifierMap = std::map<Module*, ModuleIdentifier>;

        // Add module to run queue
        void add_to_run_queue(Module*);

        // get module identifier
        ModuleIdentifier get_identifier_from_module(Module*);

        ModuleList modules_;
        IdentifierToModuleMap id_to_module_;
        ModuleToIdentifierMap module_to_id_;

        // FIXME: if we stick to the linear run we can also just run modules directly from list
        std::queue<Module*> run_queue_;
    };
} // namespace allpix

#endif /* ALLPIX_MODULE_MANAGER_H */
