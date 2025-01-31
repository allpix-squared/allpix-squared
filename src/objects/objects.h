/**
 * @file
 * @brief File including all current objects
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "DepositedCharge.hpp"
#include "MCParticle.hpp"
#include "MCTrack.hpp"
#include "Pixel.hpp"
#include "PixelCharge.hpp"
#include "PixelHit.hpp"
#include "PropagatedCharge.hpp"

namespace allpix {
    /**
     * @brief Tuple containing all objects
     */
    using OBJECTS = std::tuple<MCTrack, MCParticle, DepositedCharge, PropagatedCharge, PixelCharge, PixelHit>;
} // namespace allpix
