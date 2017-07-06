/**
 * @file
 * @brief File including all current objects
 * @copyright MIT License
 */

#include "DepositedCharge.hpp"
#include "MCParticle.hpp"
#include "PixelCharge.hpp"
#include "PixelHit.hpp"
#include "PropagatedCharge.hpp"

namespace allpix {
    /**
     * @brief Tuple containing all objects
     */
    using OBJECTS = std::tuple<MCParticle, DepositedCharge, PropagatedCharge, PixelCharge, PixelHit>;
}
