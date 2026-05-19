/**
 * @file
 * @brief Histogram Manager for all interactions with histograms
 *
 * @copyright Copyright (c) 2026-2026 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "HistogramManager.hpp"

using namespace allpix;

HistogramManager::HistogramManager(const std::string& file_path, bool deny_overwrite) {

    // (Re-)create histogram ROOT file
    auto path = std::filesystem::path(gSystem->pwd()) / file_path;
    path.replace_extension("root");

    if(std::filesystem::is_regular_file(path)) {
        if(deny_overwrite) {
            throw RuntimeError("Overwriting of existing main ROOT file " + path.string() + " denied");
        }
        LOG(WARNING) << "Main ROOT file " << path << " exists and will be overwritten.";
        std::filesystem::remove(path);
    }

    LOG(INFO) << "Creating main ROOT file: " << path;

    histogram_file_ = std::make_unique<TFile>(path.c_str(), "RECREATE");
    if(histogram_file_->IsZombie()) {
        throw RuntimeError("Cannot create main ROOT file " + path.string());
    }
    histogram_file_->cd();

    LOG(TRACE) << "Opened main ROOT file.";
}

HistogramManager::~HistogramManager() {
    // Close file
    histogram_file_->Close();
}

TDirectory* HistogramManager::registerModuleDirectory(const std::string& module_name, const std::string& module_identifier) {
    // Create main ROOT directory for this module class if it does not exist yet
    LOG(TRACE) << "Creating and accessing ROOT directory";
    auto* module_directory = histogram_file_->GetDirectory(module_name.c_str());
    if(module_directory == nullptr) {
        module_directory = histogram_file_->mkdir(module_name.c_str());
        if(module_directory == nullptr) {
            throw LogicError("Cannot create or access overall ROOT directory for module " + module_name);
        }
    }
    module_directory->cd();

    // Create local directory for this instance
    TDirectory* unique_module_directory = nullptr;
    if(module_identifier.empty()) {
        unique_module_directory = module_directory;
    } else {
        unique_module_directory = module_directory->GetDirectory(module_identifier.c_str());
        if(unique_module_directory == nullptr) {
            unique_module_directory = module_directory->mkdir(module_identifier.c_str());
        }
    }

    if(unique_module_directory == nullptr) {
        throw RuntimeError("Cannot create or access local ROOT directory for module " + module_name);
    }

    // Change to the directory
    unique_module_directory->cd();

    return unique_module_directory;
}

TDirectory* HistogramManager::registerSubdirectory(TDirectory* unique_module_directory,
                                                   const std::string& subdirectory_name = "") {
    if(unique_module_directory == nullptr) {
        throw LogicError("ROOT directory for unique module should be present but cannot be accessed.");
    }

    // Create subdirectory if requested
    TDirectory* subdirectory = nullptr;
    if(subdirectory_name.empty()) {
        subdirectory = unique_module_directory;
    } else {
        subdirectory = unique_module_directory->GetDirectory(subdirectory_name.c_str());
        if(subdirectory == nullptr) {
            subdirectory = unique_module_directory->mkdir(subdirectory_name.c_str());
        }
    }

    if(subdirectory == nullptr) {
        throw RuntimeError("Cannot create or access ROOT subdirectory: " + subdirectory_name);
    }

    return subdirectory;
}

TDirectory* HistogramManager::registerGenericPath(const std::string& path) {
    // Create ROOT directory for this path
    LOG(TRACE) << "Creating or accessing ROOT directory: " << path;

    auto* directory = histogram_file_->GetDirectory(path.c_str());
    if(directory == nullptr) {
        directory = histogram_file_->mkdir(path.c_str());
    }

    if(directory == nullptr) {
        throw RuntimeError("Cannot create or access ROOT directory: " + path);
    }

    directory->cd();

    return directory;
}

void HistogramManager::finalize() {
    LOG(INFO) << "Finalizing histograms.";

    // Only if file is open
    if(histogram_file_->IsZombie()) {
        LOG(ERROR) << "Histogram file is not open. Cannot finalize.";
        return;
    }

    for(auto& it : histogram_list_) {
        if(!(it.second)) {
            throw LogicError("A histogram in the local directory " + it.first + " can no longer be accessed.");
        }
        LOG(TRACE) << "Writing histogram " + it.second->GetName() + " to path " << it.first;
        histogram_file_->cd(it.first.c_str());
        it.second->write();
    }
}
