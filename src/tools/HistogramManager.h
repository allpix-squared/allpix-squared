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

#include <TFile.h>
#include <TROOT.h>
#include <TSystem.h>
#include <filesystem>
#include <map>

#include "core/config/exceptions.h"
#include "tools/ROOT.h"

using namespace allpix;

class HistogramManager {
public:
    static HistogramManager& getInstance() {
        static HistogramManager instance{};
        return instance;
    };

    ~HistogramManager() {};

    void register_histogram(std::string unique_name, std::string histo_name) {
        LOG(WARNING) << "Registering histogram (" << unique_name << ", " << histo_name << ")";

        histogram_map_[std::pair<std::string, std::string>(unique_name, histo_name)] = histo_name;

        LOG(INFO) << "Histograms registered so far:";
        for(auto& it : histogram_map_) {
            LOG(INFO) << "  ( " << it.first.first << ", " << it.first.second << ")";
        }
    };

    void openFile(std::string file_path, bool deny_overwrite) {
        // TODO?: For some reason, histogram_file is tagged as "unused" when I read the global config from here. If we like
        // this version, it's not an issue.
        LOG(INFO) << "This should happen only once in a lifetime (of a run)";

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

        LOG(INFO) << "Opened histogram file.";
    };

    void finalize() {
        LOG(INFO) << "Finalizing histograms ...";

        // Only if file is open
        if(histogram_file_->IsZombie()) {
            LOG(INFO) << "Histogram file is not open. Cannot finalize.";
            return;
        }

        // Looping over all modules
        // Looping over all histograms
        // Write() them

        // Close file
        histogram_file_->Close();
    };

private:
    HistogramManager() {};

    std::map<std::pair<std::string, std::string>, std::string> histogram_map_;
    std::unique_ptr<TFile> histogram_file_;
};
