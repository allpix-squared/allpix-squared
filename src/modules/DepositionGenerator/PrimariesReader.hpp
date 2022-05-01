/**
 * @file
 * @brief Defines the CRY interface to Geant4
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_PRIMARIES_DEPOSITION_MODULE_READER_H
#define ALLPIX_PRIMARIES_DEPOSITION_MODULE_READER_H

#include <memory>

#include <G4ThreeVector.hh>

#include "core/config/Configuration.hpp"

namespace allpix {
    /**
     * @brief Generates the particles in every event
     */
    class PrimariesReader {
    public:
        class Particle {
        public:
            Particle(int id, double e, G4ThreeVector dir, G4ThreeVector pos, double t)
                : id_(id), energy_(e), direction_(dir), position_(pos), time_(t) {}

            int pdg() const { return id_; }
            double energy() const { return energy_; }
            G4ThreeVector direction() const { return direction_; }
            G4ThreeVector position() const { return position_; }
            double time() const { return time_; }

        private:
            int id_;
            double energy_;
            G4ThreeVector direction_;
            G4ThreeVector position_;
            double time_;
        };

        PrimariesReader() = default;
        virtual ~PrimariesReader() = default;
        PrimariesReader(const Configuration&){};
        virtual std::vector<Particle> getParticles() = 0;
    };
} // namespace allpix

#endif /* ALLPIX_PRIMARIES_DEPOSITION_MODULE_READER_H */
