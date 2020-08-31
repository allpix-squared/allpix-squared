/**
 * @file
 * @brief Implementation of module
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "Module.hpp"

#include <fstream>
#include <memory>
#include <stdexcept>
#include <utility>

#include "core/messenger/Messenger.hpp"
#include "core/utils/file.h"
#include "core/utils/log.h"
#include "exceptions.h"

using namespace allpix;

Module::Module(Configuration& config) : Module(config, nullptr) {}
Module::Module(Configuration& config, std::shared_ptr<Detector> detector)
    : config_(config), detector_(std::move(detector)) {}
/**
 * @note The remove_delegate can throw in theory, but this should never happen in practice
 */
Module::~Module() {
    // Remove delegates
    try {
        for(auto delegate : delegates_) {
            delegate.first->remove_delegate(delegate.second);
        }
    } catch(std::out_of_range&) {
        LOG(FATAL) << "Internal fault, cannot delete bound message (should never happen)";
        std::abort();
    }
}

/**
 * @throws InvalidModuleActionException If this method is called from the constructor
 *
 * This name is guaranteed to be unique for every single instantiation of all modules
 */
std::string Module::getUniqueName() const {
    std::string unique_name = get_identifier().getUniqueName();
    if(unique_name.empty()) {
        throw InvalidModuleActionException("Cannot uniquely identify module in constructor");
    }
    return unique_name;
}

/**
 * Detector modules always have a linked detector and unique modules are guaranteed not to have one
 */
std::shared_ptr<Detector> Module::getDetector() const {
    return detector_;
}

/**
 * @throws ModuleError If the file cannot be accessed (or created if it did not yet exist)
 * @throws InvalidModuleActionException If this method is called from the constructor with the global flag false
 * @throws ModuleError If the file exists but the "deny_overwrite" flag is set to true (defaults to false)
 * @warning A local path cannot be fetched from the constructor, because the instantiation logic has not finished yet
 *
 * The output path is automatically created if it does not exists. The path is always accessible if this functions returns.
 * Obeys the "deny_overwrite" parameter of the module.
 */
std::string Module::createOutputFile(const std::string& path, bool global, bool delete_file) {
    std::string file;
    if(global) {
        file = config_.get<std::string>("_global_dir", std::string());
    } else {
        file = config_.get<std::string>("_output_dir", std::string());
    }

    // The file name will only be empty if this method is executed from the constructor
    if(file.empty()) {
        throw InvalidModuleActionException("Cannot access local output path in constructor");
    }

    try {
        // Create all the required main directories
        allpix::create_directories(file);

        // Add the file itself
        file += "/";
        file += path;

        if(path_is_file(file)) {
            auto global_overwrite = getConfigManager()->getGlobalConfiguration().get<bool>("deny_overwrite", false);
            if(config_.get<bool>("deny_overwrite", global_overwrite)) {
                throw ModuleError("Overwriting of existing file " + file + " denied.");
            }
            LOG(WARNING) << "File " << file << " exists and will be overwritten.";
            try {
                allpix::remove_file(file);
            } catch(std::invalid_argument& e) {
                throw ModuleError("Deleting file " + file + " failed: " + e.what());
            }
        }

        // Open the file to check if it can be accessed
        std::fstream file_stream(file, std::ios_base::out | std::ios_base::app);
        if(!file_stream.good()) {
            throw std::invalid_argument("file not accessible");
        }

        // Convert the file to an absolute path
        file = get_canonical_path(file);
    } catch(std::invalid_argument& e) {
        throw ModuleError("Path " + file + " cannot be created");
    }

    if(delete_file) {
        allpix::remove_file(file);
    }
    return file;
}

/**
 * @throws InvalidModuleActionException If this method is called from the constructor or destructor
 * @warning This should not be used in \ref run method to allow reproducing the results
 */
uint64_t Module::getRandomSeed() {
    if(random_generator_ == nullptr) {
        throw InvalidModuleActionException("Cannot use this generator outside of \"init\" method");
    }

    return (*random_generator_)();
}
void Module::set_random_generator(MersenneTwister* random_generator) {
    random_generator_ = random_generator;
}

/**
 * @throws InvalidModuleActionException If this method is called from the constructor or destructor
 * @warning Cannot be used from the constructor, because the instantiation logic has not finished yet
 * @warning This method should not be accessed from the destructor (the file is then already closed)
 * @note It is not needed to change directory to this file explicitly in the module, this is done automatically.
 */
TDirectory* Module::getROOTDirectory() const {
    // The directory will only be a null pointer if this method is executed from the constructor or destructor
    if(directory_ == nullptr) {
        throw InvalidModuleActionException("Cannot access ROOT directory in constructor or destructor");
    }

    return directory_;
}
void Module::set_ROOT_directory(TDirectory* directory) {
    directory_ = directory;
}

/**
 * @throws InvalidModuleActionException If this method is called from the constructor or destructor
 * @warning This function technically allows to write to the configurations of other modules, but this should never be done
 */
ConfigManager* Module::getConfigManager() const {
    if(conf_manager_ == nullptr) {
        throw InvalidModuleActionException("Cannot access the config manager in constructor or destructor.");
    };
    return conf_manager_;
}
void Module::set_config_manager(ConfigManager* conf_manager) {
    conf_manager_ = conf_manager;
}

bool Module::canParallelize() {
    return parallelize_;
}
void Module::enable_parallelization() {
    parallelize_ = true;
}
void Module::set_parallelize(bool parallelize) {
    parallelize_ = parallelize;
}

Configuration& Module::get_configuration() {
    return config_;
}

void Module::set_identifier(ModuleIdentifier identifier) {
    identifier_ = std::move(identifier);
}
ModuleIdentifier Module::get_identifier() const {
    return identifier_;
}

void Module::add_delegate(Messenger* messenger, BaseDelegate* delegate) {
    delegates_.emplace_back(messenger, delegate);
}
bool Module::check_delegates(Messenger* messenger, Event* event) {
    // Return false if any delegate is not satisfied
    return std::all_of(delegates_.cbegin(), delegates_.cend(), [messenger, event](auto& delegate) {
        return !delegate.second->isRequired() || messenger->isSatisfied(delegate.second, event);
    });
}

size_t BufferedModule::max_buffer_size_ = 128;

void BufferedModule::run_in_order(std::shared_ptr<Event> event) { // NOLINT
    auto event_number = event->number;

    {
        // Store the event in the buffer
        std::unique_lock<std::mutex> lock(buffer_mutex_);
        cond_var_.wait(lock, [this, event_number]() {
            return event_number == next_event_to_write_ || buffered_events_.size() < max_buffer_size_;
        });

        LOG(TRACE) << "Buffering event " << event->number;

        // Buffer out of order events to write them later
        buffered_events_.insert(std::make_pair(event_number, event));
    }

    // Allow only one write to run at a time
    if(writer_mutex_.try_lock()) {
        // Flush any in order buffered events
        flush_buffered_events();
        writer_mutex_.unlock();
    }
}

void BufferedModule::finalize_buffer() {
    // Write any remaining buffered events
    flush_buffered_events();

    // Make sure everything was written
    if(!buffered_events_.empty()) {
        std::string msg = std::to_string(buffered_events_.size()) + " buffered event(s) where not written to output";
        throw ModuleError(msg);
    }

    // Call user finalize
    finalize();
}

void BufferedModule::flush_buffered_events() {
    // Special random generator for events executed out of order
    static thread_local MersenneTwister random_generator;

    std::map<uint64_t, std::shared_ptr<Event>>::iterator iter;
    std::shared_ptr<Event> event;

    next_event_to_write_ = get_next_event(next_event_to_write_);
    while(true) {
        {
            // Find the next event to write
            std::lock_guard<std::mutex> lock(buffer_mutex_);
            iter = buffered_events_.find(next_event_to_write_);

            if(iter == buffered_events_.end()) {
                break;
            }

            event = std::move(iter->second);
            iter->second.reset();

            // Remove it from buffer
            buffered_events_.erase(iter);
            cond_var_.notify_one();
        }

        LOG(TRACE) << "Writing buffered event " << iter->first << ", " << buffered_events_.size() << " left in buffer";

        // set the buffered event RNG
        event->set_and_seed_random_engine(&random_generator);

        // Process the buffered event
        run(event.get());

        // Move to the next event
        next_event_to_write_ = get_next_event(next_event_to_write_ + 1);
    }
}

uint64_t BufferedModule::get_next_event(uint64_t current) {
    std::set<uint64_t>::iterator iter;
    auto next = current;

    // Check sequentially if events were skipped
    std::lock_guard<std::mutex> lock(skipped_events_mutex_);
    while((iter = skipped_events_.find(next)) != skipped_events_.end()) {
        LOG(TRACE) << "Event skipped " << *iter;

        // Remove from the skipped set because no point of checking it again
        skipped_events_.erase(iter);

        // Move on...
        next++;
    }
    return next;
}

void BufferedModule::skip_event(uint64_t event) {
    std::lock_guard<std::mutex> lock(skipped_events_mutex_);
    skipped_events_.insert(event);
}
