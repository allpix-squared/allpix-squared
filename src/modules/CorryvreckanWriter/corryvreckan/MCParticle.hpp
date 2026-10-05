/**
 * @file
 * @brief Definition of MCParticle object
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef CORRYVRECKAN_MCPARTICLE_H
#define CORRYVRECKAN_MCPARTICLE_H

#include <Math/Point3D.h>
#include "Object.hpp"

namespace corryvreckan {
    /**
     * @ingroup Objects
     * @brief Monte Carlo particle from simulation
     */
    class MCParticle : public Object {

    public:
        // Constructors and destructors
        MCParticle() = default;
        MCParticle(std::string detectorID,
                   int particle_id,
                   ROOT::Math::XYZPoint local_start_point,
                   ROOT::Math::XYZPoint local_end_point,
                   double timestamp)
            : Object(std::move(detectorID), timestamp), particle_id_(particle_id),
              local_start_point_(std::move(local_start_point)), local_end_point_(std::move(local_end_point)) {}

        /**
         * @brief Static member function to obtain base class for storage on the clipboard.
         * This method is used to store objects from derived classes under the typeid of their base classes
         *
         * @warning This function should not be implemented for derived object classes
         *
         * @return Class type of the base object
         */
        static std::type_index getBaseType() { return typeid(MCParticle); }

        // Member functions
        int getID() const { return particle_id_; }
        const ROOT::Math::XYZPoint& getLocalStart() const { return local_start_point_; }
        const ROOT::Math::XYZPoint& getLocalEnd() const { return local_end_point_; }

    private:
        int particle_id_{};
        ROOT::Math::XYZPoint local_start_point_;
        ROOT::Math::XYZPoint local_end_point_;

        // ROOT I/O class definition - update version number when you change this class!
        ClassDef(MCParticle, 3); // NOLINT
    };

    // Vector type declaration
    using MCParticleVector = std::vector<MCParticle*>;
} // namespace corryvreckan

#endif // CORRYVRECKAN_MCPARTICLE_H
