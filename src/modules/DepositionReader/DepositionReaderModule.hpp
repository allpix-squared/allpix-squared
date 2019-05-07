/**
 * @file
 * @brief Definition of DepositionReader module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 *
 * This module allows to read pre-computed energy deposits from data files, containing the energy deposit and a position
 * given in local coordinates
 *
 * Refer to the User's Manual for more details.
 */

#include <fstream>
#include <string>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to read pre-computed energy deposits
     *
     * This module allows to read pre-computed energy deposits from data files fo different formats. The files should contain
     * individual events with a list of energy deposits at specific position given in local coordinates of the respective
     * detector.
     */
    class DepositionReaderModule : public Module {
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
        void init() override;

        /**
         * @brief Read the deposited energy for a given event and create a corresponding DepositedCharge message
         */
        void run(unsigned int) override;

    private:
        // General module members
        GeometryManager* geo_manager_;
        Messenger* messenger_;

        std::shared_ptr<Detector> detector_;

        // File containing the input data
        std::unique_ptr<std::ifstream> input_file_;

        double charge_creation_energy_;
        double fano_factor_;

        // Random number generator for e/h pair creation fluctuation
        std::mt19937_64 random_generator_;
    };
} // namespace allpix
