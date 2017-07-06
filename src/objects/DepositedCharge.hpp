/**
 * @file
 * @brief Definition of deposited charge object
 * @copyright MIT License
 */

#ifndef ALLPIX_DEPOSITED_CHARGE_H
#define ALLPIX_DEPOSITED_CHARGE_H

#include <TRef.h>

#include "MCParticle.hpp"
#include "SensorCharge.hpp"

namespace allpix {
    /**
     * @ingroup Objects
     * @brief Charge deposit in sensor of detector
     */
    class DepositedCharge : public SensorCharge {
    public:
        /**
         * @brief Construct a charge deposit
         * @param position Local position of the deposit in the sensor
         * @param charge Total charge of the deposit
         * @param event_time Time of deposition after event start
         * @param mc_particle Optional pointer to related MC particle
         */
        DepositedCharge(ROOT::Math::XYZPoint position,
                        unsigned int charge,
                        double event_time,
                        const MCParticle* mc_particle = nullptr);

        /**
         * @brief Get related Monte-Carlo particle
         * @return Pointer to possible Monte-Carlo particle
         */
        const MCParticle* getMCParticle() const;

        /**
         * @brief Set the Monte-Carlo particle
         * @param mc_particle The Monte-Carlo particle
         * @warning Special method because MCParticle is only known after deposit creation, should not be used to replace the
         * particle later
         */
        void setMCParticle(const MCParticle* mc_particle);

        /**
         * @brief ROOT class definition
         */
        ClassDef(DepositedCharge, 1);
        /**
         * @brief Default constructor for ROOT I/O
         */
        DepositedCharge() = default;

    private:
        TRef mc_particle_;
    };

    /**
     * @brief Typedef for message carrying deposits
     */
    using DepositedChargeMessage = Message<DepositedCharge>;
} // namespace allpix

#endif
