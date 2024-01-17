/**
 * @file
 * @brief Defines the GENIE MC generator file reader module for primary particles
 *
 * @copyright Copyright (c) 2022-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_PRIMARIES_DEPOSITION_MODULE_READER_GENIE_H
#define ALLPIX_PRIMARIES_DEPOSITION_MODULE_READER_GENIE_H

#include "PrimariesReader.hpp"

#include <TFile.h>
#include <TTreeReader.h>
#include <TTreeReaderArray.h>

namespace allpix {
    /**
     * @brief Reads particles from an input data file
     */
    class PrimariesReaderGenie : public PrimariesReader {
    public:
        /**
         * Default constructor which opens the file and checks that all expected trees and branches are available
         * @param config  Module configuration object
         */
        explicit PrimariesReaderGenie(const Configuration& config);

        /**
         * Overwritten method to obtain the primary particles for the current event. This method needs to be called
         * sequentially and is not thread-safe.
         * @return Vector of primary particles
         */
        std::vector<Particle> getParticles() override;

    private:
        // Helper to create and check tree branches
        template <typename T> void create_tree_reader(std::shared_ptr<T>& branch_ptr, const std::string& name);
        template <typename T> void check_tree_reader(const Configuration& config, std::shared_ptr<T> branch_ptr);

        std::unique_ptr<TFile> input_file_;

        std::shared_ptr<TTreeReader> tree_reader_;
        std::shared_ptr<TTreeReaderValue<int>> event_;
        std::shared_ptr<TTreeReaderArray<int>> pdg_code_;
        std::shared_ptr<TTreeReaderArray<float>> px_;
        std::shared_ptr<TTreeReaderArray<float>> py_;
        std::shared_ptr<TTreeReaderArray<float>> pz_;
        std::shared_ptr<TTreeReaderArray<float>> energy_;
    };
} // namespace allpix

#endif /* ALLPIX_PRIMARIES_DEPOSITION_MODULE_READER_GENIE_H */
