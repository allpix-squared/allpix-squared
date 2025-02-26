/**
 * @file
 * @brief Defines the HepMC generator file reader module for primary particles
 *
 * @copyright Copyright (c) 2022-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_PRIMARIES_DEPOSITION_MODULE_READER_HEPMC_H
#define ALLPIX_PRIMARIES_DEPOSITION_MODULE_READER_HEPMC_H

#include "PrimariesReader.hpp"

#include "HepMC3/GenEvent.h"
#include "HepMC3/Reader.h"

namespace allpix {
    /**
     * @brief Reads particles from an input data file
     */
    class PrimariesReaderHepMC : public PrimariesReader {
    public:
        /**
         * Default constructor which opens the file and checks that all expected trees and branches are available
         * @param config  Module configuration object
         */
        explicit PrimariesReaderHepMC(const Configuration& config);

        /**
         * Overwritten method to obtain the primary particles for the current event. This method needs to be called
         * sequentially and is not thread-safe.
         * @return Vector of primary particles
         */
        std::vector<Particle> getParticles() override;

    private:
        std::shared_ptr<HepMC3::Reader> reader_;
    };
} // namespace allpix

#endif /* ALLPIX_PRIMARIES_DEPOSITION_MODULE_READER_HEPMC_H */
