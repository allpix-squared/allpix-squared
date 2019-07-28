/**
 * @file
 * @brief Implementation of RunManager
 * @copyright Copyright (c) 2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "RunManager.hpp"

using namespace allpix;

void RunManager::BeamOn(G4int n_event, const char* macro_file, G4int n_select) {
    if(!fakeRun) {
        // Create a new engine with the same type as the default engine
        if(event_random_engine_ == nullptr) {
            // Remember the default RNG engine before replacing it so we can use it
            // later.
            // TODO: maybe we should delete it ourselves too
            master_random_engine_ = G4Random::getTheEngine();

            if(dynamic_cast<const CLHEP::HepJamesRandom*>(master_random_engine_) != nullptr) {
                event_random_engine_ = new CLHEP::HepJamesRandom;
            }
            if(dynamic_cast<const CLHEP::MixMaxRng*>(master_random_engine_) != nullptr) {
                event_random_engine_ = new CLHEP::MixMaxRng;
            }
            if(dynamic_cast<const CLHEP::RanecuEngine*>(master_random_engine_) != nullptr) {
                event_random_engine_ = new CLHEP::RanecuEngine;
            }
            if(dynamic_cast<const CLHEP::Ranlux64Engine*>(master_random_engine_) != nullptr) {
                event_random_engine_ = new CLHEP::Ranlux64Engine;
            }
            if(dynamic_cast<const CLHEP::MTwistEngine*>(master_random_engine_) != nullptr) {
                event_random_engine_ = new CLHEP::MTwistEngine;
            }
            if(dynamic_cast<const CLHEP::DualRand*>(master_random_engine_) != nullptr) {
                event_random_engine_ = new CLHEP::DualRand;
            }
            if(dynamic_cast<const CLHEP::RanluxEngine*>(master_random_engine_) != nullptr) {
                event_random_engine_ = new CLHEP::RanluxEngine;
            }
            if(dynamic_cast<const CLHEP::RanshiEngine*>(master_random_engine_) != nullptr) {
                event_random_engine_ = new CLHEP::RanshiEngine;
            }

            // Replace the RNG engine with the new one
            // TODO: maybe need to do something if it was null?!
            if(event_random_engine_ != nullptr) {
                G4Random::setTheEngine(event_random_engine_);
            }
        }

        // For each call to BeamOn, we draw random numbers from the first engine
        // and use them as seeds to the second one. This is exactly what the MTRunManager
        // does where the second engine is a thread local version
        G4RNGHelper* helper = G4RNGHelper::GetInstance();

        // Fill 1 set of seeds only
        master_random_engine_->flatArray(number_seeds_per_event_, &seed_array_[0]);
        helper->Fill(&seed_array_[0], 1, 1, number_seeds_per_event_);

        long s1 = helper->GetSeed(0);
        long s2 = helper->GetSeed(1);
        long seeds[3] = {s1, s2, 0};
        G4Random::setTheSeeds(seeds, -1);
    }

    // Redirect the call to BeamOn
    G4RunManager::BeamOn(n_event, macro_file, n_select);
}
