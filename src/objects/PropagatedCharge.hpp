/**
 * @file
 * @brief Definition of propagated charge object
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_PROPAGATED_CHARGE_H
#define ALLPIX_PROPAGATED_CHARGE_H

#include <map>

#include "DepositedCharge.hpp"
#include "MCParticle.hpp"
#include "Pixel.hpp"
#include "Pulse.hpp"
#include "SensorCharge.hpp"

namespace allpix {
    /**
     * @ingroup Objects
     * @brief Set of charges propagated through the sensor
     */
    class PropagatedCharge : public SensorCharge {
        friend class PixelCharge;

    public:
        /**
         * @brief Construct a set of propagated charges
         * @param local_position Local position of the propagated set of charges in the sensor
         * @param global_position Global position of the propagated set of charges in the sensor
         * @param type Type of the carrier to propagate
         * @param charge Total charge propagated
         * @param event_time Total time of propagation arrival after event start
         * @param deposited_charge Optional pointer to related deposited charge
         */
        PropagatedCharge(ROOT::Math::XYZPoint local_position,
                         ROOT::Math::XYZPoint global_position,
                         CarrierType type,
                         unsigned int charge,
                         double event_time,
                         const DepositedCharge* deposited_charge = nullptr);

        /**
         * @brief Construct a set of propagated charges
         * @param local_position Local position of the propagated set of charges in the sensor
         * @param global_position Global position of the propagated set of charges in the sensor
         * @param type Type of the carrier to propagate
         * @param pulses Map of pulses induced at electrodes identified by their index
         * @param event_time Total time of propagation arrival after event start
         * @param deposited_charge Optional pointer to related deposited charge
         */
        PropagatedCharge(ROOT::Math::XYZPoint local_position,
                         ROOT::Math::XYZPoint global_position,
                         CarrierType type,
                         std::map<Pixel::Index, Pulse> pulses,
                         double event_time,
                         const DepositedCharge* deposited_charge = nullptr);

        /**
         * @brief Get related deposited charge
         * @return Pointer to possible deposited charge
         */
        const DepositedCharge* getDepositedCharge() const;

        /**
         * @brief Get related Monte-Carlo particle
         * @return Pointer to Monte-Carlo particle
         */
        const MCParticle* getMCParticle() const;

        /**
         * @brief Get related induced pulses
         * @return Map with induced pulses if available
         */
        std::map<Pixel::Index, Pulse> getPulses() const;

        /**
         * @brief Print an ASCII representation of PropagatedCharge to the given stream
         * @param out Stream to print to
         */
        void print(std::ostream& out) const override;

        /**
         * @brief ROOT class definition
         */
        ClassDefOverride(PropagatedCharge, 5);
        /**
         * @brief Default constructor for ROOT I/O
         */
        PropagatedCharge() = default;

        void petrifyHistory() override;

    private:
        ReferenceWrapper<DepositedCharge> deposited_charge_;
        ReferenceWrapper<MCParticle> mc_particle_;

        std::map<Pixel::Index, Pulse> pulses_;
    };

    /**
     * @brief Typedef for message carrying propagated charges
     */
    using PropagatedChargeMessage = Message<PropagatedCharge>;
} // namespace allpix

#endif /* ALLPIX_PROPAGATED_CHARGE_H */
