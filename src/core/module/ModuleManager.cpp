/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "ModuleManager.hpp"

using namespace allpix;

// Constructor and destructor
ModuleManager::ModuleManager(): modules_(), identifiers_(), run_queue_() {}
ModuleManager::~ModuleManager() = default;

// Run all the modules in the queue
void ModuleManager::run() {
    // go through the run queue as long it is not empty
    while (!run_queue_.empty()) {
        Module *mod = run_queue_.front();
        run_queue_.pop();

        mod->run();
    }
}

// Finalize the modules
void ModuleManager::finalize() {
    // finalize the modules
    for (auto &mod : modules_) {
        mod->finalize();
    }
}

// Add a module to the run queue (NOTE: this can just be inlined if it remains simple)
void ModuleManager::add_to_run_queue(Module *mod) {
    run_queue_.push(mod);
}
