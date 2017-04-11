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
        mod->init();
    }
}

// Run all the modules in the queue
void ModuleManager::run() {
    // loop over the number of events
    auto number_of_events = global_config_.get<unsigned int>("number_of_events", 1u);
    for(unsigned int i = 0; i < number_of_events; ++i) {
        LOG(DEBUG) << "Running event " << (i + 1) << " of " << number_of_events;
        // go through each module run method every event
        for(auto& mod : modules_) {
            mod->run();
        }
    }
}

// Finalize the modules
void ModuleManager::finalize() {
    // finalize the modules
    for(auto& mod : modules_) {
        mod->finalize();
    }
}
