/**
 * @file
 * @brief Definition of DepositionReader module
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 *
 * This module allows to read pre-computed energy deposits from data files, containing the energy deposit and a position
 * given in local coordinates
 *
 * Refer to the User's Manual for more details.
 */

#include <fstream>
#include <functional>
#include <string>

#include <TFile.h>
#include <TH1D.h>
#include <TTreeReader.h>
#include <TTreeReaderArray.h>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"
#include "objects/DepositedCharge.hpp"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to read pre-computed energy deposits
     *
     * This module allows to read pre-computed energy deposits from data files of different formats. The files should contain
     * individual events with a list of energy deposits at specific position given in local coordinates of the respective
     * detector.
     */
    class DepositionReaderModule : public SequentialModule {

        /**
         * @brief Different implemented file models
         */
        enum class FileModel {
            ROOT, ///< ROOT Trees
            CSV,  ///< Comma-separated value files
        };

    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param geo_manager Pointer to the geometry manager, containing the detectors
         */
        DepositionReaderModule(Configuration& config, Messenger* messenger, GeometryManager* geo_manager);

        /**
         * @brief Initialize the input file stream
         */
        void initialize() override;

        /**
         * @brief Read the deposited energy for a given event and create a corresponding DepositedCharge message
         */
        void run(Event*) override;

        /**
         * @brief Finalize and write histograms
         */
        void finalize() override;

    private:
        // General module members
        GeometryManager* geo_manager_;
        Messenger* messenger_;

        // File containing the input data
        std::unique_ptr<std::ifstream> input_file_;
        std::unique_ptr<TFile> input_file_root_;

        // Helper to create and check tree branches
        template <typename T> void create_tree_reader(std::shared_ptr<T>& branch_ptr, const std::string& name);
        template <typename T> void check_tree_reader(std::shared_ptr<T> branch_ptr);

        // Set up branches:
        std::shared_ptr<TTreeReader> tree_reader_;
        std::shared_ptr<TTreeReaderValue<int>> event_;
        std::shared_ptr<TTreeReaderValue<double>> edep_;
        std::shared_ptr<TTreeReaderValue<double>> time_;
        std::shared_ptr<TTreeReaderValue<double>> px_;
        std::shared_ptr<TTreeReaderValue<double>> py_;
        std::shared_ptr<TTreeReaderValue<double>> pz_;
        std::shared_ptr<TTreeReaderArray<char>> volume_;
        std::shared_ptr<TTreeReaderValue<int>> pdg_code_;
        std::shared_ptr<TTreeReaderValue<int>> track_id_;
        std::shared_ptr<TTreeReaderValue<int>> parent_id_;
        std::map<std::shared_ptr<Detector>, double> charge_creation_energy_;
        std::map<std::shared_ptr<Detector>, double> fano_factor_;

        FileModel file_model_;
        size_t volume_chars_{};
        std::string unit_length_{}, unit_time_{}, unit_energy_{};
        bool output_plots_{};

        bool require_sequential_events_{}, create_mcparticles_{}, time_available_{};

        bool read_csv(uint64_t event_num,
                      std::string& volume,
                      ROOT::Math::XYZPoint& position,
                      double& time,
                      double& energy,
                      int& pdg_code,
                      int& track_id,
                      int& parent_id);
        bool read_root(uint64_t event_num,
                       int64_t& curr_event_id,
                       std::string& volume,
                       ROOT::Math::XYZPoint& position,
                       double& time,
                       double& energy,
                       int& pdg_code,
                       int& track_id,
                       int& parent_id);

        // Vector of histogram pointers for debugging plots
        std::map<std::shared_ptr<Detector>, Histogram<TH1D>> charge_per_event_;
    };
} // namespace allpix
