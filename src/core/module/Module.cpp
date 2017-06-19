/**
 * @file
 * @brief Implementation of module
 *
 * @copyright MIT License
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

#include <iostream>

using namespace allpix;

Module::Module(Configuration config) : Module(std::move(config), nullptr) {}
Module::Module(Configuration config, std::shared_ptr<Detector> detector)
    : config_(std::move(config)), detector_(std::move(detector)) {}
/**
 * @note The remove_delegate can throw in theory, but this could never happen in practice
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
    std::string unique_name = config_.get<std::string>("_unique_name");
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
 * @warning A local path cannot be fetched from the constructor, because the instantiation logic has not finished yet
 *
 * The output path is automatically created if it does not exists. The path is always accessible if this functions returns.
 */
std::string Module::getOutputPath(const std::string& path, bool global) const {
    std::string file;
    if(global) {
        file = config_.get<std::string>("_global_dir");
    } else {
        file = config_.get<std::string>("_output_dir");
    }

    if(file.empty()) {
        throw InvalidModuleActionException("Cannot access local output path in constructor");
    }

    try {
        // Create all the required main directories
        create_directories(file);

        // Add the file itself
        file += "/";
        file += path;

        // Open the file to check if it can be accessed
        std::fstream file_stream(file, std::ios_base::out | std::ios_base::app);
        if(!file_stream.good()) {
            throw std::invalid_argument("file not accessible");
        }

        // Convert the file to an absolute path
        file = get_absolute_path(file);
    } catch(std::invalid_argument& e) {
        throw ModuleError("Path " + file + " cannot be created");
    }

    return file;
}

/**
 * @warning Cannot be used from the constructor, because the instantiation logic has not finished yet
 * @warning This method should not be accessed from the destructor (the file is then already closed)
 * @note It is not needed to change directory to this file explicitly in the module, this is done automatically.
 */
TDirectory* Module::getROOTDirectory() const {
    if(directory_ == nullptr) {
        throw InvalidModuleActionException("Cannot access ROOT directory in constructor or destructor");
    }

    return directory_;
}
void Module::set_ROOT_directory(TDirectory* directory) {
    directory_ = directory;
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
void Module::reset_delegates() {
    for(auto& delegate : delegates_) {
        delegate.second->reset();
    }
}
bool Module::check_delegates() {
    for(auto& delegate : delegates_) {
        // Return false if any delegate is not satisfied
        if(!delegate.second->isSatisfied()) {
            return false;
        }
    }
    return true;
}
