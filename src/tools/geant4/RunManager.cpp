/**
 * @file
 * @brief Implementation of RunManager
 * @copyright Copyright (c) 2019-2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "RunManager.hpp"

#include <array>

using namespace allpix;

void RunManager::Run(G4int n_event, uint64_t seed1, uint64_t seed2) { // NOLINT

    // Set the event seeds - with a zero-terminated list:
    std::array<long, 3> seeds{static_cast<long>(seed1 % LONG_MAX), static_cast<long>(seed2 % LONG_MAX), 0};
    G4Random::setTheSeeds(&seeds[0], -1);

    // Call the RunManager's BeamOn
    G4RunManager::BeamOn(n_event);
}
