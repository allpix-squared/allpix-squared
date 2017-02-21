/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "ModuleManager.hpp"

using namespace allpix;

void ModuleManager::run() {
    // go through the run queue as long it is not empty
    while (!run_queue_.empty()) {
        Module *mod = run_queue_.front();
        run_queue_.pop();
        
        mod->run();
    }
}

void ModuleManager::finalize() {
    // finalize the modules
    for (auto &mod : modules_) {
        mod->finalize();
    }
}

void ModuleManager::add_to_run_queue(Module *mod) {
    run_queue_.push(mod);
}
