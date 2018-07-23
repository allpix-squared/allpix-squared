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

#include <chrono>
#include <list>
#include <memory>
#include <string>

#include <TProcessID.h>

#include "core/utils/log.h"

using namespace allpix;
using namespace std::chrono_literals;

std::mutex Event::stats_mutex_;
Event::IOLock Event::reader_lock_;
Event::IOLock Event::writer_lock_;

Event::Event(ModuleList modules,
             const unsigned int event_num,
             std::atomic<bool>& terminate,
             std::map<Module*, long double>& module_execution_time,
             Messenger* messenger,
             std::mt19937_64& seeder)
    : number(event_num), modules_(std::move(modules)), terminate_(terminate), module_execution_time_(module_execution_time),
      delegates_(messenger->delegates_) {
    random_generator_.seed(seeder());
}

bool Event::handle_iomodule(const std::shared_ptr<Module>& module) {
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
    LOG(TRACE) << module->getUniqueName() << " is a " << (reader ? "reader" : "writer")
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

void Event::run(std::shared_ptr<Module>& module) {
    // Modules that read/write files must be run in order of event number
    const bool io_module = handle_iomodule(module);

    auto lock = (!module->canParallelize() || io_module) ? std::unique_lock<std::mutex>(module->run_mutex_)
                                                         : std::unique_lock<std::mutex>();

    LOG_PROGRESS(TRACE, "EVENT_LOOP") << "Running event " << this->number << " [" << module->get_identifier().getUniqueName()
                                      << "]";

    // Check if module is satisfied to run
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

void Event::init() {
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

        run(module);

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
void Event::run() {
    // Get object count for linking objects in current event
    /* auto save_id = TProcessID::GetObjectCount(); */

    for(auto& module : modules_) {
        run(module);
    }

    // Resetting delegates
    // XXX: is this required?
    for(auto& module : modules_) {
        LOG(TRACE) << "Resetting messages";
        auto lock =
            !module->canParallelize() ? std::unique_lock<std::mutex>(module->run_mutex_) : std::unique_lock<std::mutex>();
        module->reset_delegates();
    }

    // Reset object count for next event
    /* TProcessID::SetObjectCount(save_id); */

    // All writers have been run for this event, let the next event run its writers
    writer_lock_.next();
}

void Event::finalize() {
    // stub
}

// Check if the detectors match for the message and the delegate
static bool check_send(BaseMessage* message, BaseDelegate* delegate) {
    if(delegate->getDetector() != nullptr &&
       (message->getDetector() == nullptr || delegate->getDetector()->getName() != message->getDetector()->getName())) {
        return false;
    }
    return true;
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
    if(module->check_delegates()) {
        return true;
    }

    try {
        return satisfied_modules_.at(module->getUniqueName());
    } catch(const std::out_of_range&) {
        return false;
    }
}
