/**
 * @file
 * @brief Implementation of event
 *
 * @copyright Copyright (c) 2018 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "Event.hpp"

#include <algorithm>
#include <chrono>
#include <list>
#include <memory>
#include <string>

#include "Module.hpp"
#include "ModuleManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/utils/log.h"

using namespace allpix;

std::mutex Event::stats_mutex_;
event_context* Event::context_ = nullptr;

Event::Event(const unsigned int event_num, uint64_t seed) : number(event_num), seed_(seed) {
#ifndef NDEBUG
    assert(context_ != nullptr);
#endif

    local_messenger_ = std::make_unique<LocalMessenger>(context_->messenger_);
}

/**
 * Runs a single module.
 * The run for a module is skipped if it isn't \ref Event::is_satisfied() "satisfied".
 * Sets the section header and logging settings before exeuting the \ref Module::run() function.
 */
void Event::run(std::shared_ptr<Module>& module) {
    LOG_PROGRESS(TRACE, "EVENT_LOOP") << "Running event " << this->number << " [" << module->get_identifier().getUniqueName()
                                      << "]";

    // Check if the module is satisfied to run
    if(!module->check_delegates(&context_->messenger_, this)) {
        LOG(TRACE) << "Not all required messages are received for " << module->get_identifier().getUniqueName()
                   << ", skipping module!";
        module->skip_event(number);
        return;
    }

    // Get current time
    auto start = std::chrono::steady_clock::now();

    // Set run module section header
    std::string old_section_name = Log::getSection();
    unsigned int old_event_num = Log::getEventNum();
    std::string section_name = "R:";
    section_name += module->get_identifier().getUniqueName();
    Log::setSection(section_name);
    Log::setEventNum(this->number);

    // Set module specific settings
    auto old_settings =
        ModuleManager::set_module_before(module->get_identifier().getUniqueName(), module->get_configuration());

    // Run module
    try {
        module->run(this);
    } catch(const EndOfRunException& e) {
        // Terminate if the module threw the EndOfRun request exception:
        LOG(WARNING) << "Request to terminate:" << std::endl << e.what();
        context_->terminate_ = true;
    }

    // Reset logging
    Log::setSection(old_section_name);
    Log::setEventNum(old_event_num);
    ModuleManager::set_module_after(old_settings);

    // Update execution time
    auto end = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> stat_lock{stats_mutex_};
    context_->module_execution_time_[module.get()] += static_cast<std::chrono::duration<long double>>(end - start).count();
}

void Event::run() {
    for(auto& module : context_->modules_) {
        run(module);
    }
}

void Event::set_and_seed_random_engine(std::mt19937_64* random_engine) {
    random_engine_ = random_engine;
    random_engine_->seed(seed_);
}

LocalMessenger* Event::get_local_messenger() const {
    return local_messenger_.get();
}
