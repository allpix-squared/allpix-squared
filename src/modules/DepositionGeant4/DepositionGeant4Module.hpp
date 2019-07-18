/**
 * @file
 * @brief Definition of Geant4 deposition module
 * @copyright Copyright (c) 2017-2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_SIMPLE_DEPOSITION_MODULE_H
#define ALLPIX_SIMPLE_DEPOSITION_MODULE_H

#include <memory>
#include <string>
#include <atomic>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Event.hpp"
#include "core/module/Module.hpp"

#include "SensitiveDetectorActionG4.hpp"
#include "TrackInfoManager.hpp"

#include <TH1D.h>
#include <ROOT/TThreadedObject.hxx>

class G4UserLimits;
class G4RunManager;

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to simulate the particle beam and generating the charge deposits in the sensor
     *
     * A beam is defined at a certain position that propagates a particular particle in a certain direction. When the beam
     * hits the sensor the energy loss is converted to charge deposits using the electron-hole creation energy. The energy
     * deposits are specific for a detector. The module also returns the information of the real particle passage (the
     * MCParticle).
     */
    class DepositionGeant4Module : public Geant4Module {
        friend class SDAndFieldConstruction;
        friend class SetTrackInfoUserHookG4;
    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param geo_manager Pointer to the geometry manager, containing the detectors
         */
        DepositionGeant4Module(Configuration& config, Messenger* messenger, GeometryManager* geo_manager);

        /**
         * @brief Initializes the physics list of processes and constructs the particle source
         */
        void init(std::mt19937_64&) override;

        /**
         * @brief Deposit charges for a single event
         */
        void run(Event*) override;

        /**
         * @brief Cleanup \ref RunManager for each thread
         */
        void finalizeThread() override;

        /**
         * @brief Display statistical summary
         */
        void finalize() override;

    private:
        /**
         * @brief Construct the sensitive detectors and magnetic fields.
         * @param fano_factor
         * @param charge_creation_energy
         */
        void construct_sensitive_detectors_and_fields(double fano_factor, double charge_creation_energy);

        /**
         * @brief Record statistics for the module run.
         */
        void record_module_statistics();

        Messenger* messenger_;
        GeometryManager* geo_manager_;

        // The track manager which this module uses to assign custom track IDs and manage & create MCTracks
        static thread_local std::unique_ptr<TrackInfoManager> track_info_manager_;

        // Handling of the charge deposition in all the sensitive devices
        static thread_local std::vector<SensitiveDetectorActionG4*> sensors_;

        // Number of the last event
        std::atomic_uint last_event_num_{0};

        // Class holding the limits for the step size
        std::unique_ptr<G4UserLimits> user_limits_;

        // Pointer to the Geant4 manager (owned by GeometryBuilderGeant4)
        G4RunManager* run_manager_g4_;

        // Vector of histogram pointers for debugging plots
        std::map<std::string, ROOT::TThreadedObject<TH1D>*> charge_per_event_;

        std::once_flag geant4_seed;

        // Total deposited charges
        std::atomic_uint total_charges_{0};

        std::atomic_size_t number_of_sensors_{0};

        // cached copy of the multithreading configuration flag
        bool using_multithreading_{false};

        // Mutex used for the construction of histograms
        std::mutex histogram_mutex_;
    };
} // namespace allpix

#endif /* ALLPIX_SIMPLE_DEPOSITION_MODULE_H */
