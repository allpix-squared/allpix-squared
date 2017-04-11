/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "ModuleManager.hpp"
#include "core/utils/log.h"

using namespace allpix;

// Constructor and destructor
ModuleManager::ModuleManager() : modules_(), id_to_module_(), module_to_id_(), global_config_() {}
ModuleManager::~ModuleManager() = default;

// Initialize all modules
void ModuleManager::init() {
    // initialize all modules
    for(auto& mod : modules_) {
        // set init module section header
        std::string old_section_name = Log::getSection();
        std::string section_name = "I:";
        section_name += module_to_id_.at(mod.get()).getName();
        Log::setSection(section_name);
        // init module
        mod->init();
        // reset header
        Log::setSection(old_section_name);
    }
}

// Run all the modules in the queue
void ModuleManager::run() {
    // loop over the number of events
    auto number_of_events = global_config_.get<unsigned int>("number_of_events", 1u);
    for(unsigned int i = 0; i < number_of_events; ++i) {
        // go through each module run method every event
        LOG(DEBUG) << "Running event " << (i + 1) << " of " << number_of_events;
        for(auto& mod : modules_) {
            // set run module section header
            std::string old_section_name = Log::getSection();
            std::string section_name = "R:";
            section_name += module_to_id_.at(mod.get()).getName();
            Log::setSection(section_name);
            // run module
            mod->run();
            // reset header
            Log::setSection(old_section_name);
        }
    }
}

// Finalize the modules
void ModuleManager::finalize() {
    // finalize the modules
    for(auto& mod : modules_) {
        // set finalize module section header
        std::string old_section_name = Log::getSection();
        std::string section_name = "F:";
        section_name += module_to_id_.at(mod.get()).getName();
        Log::setSection(section_name);
        // finalize module
        mod->finalize();
        // reset header
        Log::setSection(old_section_name);
    }
}
