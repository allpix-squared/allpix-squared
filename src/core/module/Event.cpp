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

Event::Event(ModuleList modules,
             const unsigned int event_num,
             std::atomic<bool>& terminate,
             std::map<Module*, long double>& module_execution_time,
             Messenger* messenger)
    : modules_(modules), message_storage_(messenger->delegates_), event_num_(event_num), terminate_(terminate), module_execution_time_(module_execution_time) {}

// Check if the detectors match for the message and the delegate
static bool check_send(BaseMessage* message, BaseDelegate* delegate) {
    if(delegate->getDetector() != nullptr &&
       (message->getDetector() == nullptr || delegate->getDetector()->getName() != message->getDetector()->getName())) {
        return false;
    }
    return true;
}

void Event::MessageStorage::append(Module* source, std::vector<std::pair<std::shared_ptr<BaseMessage>, std::string>> messages) {
    if(messages.empty()) {
        return;
    }

    for(auto message : messages) {
        dispatch_message(source, message.first, message.second);
    }

    /* LOG(WARNING) << "Appended " << messages.size() << " messages to storage."; */
}

void Event::MessageStorage::dispatch_message(Module* source, std::shared_ptr<BaseMessage> message, std::string name) {
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

bool Event::MessageStorage::dispatch_message(Module* source, const std::shared_ptr<BaseMessage>& message, const std::string& name, const std::string& id) {
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
            send = true;
        }
    }

    return send;
}

DelegateVariants& Event::MessageStorage::fetch_for(Module* module) {
    return messages_[module->getUniqueName()];
}

void Event::init() {
    /* LOG_PROGRESS(STATUS, "EVENT_LOOP") << "Initializing event " << event_num_; */

    // Get object count for linking objects in current event
    /* auto save_id = TProcessID::GetObjectCount(); */

    // Execute every Geant4 module
    // XXX: Geant4 modules are only executed if they are at the start of modules_
    while(modules_.size() > 0) {

        auto module = modules_.front();
        if(module->getUniqueName().find("Geant4") == std::string::npos) {
            // All Geant4 modules have been executed
            break;
        }

        std::lock_guard<std::mutex> lock(module->run_mutex_);

        // Check if module is satisfied to run
        // TODO: rewrite this for event-local messages!
#if 0
        if(!module->check_delegates(event_num_)) {
            LOG(TRACE) << "Not all required messages are received for " << module->get_identifier().getUniqueName()
                       << ", skipping module!";
            return;
        }
#endif

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
            auto input_msgs = message_storage_.fetch_for(module.get());
            auto output_msgs = module->run(event_num_, input_msgs);
            message_storage_.append(module.get(), output_msgs);
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
#if 0
        if(!module->check_delegates(event_num_)) {
            LOG(TRACE) << "Not all required messages are received for " << module->get_identifier().getUniqueName()
                       << ", skipping module!";
            return;
        }
#endif

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
            auto input_msgs = message_storage_.fetch_for(module.get());
            auto output_msgs = module->run(this->event_num_, input_msgs);
            message_storage_.append(module.get(), output_msgs);
        } catch(EndOfRunException& e) {
            // Terminate if the module threw the EndOfRun request exception:
            LOG(WARNING) << "Request to terminate:" << std::endl << e.what();
            this->terminate_ = true;
        } catch(std::out_of_range&) {
            throw InvalidMessageIDException(module->getUniqueName(), event_num_);
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
