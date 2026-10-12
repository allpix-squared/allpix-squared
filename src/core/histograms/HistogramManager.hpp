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

#include "core/config/exceptions.h"
#include "core/histograms/ThreadedHistogram.hpp"

namespace allpix {

    /**
     * @brief The HistogramManager manages histograms - centralized for all modules and the ModuleManager
     *
     * This class holds all interactions with the histogram file, keeps track of all histograms registered, creates
     * corresponding directories and writes the histograms.
     * @note it is still possible to create TObjects without registering, as exercised e.g. via TGraphs in a few modules.
     */
    class HistogramManager {
    public:
        /**
         * @brief Base constructor for histogram manager
         * @param file_path Path to the histogram file as provided in the configuration
         * @param deny_overwrite Prevents from overwriting files if set to true
         */
        HistogramManager(const std::string& file_path, bool deny_overwrite);

        /**
         * @brief Destructor: closes ROOT file
         */
        ~HistogramManager();

        /// @{
        /**
         * @brief Copying the manager is not allowed
         */
        HistogramManager(const HistogramManager&) = delete;
        HistogramManager& operator=(const HistogramManager&) = delete;
        /// @}

        /// @{
        /**
         * @brief Use default move behaviour
         */
        HistogramManager(HistogramManager&&) noexcept = default;
        HistogramManager& operator=(HistogramManager&&) noexcept = default;
        /// @}

        /**
         * @brief Creates and registers a directory in the histogram file for each module
         * @param module_name Name of the module
         * @param module_identifier Identifier of the module, e.g. detector name
         * @return Pointer to created ROOT TDirectory
         */
        TDirectory* registerModuleDirectory(const std::string& module_name, const std::string& module_identifier);

        /**
         * @brief Creates and registers a subdirectory within a modules' directory in the histogram file
         * @param unique_module_directory Pointer to TDirectory of this unique module
         * @param subdirectory_name Name of the directory to be created
         * @return Pointer to created ROOT TDirectory
         */
        static TDirectory* registerSubdirectory(TDirectory* unique_module_directory, const std::string& subdirectory_name);

        /**
         * @brief Creates and registers a directory with a generic path in the histogram file
         * @param path Name of path to be created
         * @return Pointer to created ROOT TDirectory
         * @note This creates subdirectories via the syntax "path/to/directory"
         */
        TDirectory* registerGenericPath(const std::string& path);

        /**
         * @brief Create and register histogram within a given directory of the histogram file
         * @param directory Directory for the histogram to live in
         * @param args Further arguments for histogram constructor
         * @return Shared pointer to generated histogram object
         */
        template <typename T, class... ARGS>
        std::shared_ptr<ThreadedHistogram<T>> registerHistogram(TDirectory* directory, ARGS&&... args);

        /**
         * @brief Register path in the histogram file and emplace newly generated histogram there
         * @param path Name of the path to be created
         * @param args Further arguments for histogram constructor
         * @return Shared pointer to generated histogram object
         */
        template <typename T, class... ARGS>
        std::shared_ptr<ThreadedHistogram<T>> registerHistogram(const std::string& path, ARGS&&... args);

        /**
         * @brief Writes all histograms into their corresponding directories in the histogram file
         */
        void finalize();

    protected:
        /**
         * @brief Get the histogram list
         * @return vector with directories of histograms (key) and pointers to histograms (value)
         */
        const std::vector<std::pair<std::string, std::shared_ptr<BaseHistogram>>>& get_histogram_list() const {
            return histogram_list_;
        };

    private:
        HistogramManager() = default;

        /**
         * @brief Create histogram and register it in the histogram list
         * @param directory Pointer to histogram file directory the histogram should be written to
         * @param args Further arguments for histogram constructor
         * @return shared pointer to generated histogram object
         */
        template <typename T, class... ARGS>
        std::shared_ptr<ThreadedHistogram<T>> create_histogram(TDirectory* directory, ARGS&&... args);

        std::vector<std::pair<std::string, std::shared_ptr<BaseHistogram>>> histogram_list_;
        std::unique_ptr<TFile> histogram_file_;
    };
} // namespace allpix

// Include template members
#include "HistogramManager.tpp"

#endif /* ALLPIX_HISTOGRAM_MANAGER_H */
