/**
 * @file
 * @brief Implementation of the event wrapper
 *
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "Event.hpp"
#include "MessageStorage.hpp"
#include "Module.hpp"

#include <chrono>
#include <list>
#include <memory>
#include <string>

#include <TProcessID.h>

#include "core/utils/log.h"

using namespace allpix;

Event::Event(ModuleList modules,
             const unsigned int event_num,
             std::atomic<bool>& terminate,
             std::map<Module*, long double>& module_execution_time,
             Messenger* messenger)
    : modules_(std::move(modules)), message_storage_(messenger->delegates_), event_num_(event_num), terminate_(terminate),
      module_execution_time_(module_execution_time) {}

void Event::init() {
    /* LOG_PROGRESS(STATUS, "EVENT_LOOP") << "Initializing event " << event_num_; */

    // Get object count for linking objects in current event
    /* auto save_id = TProcessID::GetObjectCount(); */

    // Execute every Geant4 module
    // XXX: Geant4 modules are only executed if they are at the start of modules_
    while(!modules_.empty()) {

        auto module = modules_.front();
        if(module->getUniqueName().find("Geant4") == std::string::npos) {
            // All Geant4 modules have been executed
            break;
        }

        std::lock_guard<std::mutex> lock(module->run_mutex_);

        // Check if module is satisfied to run
        if(!message_storage_.is_satisfied(module.get())) {
            LOG(TRACE) << "Not all required messages are received for " << module->get_identifier().getUniqueName()
                       << ", skipping module!";
            return;
        }

        // Get current time
        auto start = std::chrono::steady_clock::now();

        // Set run module section header
        std::string old_section_name = Log::getSection();
        std::string section_name = "R:";
        section_name += module->get_identifier().getUniqueName();
        Log::setSection(section_name);

        // Set module specific settings
        auto old_settings =
            ModuleManager::set_module_before(module->get_identifier().getUniqueName(), module->get_configuration());

        // Run module
        try {
            module->run(event_num_, message_storage_.using_module(module.get()));
        } catch(EndOfRunException& e) {
            // Terminate if the module threw the EndOfRun request exception:
            LOG(WARNING) << "Request to terminate:" << std::endl << e.what();
            this->terminate_ = true;
        }

        // Reset logging
        Log::setSection(old_section_name);
        ModuleManager::set_module_after(old_settings);

        // Update execution time
        auto end = std::chrono::steady_clock::now();
        module_execution_time_[module.get()] += static_cast<std::chrono::duration<long double>>(end - start).count();

        /* module->reset_delegates(); */
        modules_.pop_front();
    }

    // Reset object count for next event
    /* TProcessID::SetObjectCount(save_id); */
}

/**
 * Sequentially runs the modules that constitutes the event.
 * The run for a module is skipped if its delegates are not \ref Module::check_delegates() "satisfied".
 * Sets the section header and logging settings before exeuting the \ref Module::run() function.
 * \ref Module::reset_delegates() "Resets" the delegates and the logging after initialization
 */
void Event::run(const unsigned int number_of_events) {
    LOG_PROGRESS(STATUS, "EVENT_LOOP") << "Running event " << event_num_ << " of " << number_of_events;

    // Get object count for linking objects in current event
    /* auto save_id = TProcessID::GetObjectCount(); */

    for(auto& module : modules_) {
        auto lock =
            !module->canParallelize() ? std::unique_lock<std::mutex>(module->run_mutex_) : std::unique_lock<std::mutex>();

        LOG_PROGRESS(TRACE, "EVENT_LOOP") << "Running event " << this->event_num_ << " of " << number_of_events << " ["
                                          << module->get_identifier().getUniqueName() << "]";
        // Check if module is satisfied to run
        if(!message_storage_.is_satisfied(module.get())) {
            LOG(TRACE) << "Not all required messages are received for " << module->get_identifier().getUniqueName()
                       << ", skipping module!";
            return;
        }

        // Get current time
        auto start = std::chrono::steady_clock::now();

        // Set run module section header
        std::string old_section_name = Log::getSection();
        std::string section_name = "R:";
        section_name += module->get_identifier().getUniqueName();
        Log::setSection(section_name);

        // Set module specific settings
        auto old_settings =
            ModuleManager::set_module_before(module->get_identifier().getUniqueName(), module->get_configuration());

        // Run module
        try {
            module->run(event_num_, message_storage_.using_module(module.get()));
        } catch(EndOfRunException& e) {
            // Terminate if the module threw the EndOfRun request exception:
            LOG(WARNING) << "Request to terminate:" << std::endl << e.what();
            this->terminate_ = true;
        }

        // Reset logging
        Log::setSection(old_section_name);
        ModuleManager::set_module_after(old_settings);

        // Update execution time
        auto end = std::chrono::steady_clock::now();
        module_execution_time_[module.get()] += static_cast<std::chrono::duration<long double>>(end - start).count();
    }

    // Resetting delegates
    for(auto& module : modules_) {
        LOG(TRACE) << "Resetting messages";
        auto lock =
            !module->canParallelize() ? std::unique_lock<std::mutex>(module->run_mutex_) : std::unique_lock<std::mutex>();
        module->reset_delegates();
    }

    // Reset object count for next event
    /* TProcessID::SetObjectCount(save_id); */
}

void Event::finalize() {
    // stub
}
