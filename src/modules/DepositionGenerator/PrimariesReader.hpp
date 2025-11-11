/**
 * @file
 * @brief Defines a generic and purely virtual particle reader interface class
 *
 * @copyright Copyright (c) 2022-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_GENERATOR_DEPOSITION_MODULE_READER_H
#define ALLPIX_GENERATOR_DEPOSITION_MODULE_READER_H

#include <memory>

#include <G4ThreeVector.hh>

#include "core/config/Configuration.hpp"

namespace allpix {
    /**
     * @brief Interface class to read primary particles from input data in different file formats
     */
    class PrimariesReader {
        friend class DepositionGeneratorModule;

    public:
        /**
         * @brief Different implemented file models
         */
        enum class FileModel : std::uint8_t {
            GENIE,      ///< Genie generator ROOT files
            HEPMC,      ///< HepMC data files from generators such as Pythia
            HEPMC2,     ///< HepMC2 data files, outdated format
            HEPMCROOT,  ///< HepMC ROOTIO file format
            HEPMCTTREE, ///< HepMC ROOTIO TTree file format
        };

        /**
         * @brief Particle class to hold information for primary particles before dispatching them to Geant4
         */
        class Particle {
        public:
            /**
             * Particle constructor
             * @param id   PDG ID of the particle
             * @param e    Energy
             * @param dir  Direction vector
             * @param pos  Position vector
             * @param t    Creation time
             */
            Particle(int id, double e, G4ThreeVector dir, G4ThreeVector pos, double t) noexcept
                : id_(id), energy_(e), direction_(std::move(dir)), position_(std::move(pos)), time_(t) {}

            /**
             * Provides the PDG ID of the particle
             * @return PDG ID
             */
            int pdg() const { return id_; }

            /**
             * Provides the particle energy
             * @return Energy
             */
            double energy() const { return energy_; }

            /**
             * Provides the direction vector of the particle momentum
             * @return Direction vector
             */
            G4ThreeVector direction() const { return direction_; }

            /**
             * Provides the position of the primary vertex
             * @return Position vector
             */
            G4ThreeVector position() const { return position_; }

            /**
             * Provides the creation time of the particle in the event
             * @return Time
             */
            double time() const { return time_; }

        private:
            int id_;
            double energy_;
            G4ThreeVector direction_;
            G4ThreeVector position_;
            double time_;
        };

        /**
         * Default constructor and destructor
         */
        PrimariesReader() = default;
        virtual ~PrimariesReader() = default;

        /**
         * Purely virtual method to obtain a vector of primary particles for the current event. This methods needs to be
         * implemented by derived classes which implement a specific file format. This method normally needs to be called
         * sequentially and is not thread-safe.
         * @return Vector of primary particles
         */
        virtual std::vector<Particle> getParticles() = 0;

        /**
         * Get the event number of the currently processed event. This allows to cross-check with potentially available event
         * ID information from the input data file.
         * @return Event number
         */
        uint64_t eventNum() const { return event_num_; }

    private:
        /**
         * Helper method to set the currently processed event number from the \ref DepositionGeneratorModule::run() function.
         * This is not thread-safe and should only be called in sequential processing mode.
         * @param event_num  Event number
         */
        void set_event_num(uint64_t event_num) { event_num_ = event_num; }
        uint64_t event_num_;
    };
} // namespace allpix

#endif /* ALLPIX_GENERATOR_DEPOSITION_MODULE_READER_H */
