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

#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <utility>

#include "core/messenger/Messenger.hpp"
#include "core/module/exceptions.h"
#include "core/utils/log.h"

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
std::string Module::createOutputFile(const std::string& path, const std::string& extension, bool global, bool delete_file) {
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
        std::filesystem::create_directories(file);

        // Add the file itself
        file += "/";
        file +=
            (extension.empty() ? path : static_cast<std::string>(std::filesystem::path(path).replace_extension(extension)));

        if(std::filesystem::is_regular_file(file)) {
            auto global_overwrite = getConfigManager()->getGlobalConfiguration().get<bool>("deny_overwrite", false);
            if(config_.get<bool>("deny_overwrite", global_overwrite)) {
                throw ModuleError("Overwriting of existing file " + file + " denied.");
            }
            LOG(WARNING) << "File " << file << " exists and will be overwritten.";
            try {
                std::filesystem::remove(file);
            } catch(std::filesystem::filesystem_error& e) {
                throw ModuleError("Deleting file " + file + " failed: " + e.what());
            }
        }

        // Open the file to check if it can be accessed
        std::fstream file_stream(file, std::ios_base::out | std::ios_base::app);
        if(!file_stream.good()) {
            throw ModuleError("File " + file + " not accessible");
        }

        // Convert the file to an absolute path
        file = std::filesystem::canonical(file);
    } catch(std::filesystem::filesystem_error& e) {
        throw ModuleError("Path " + file + " cannot be created");
    }

    if(delete_file) {
        std::filesystem::remove(file);
    }
    return file;
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

bool Module::multithreadingEnabled() const {
    return multithreading_;
}
void Module::allow_multithreading() {
    multithreading_ = true;
}
void Module::set_multithreading(bool multithreading) {
    multithreading_ = multithreading;
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

void SequentialModule::waive_sequence_requirement() {
    sequence_required_ = false;
}
