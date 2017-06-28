/**
 * Message for a deposited charge in a sensor
 *
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_DEPOSITED_CHARGE_H
#define ALLPIX_DEPOSITED_CHARGE_H

#include <TRef.h>

#include "MCParticle.hpp"
#include "SensorCharge.hpp"

namespace allpix {
    // object definition
    class DepositedCharge : public SensorCharge {
    public:
        DepositedCharge(ROOT::Math::XYZPoint position,
                        unsigned int charge,
                        double event_time,
                        const MCParticle* mc_particle = nullptr);

        const MCParticle* getMCParticle() const;

        ClassDef(DepositedCharge, 1);
        DepositedCharge() = default;

        // NOTE special method because MCParticle is only known after deposit creation
        void setMCParticle(const MCParticle* mc_particle);

    private:
        TRef mc_particle_;
    };

    // link to the carrying message
    using DepositedChargeMessage = Message<DepositedCharge>;
} // namespace allpix

#endif
