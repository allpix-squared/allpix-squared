/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
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
Module::Module(std::shared_ptr<Detector> detector)
    : output_directory_(), global_directory_(), identifier_(), config_(), delegates_(), detector_(std::move(detector)) {}

// Get the detector
std::shared_ptr<Detector> Module::getDetector() {
    return detector_;
}

// Get output path
std::string Module::getOutputPath(const std::string& path, bool global) {
    std::string file;
    if(global) {
        file = global_directory_;
    } else {
        file = output_directory_;
    }

    try {
        if(file.empty()) {
            throw InvalidModuleActionException("Output path cannot be retrieved from within the constructor");
        }

        // create the directories if needed
        create_directories(file);

        // add the file itself
        file += "/";
        file += path;

        // open the file to check if you can read it
        std::fstream file_stream(file, std::ios_base::out | std::ios_base::app);
        if(!file_stream.good()) {
            throw std::invalid_argument("file not accessible");
        }

        // convert the file to an absolute path
        file = get_absolute_path(file);
    } catch(std::invalid_argument& e) {
        throw ModuleError("Path " + file + " cannot be accessed (or created if it did not yet exist)");
    }

    return file;
}
void Module::set_output_directory(std::string output_dir) {
    output_directory_ = std::move(output_dir);
}
void Module::set_global_directory(std::string output_dir) {
    global_directory_ = std::move(output_dir);
}

// Set internal config
void Module::set_configuration(Configuration config) {
    config_ = std::move(config);
}
Configuration Module::get_configuration() {
    return config_;
}

// Set internal identifier
void Module::set_identifier(ModuleIdentifier identifier) {
    identifier_ = std::move(identifier);
}
ModuleIdentifier Module::get_identifier() {
    return identifier_;
}

// Add delegate
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
        // Return if any delegate is not satisfied
        if(!delegate->isSatisfied()) {
            return false;
        }
    }
    return true;
}
