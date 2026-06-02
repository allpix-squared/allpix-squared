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

#pragma once

#include "HistogramManager.hpp" // NOLINT(misc-header-include-cycle)

#include "core/histograms/ThreadedHistogram.hpp"

namespace allpix {
    template <typename T, class... ARGS>
    std::shared_ptr<ThreadedHistogram<T>> HistogramManager::registerHistogram(TDirectory* directory, ARGS&&... args) {
        return create_histogram<T>(directory, std::forward<ARGS>(args)...);
    };

    template <typename T, class... ARGS>
    std::shared_ptr<ThreadedHistogram<T>> HistogramManager::registerHistogram(const std::string& path, ARGS&&... args) {
        auto* directory = registerGenericPath(path);
        return create_histogram<T>(directory, std::forward<ARGS>(args)...);
    }

    template <typename T, class... ARGS>
    std::shared_ptr<ThreadedHistogram<T>> HistogramManager::create_histogram(TDirectory* directory, ARGS&&... args) {
        directory->cd();

        const std::string abs_path_str = gDirectory->GetPath();
        const auto local_path_str = abs_path_str.substr(abs_path_str.find_last_of(':') + 1);

        auto histogram = std::make_shared<ThreadedHistogram<T>>(std::forward<ARGS>(args)...);

        LOG(DEBUG) << "Registering histogram (" << local_path_str << ", " << histogram->Get()->GetName() << ")";
        histogram_list_.push_back({local_path_str, histogram});

        return histogram;
    }
} // namespace allpix
