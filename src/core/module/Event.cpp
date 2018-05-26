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
#include "Module.hpp"

#include <string>
#include <chrono>
#include <memory>
#include <list>

#include <TProcessID.h>

#include "core/utils/log.h"

using namespace allpix;

Event::Event(ModuleList &modules, const unsigned int event_num, std::atomic<bool> &terminate)
    : modules_(modules), event_num_(event_num), terminate_(terminate) {}

void Event::init()
{
    // stub
}

/**
 * Sequentially runs the modules that constitutes the event.
 * The run for a module is skipped if its delegates are not \ref Module::check_delegates() "satisfied".
 * Sets the section header and logging settings before exeuting the \ref Module::run() function.
 * \ref Module::reset_delegates() "Resets" the delegates and the logging after initialization
 */
void Event::run(const unsigned int number_of_events, std::map<Module*, long double> &module_execution_time)
{
    LOG_PROGRESS(STATUS, "EVENT_LOOP") << "Running event " << event_num_ << " of " << number_of_events;

    // Get object count for linking objects in current event
    auto save_id = TProcessID::GetObjectCount();

    for (auto& module : modules_) {
        // clang-format off
        auto execute_module = [module = module.get(), this, number_of_events, &module_execution_time]() {
            std::lock_guard<std::mutex> lock(module->run_mutex_);

            // clang-format on
            LOG_PROGRESS(TRACE, "EVENT_LOOP") << "Running event " << this->event_num_ << " of " << number_of_events << " ["
                                              << module->get_identifier().getUniqueName() << "]";
            // Check if module is satisfied to run
            if(!module->check_delegates()) {
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
            auto old_settings = ModuleManager::set_module_before(module->get_identifier().getUniqueName(), module->get_configuration());

            // Run module
            try {
                module->run(this->event_num_);
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
            module_execution_time[module] += static_cast<std::chrono::duration<long double>>(end - start).count();
        };

        // Execute current module
        execute_module();
    }

    // Resetting delegates
    for(auto& module : modules_) {
        LOG(TRACE) << "Resetting messages";
        std::lock_guard<std::mutex> lock(module->run_mutex_);
        module->reset_delegates();
    }

    // Reset object count for next event
    TProcessID::SetObjectCount(save_id);
}

void Event::finalize()
{
    // stub
}
