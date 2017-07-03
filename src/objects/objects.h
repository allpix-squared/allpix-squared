#include "DepositedCharge.hpp"
#include "MCParticle.hpp"
#include "PixelCharge.hpp"
#include "PixelHit.hpp"
#include "PropagatedCharge.hpp"

namespace allpix {
    using OBJECTS = std::tuple<MCParticle, DepositedCharge, PropagatedCharge, PixelCharge, PixelHit>;
}
