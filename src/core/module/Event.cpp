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

// Check if the detectors match for the message and the delegate
static bool check_send(BaseMessage* message, BaseDelegate* delegate) {
    if(delegate->getDetector() != nullptr &&
       (message->getDetector() == nullptr || delegate->getDetector()->getName() != message->getDetector()->getName())) {
        return false;
    }
    return true;
}

Event::Event(ModuleList modules,
             const unsigned int event_num,
             std::atomic<bool>& terminate,
             std::condition_variable& master_condition,
             std::map<Module*, long double>& module_execution_time,
             Messenger* messenger,
             std::mt19937_64& seeder)
    : number(event_num), modules_(std::move(modules)), terminate_(terminate), master_condition_(master_condition),
      module_execution_time_(module_execution_time), delegates_(messenger->delegates_) {
    random_engine_.seed(seeder());
#ifndef NDEBUG
    // Ensure that the ID is unique
    assert(unique_ids_.find(event_num) == unique_ids_.end());
    unique_ids_.insert(event_num);
#endif
}

/**
 * Geant4 modules must be run single-threaded on the main thread.
 * Runs all modules up to and including the last Geant4 module and pops them from the \ref Event::modules_ "list of modules".
 */
void Event::run_geant4() {
    // Figure out how many modules must run on the main thread
    auto remove_modules = modules_.size();
    for(auto module = modules_.crbegin(); module != modules_.crend(); module++) {
        if((*module)->getUniqueName().find("Geant4") == std::string::npos) {
            remove_modules--;
        }
    }

    // Ensure we didn't wrap around
    assert(remove_modules < modules_.size());

    for(auto i = remove_modules; i != 0; i--) {
        auto module = modules_.front();
        LOG(DEBUG) << module->getUniqueName() << " is a Geant4 module; running on main thread";
        run(module);
        modules_.pop_front();
    }

        // Below implementation is a modern approach, which works locally, but segfaults constantly on the GitLab CI. Might
        // be worth investigating why.
#if 0
    auto first_after_last_geant4 = [&]() {
        // Find the last Geant4 module from the bottom of the list up
        auto last_geant4 = std::find_if(modules_.crbegin(), modules_.crend(), [](const auto& module) {
            return module->getUniqueName().find("Geant4") != std::string::npos;
        });

        // The first module after the last Geant4 module is where we can safely run the event on another thread
        return *std::prev(last_geant4, 1);
    }();

    // Execute every module up to and including the last Geant4 module
    while(!modules_.empty()) {
        auto module = modules_.front();
        if(module == first_after_last_geant4) {
            // All Geant4 module have been executed
            break;
        }

        LOG(DEBUG) << module->getUniqueName() << " is a Geant4 module; running on main thread";
        run(module);

        modules_.pop_front();
    }
#endif

#ifndef NDEBUG
    // Ensure all Geant4 modules have been removed
    for(auto& module : modules_) {
        assert(module->getUniqueName().find("Geant4") == std::string::npos);
    }
#endif
}

/**
 * Runs modules that read from or write to files in order of increasing event number.
 * The appropriate \ref Event::IOOrderLock "order lock" is operated upon -- counter read/modified and condition watched -- to
 * achieve this.
 * When a previous event has yet to write to file, the current event waits until it's its own turn to write.
 * No work is done while waiting.
 */
bool Event::handle_iomodule(const std::shared_ptr<Module>& module) {
    using namespace std::chrono_literals;

    const bool reader = dynamic_cast<ReaderModule*>(module.get()), writer = dynamic_cast<WriterModule*>(module.get());
    if(previous_was_reader_ && !reader) {
        // All readers have been run for this event, let the next event run its readers
        reader_lock_.next();
    }
    previous_was_reader_ = reader;

    if(!reader && !writer) {
        // Module doesn't require IO; nothing else to do
        return false;
    }
    LOG(DEBUG) << module->getUniqueName() << " is a " << (reader ? "reader" : "writer")
               << "; running in order of event number";

    // Acquire reader/writer lock
    auto& typelock = reader ? reader_lock_ : writer_lock_;
    auto lock = std::unique_lock<std::mutex>(typelock.mutex);
    // Check every 50ms if it's this event's turn to run
    while(!typelock.condition.wait_for(
        lock, 50ms, [this, &typelock]() { return this->number == typelock.current_event.load(); }))
        ;

    return true;
}

/**
 * Runs a single module.
 * The run for a module is skipped if it isn't \ref Event::is_satisfied() "satisfied".
 * Sets the section header and logging settings before exeuting the \ref Module::run() function.
 */
void Event::run(std::shared_ptr<Module>& module) {
    // Modules that read/write files must be run in order of event number
    const bool io_module = handle_iomodule(module);

    auto lock = (!module->canParallelize() || io_module) ? std::unique_lock<std::mutex>(module->run_mutex_)
                                                         : std::unique_lock<std::mutex>();

    LOG_PROGRESS(TRACE, "EVENT_LOOP") << "Running event " << this->number << " [" << module->get_identifier().getUniqueName()
                                      << "]";

    // Check if the module is satisfied to run
    if(!is_satisfied(module.get())) {
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
    for(auto& module : modules_) {
        run(module);
    }

    // All writers have been run for this event, let the next event run its writers
    writer_lock_.next();
}

void Event::dispatch_message(Module* source, std::shared_ptr<BaseMessage> message, std::string name) {
    // Get the name of the output message
    if(name == "-") {
        name = source->get_configuration().get<std::string>("output");
    }

    bool send = false;

    // Send messages to specific listeners
    send = dispatch_message(source, message, name, name) || send;

    // Send to generic listeners
    send = dispatch_message(source, message, name, "*") || send;

    // Display a TRACE log message if the message is send to no receiver
    if(!send) {
        const BaseMessage* inst = message.get();
        LOG(TRACE) << "Dispatched message " << allpix::demangle(typeid(*inst).name()) << " from " << source->getUniqueName()
                   << " has no receivers!";
    }

    // Save a copy of the sent message
    sent_messages_.emplace_back(message);
}

bool Event::dispatch_message(Module* source,
                             const std::shared_ptr<BaseMessage>& message,
                             const std::string& name,
                             const std::string& id) {
    bool send = false;

    // Create type identifier from the typeid
    const BaseMessage* inst = message.get();
    std::type_index type_idx = typeid(*inst);

    // Send messages only to their specific listeners
    for(auto& delegate : delegates_[type_idx][id]) {
        if(check_send(message.get(), delegate.get())) {
            LOG(TRACE) << "Sending message " << allpix::demangle(type_idx.name()) << " from " << source->getUniqueName()
                       << " to " << delegate->getUniqueName();
            // Construct BaseMessage where message should be stored
            auto& dest = messages_[delegate->getUniqueName()];

            delegate->process(message, name, dest);
            satisfied_modules_[delegate->getUniqueName()] = true;
            send = true;
        }
    }

    // Dispatch to base message listeners
    assert(typeid(BaseMessage) != typeid(*inst));
    for(auto& delegate : delegates_[typeid(BaseMessage)][id]) {
        if(check_send(message.get(), delegate.get())) {
            LOG(TRACE) << "Sending message " << allpix::demangle(type_idx.name()) << " from " << source->getUniqueName()
                       << " to generic listener " << delegate->getUniqueName();
            auto& dest = messages_[delegate->getUniqueName()];
            delegate->process(message, name, dest);
            satisfied_modules_[delegate->getUniqueName()] = true;
            send = true;
        }
    }

    return send;
}

bool Event::is_satisfied(Module* module) const {
    // Check delegate flags. If false, check event-local satisfaction.
    try {
        return module->check_delegates() || satisfied_modules_.at(module->getUniqueName());
    } catch(const std::out_of_range&) {
        return false;
    }
}

std::vector<std::pair<std::shared_ptr<BaseMessage>, std::string>> Event::fetchFilteredMessages() {
    return messages_[current_module_->getUniqueName()].filter_multi;
}
