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
#include "exceptions.h"

#include <iostream>

using namespace allpix;

Module::Module() : Module(nullptr) {}
Module::Module(std::shared_ptr<Detector> detector) : detector_(std::move(detector)) {}

/**
 * @throws InvalidModuleActionException If this method is called from the constructor
 *
 * This name is guaranteed to be unique for every single instantiation of all modules
 */
std::string Module::getUniqueName() {
    std::string unique_name = identifier_.getUniqueName();
    if(unique_name.empty()) {
        throw InvalidModuleActionException("Unique name cannot be retrieved from within the constructor");
    }
    return unique_name;
}

/**
 * Detector modules always have a linked detector and unique modules are guaranteed not to have one
 */
std::shared_ptr<Detector> Module::getDetector() {
    return detector_;
}

/**
 * @throws InvalidModuleActionException If this method is called from the constructor
 * @throws ModuleError If the file cannot be accessed (or created if it did not yet exist)
 *
 * The output path is automatically created if it does not exists. The path is always accessible if this functions returns.
 */
// TODO [DOC] This function does not support subdirectories
std::string Module::getOutputPath(const std::string& path, bool global) {
    std::string file;
    if(global) {
        file = global_directory_;
    } else {
        file = output_directory_;
    }
    if(file.empty()) {
        throw InvalidModuleActionException("Output path cannot be retrieved from within the constructor");
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

// Set internal directories
void Module::set_output_directory(std::string output_dir) {
    output_directory_ = std::move(output_dir);
}
void Module::set_global_directory(std::string output_dir) {
    global_directory_ = std::move(output_dir);
}

// Getters and setters for internal configuration
void Module::set_configuration(Configuration config) {
    config_ = std::move(config);
}
Configuration Module::get_configuration() {
    return config_;
}

// Getters and setters for internal identifier
void Module::set_identifier(ModuleIdentifier identifier) {
    identifier_ = std::move(identifier);
}
ModuleIdentifier Module::get_identifier() {
    return identifier_;
}

// Add messenger delegate
void Module::add_delegate(BaseDelegate* delegate) {
    delegates_.emplace_back(delegate);
}
// Reset all delegates
void Module::reset_delegates() {
    for(auto& delegate : delegates_) {
        delegate->reset();
    }
}
// Check if all delegates are satisfied
bool Module::check_delegates() {
    for(auto& delegate : delegates_) {
        // Return false if any delegate is not satisfied
        if(!delegate->isSatisfied()) {
            return false;
        }
    }
    return true;
}
