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
#include "core/utils/log.h"

using namespace allpix;

std::mutex Event::stats_mutex_;
Event::IOOrderLock Event::reader_lock_;
Event::IOOrderLock Event::writer_lock_;

#ifndef NDEBUG
std::set<unsigned int> Event::unique_ids_;
#endif

Event::Event(ModuleList modules,
             const unsigned int event_num,
             Messenger& global_messenger,
             std::atomic<bool>& terminate,
             std::condition_variable& master_condition,
             std::map<Module*, long double>& module_execution_time,
             std::mt19937_64& seeder)
    : number(event_num), modules_(std::move(modules)), terminate_(terminate), master_condition_(master_condition),
      module_execution_time_(module_execution_time), messenger_(global_messenger) {
    random_engine_.seed(seeder());
#ifndef NDEBUG
    // Ensure that the ID is unique
    assert(unique_ids_.find(event_num) == unique_ids_.end());
    unique_ids_.insert(event_num);
#endif
}

/**
 * Runs modules that read from or write to files in order of increasing event number.
 * The appropriate \ref Event::IOOrderLock "order lock" is operated upon -- counter read/modified and condition watched -- to
 * achieve this.
 * When a previous event has yet to write to file, the current event waits until it's its own turn to write.
 * No work is done while waiting.
 */
void Event::handle_iomodule(const std::shared_ptr<Module>& module) {
    using namespace std::chrono_literals;

    const bool reader = dynamic_cast<ReaderModule*>(module.get()) != nullptr,
               writer = dynamic_cast<WriterModule*>(module.get()) != nullptr;
    if(previous_was_reader_ && !reader) {
        // All readers have been run for this event, let the next event run its readers
        reader_lock_.next();
    }
    previous_was_reader_ = reader;

    if(!reader && !writer) {
        // Module doesn't require IO; nothing else to do
        return;
    }
    LOG(DEBUG) << module->getUniqueName() << " is a " << (reader ? "reader" : "writer")
               << "; running in order of event number";

    // Acquire reader/writer lock
    auto& typelock = reader ? reader_lock_ : writer_lock_;
    auto lock = std::unique_lock<std::mutex>(typelock.mutex);
    // Check every 50ms if it's this event's turn to run
    // TODO [doc] don't pseudo-busy loop
    while(!typelock.condition.wait_for(
        lock, 50ms, [this, &typelock]() { return this->number == typelock.current_event.load(); })) {
    };
}

Event* Event::with_context(const std::shared_ptr<Module>& module) {
    current_module_ = module.get();
    return this;
}

/**
 * Runs a single module.
 * The run for a module is skipped if it isn't \ref Event::is_satisfied() "satisfied".
 * Sets the section header and logging settings before exeuting the \ref Module::run() function.
 */
void Event::run(std::shared_ptr<Module>& module) {
    // Modules that read/write files must be run in order of event number
    handle_iomodule(module);

    LOG_PROGRESS(TRACE, "EVENT_LOOP") << "Running event " << this->number << " [" << module->get_identifier().getUniqueName()
                                      << "]";

    // Check if the module is satisfied to run
    if(!module->check_delegates(&messenger_)) {
        LOG(TRACE) << "Not all required messages are received for " << module->get_identifier().getUniqueName()
                   << ", skipping module!";
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
        module->run(with_context(module));
    } catch(const EndOfRunException& e) {
        // Terminate if the module threw the EndOfRun request exception:
        LOG(WARNING) << "Request to terminate:" << std::endl << e.what();
        this->terminate_ = true;
        // Notify master thread that we wish to terminate
        master_condition_.notify_one();
    }

    // Reset logging
    Log::setSection(old_section_name);
    Log::setEventNum(old_event_num);
    ModuleManager::set_module_after(old_settings);

    // Update execution time
    auto end = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> stat_lock{stats_mutex_};
    module_execution_time_[module.get()] += static_cast<std::chrono::duration<long double>>(end - start).count();
}

void Event::run() {
    messenger_.reset();

    for(auto& module : modules_) {
        run(module);
    }

    // All writers have been run for this event, let the next event run its writers
    writer_lock_.next();
}
