/**
 * @file
 * @brief Definition of [ParticleDistributions] module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 *
 * Contains minimal dummy module to use as a start for the development of your own module
 *
 * Refer to the User's Manual for more details.
 */

#include <string>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

#include "objects/MCParticle.hpp"

#include "TH1F.h"
#include "TH2F.h"
#include "TH3F.h"
#include "TTree.h"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to do function
     *
     * More detailed explanation of module
     */
    class ParticleDistributionsModule : public Module {
    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param geo_manager Pointer to the geometry manager, containing the detectors
         */
        ParticleDistributionsModule(Configuration& config, Messenger* messenger, GeometryManager* geo_manager);

        /**
         * @brief [Initialise this module]
         */
        void init() override;

        /**
         * @brief [Run the function of this module]
         */
        void run(unsigned int) override;

        void finalize() override;

    private:
        // General module members
        Messenger* messenger_;
        GeometryManager* geo_manager_;
        std::shared_ptr<MCTrackMessage> message_;
        TH1F* energy_distribution_;
        TH2F* zx_distribution_;
        TH2F* zy_distribution_;
        TH3F* xyz_distribution_;
        TH3F* xyz_energy_distribution_;

        // ntuple variables
        TTree* simple_tree_;
        double energy_;
        double particle_id_;
        double start_position_x_;
        double start_position_y_;
        double start_position_z_;
        double momentum_x_;
        double momentum_y_;
        double momentum_z_;

        bool store_particles_;
    };
} // namespace allpix
