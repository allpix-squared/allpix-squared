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

#ifndef ALLPIX_HISTOGRAM_MANAGER_H
#define ALLPIX_HISTOGRAM_MANAGER_H

#include <TFile.h>
#include <TROOT.h>
#include <TSystem.h>
#include <filesystem>
#include <map>

#include "core/config/exceptions.h"
#include "tools/ROOT.h"

namespace allpix {
    class HistogramManager {
    public:
        HistogramManager(const std::string& file_path, const bool& deny_overwrite) {
            LOG(INFO) << "Opening histogram file.";

            if(histogram_file_ && !histogram_file_->IsZombie()) {
                throw RuntimeError("Histogram Manager had already been started.");
            }

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

            histogram_file_ = std::make_unique<TFile>(path.c_str(), "RECREATE");
            if(histogram_file_->IsZombie()) {
                throw RuntimeError("Cannot create main ROOT file " + path.string());
            }
            histogram_file_->cd();

            LOG(TRACE) << "Opened histogram file.";
        };

        ~HistogramManager() {};

        TDirectory* register_module_directory(const std::string& module_name, const std::string& module_identifier) {
            // Create main ROOT directory for this module class if it does not exist yet
            LOG(TRACE) << "Creating and accessing ROOT directory";
            auto* module_directory = histogram_file_->GetDirectory(module_name.c_str());
            if(module_directory == nullptr) {
                module_directory = histogram_file_->mkdir(module_name.c_str());
                if(module_directory == nullptr) {
                    throw RuntimeError("Cannot create or access overall ROOT directory for module " + module_name);
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
                if(unique_module_directory == nullptr) {
                    throw RuntimeError("Cannot create or access local ROOT directory for module " + module_name);
                }
            }

            // Change to the directory
            unique_module_directory->cd();

            return unique_module_directory;
        }

        TDirectory* register_subdirectory(TDirectory* unique_module_directory, const std::string& subdirectory_name = "") {
            if(unique_module_directory == nullptr) {
                throw RuntimeError("ROOT directory for unique module should be present but cannot be accessed.");
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
                if(subdirectory == nullptr) {
                    throw RuntimeError("Cannot create or access ROOT subdirectory: " + subdirectory_name);
                }
            }

            return subdirectory;
        };

        TDirectory* register_generic_path(const std::string& path) {
            // Create ROOT directory for this path
            LOG(TRACE) << "Creating and accessing ROOT directory";
            auto* directory = histogram_file_->GetDirectory(path.c_str());
            if(directory == nullptr) {
                directory = histogram_file_->mkdir(path.c_str());
                if(directory == nullptr) {
                    throw RuntimeError("Cannot create or access overall ROOT directory: " + path);
                }
            }
            directory->cd();

            return directory;
        };

        template <typename T, class... ARGS>
        std::shared_ptr<ThreadedHistogram<T>> register_histogram(TDirectory* directory, ARGS&&... args) {
            return create_histogram<T>(directory, std::forward<ARGS>(args)...);
        };

        template <typename T, class... ARGS>
        std::shared_ptr<ThreadedHistogram<T>> register_histogram_with_path(const std::string& path, ARGS&&... args) {
            auto directory = register_generic_path(path);
            return create_histogram<T>(directory, std::forward<ARGS>(args)...);
        };

        void finalize() {
            LOG(INFO) << "Finalizing histograms ...";

            // Only if file is open
            if(histogram_file_->IsZombie()) {
                LOG(ERROR) << "Histogram file is not open. Cannot finalize.";
                return; // TODO: throw or return?
            }

            for(auto it = histogram_map_.begin(); it != histogram_map_.end(); it++) {
                LOG(TRACE) << "Storing histogram " + it->second->GetName() + " in path " << it->first;
                histogram_file_->cd(it->first.c_str());
                it->second->Write();
            }

            // Close file
            histogram_file_->Close();
        };

    protected:
        std::multimap<std::string, std::shared_ptr<BaseHistogram>> get_histogram_map() { return histogram_map_; };

    private:
        HistogramManager() = default;

        template <typename T, class... ARGS>
        std::shared_ptr<ThreadedHistogram<T>> create_histogram(TDirectory* directory, ARGS&&... args) {
            directory->cd();
            LOG(TRACE) << "Current directory: " << gDirectory->GetPath();
            auto histogram = std::make_shared<ThreadedHistogram<T>>(std::forward<ARGS>(args)...);

            LOG(DEBUG) << "Registering histogram (" << directory->GetPath() << ", " << histogram->Get()->GetName() << ")";
            histogram_map_.emplace(directory->GetPath(), histogram);

            return histogram;
        };

        std::multimap<std::string, std::shared_ptr<BaseHistogram>> histogram_map_;
        std::unique_ptr<TFile> histogram_file_;
    };
} // namespace allpix

#endif /* ALLPIX_HISTOGRAM_MANAGER_H */
