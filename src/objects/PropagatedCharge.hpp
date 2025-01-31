/**
 * @file
 * @brief Definition of propagated charge object
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
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
     * @brief State of the charge carrier
     */
    enum class CarrierState {
        UNKNOWN = 0, ///< State of the propagated charge carrier is unknown
        MOTION,      ///< The propagated charge carrier is in motion
        RECOMBINED,  ///< The propagated charge carrier has recombined with the lattice
        TRAPPED,     ///< The propagated charge carrier is trapped temporarily
        HALTED, ///< The carrier has come to a halt because it, for example, has reached the sensor surface or an implant
    };

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
         * @param local_time Time of propagation arrival after energy deposition, local reference frame
         * @param global_time Total time of propagation arrival after event start, global reference frame
         * @param state State of the charge carrier when reaching its position
         * @param deposited_charge Optional pointer to related deposited charge
         */
        PropagatedCharge(ROOT::Math::XYZPoint local_position,
                         ROOT::Math::XYZPoint global_position,
                         CarrierType type,
                         unsigned int charge,
                         double local_time,
                         double global_time,
                         CarrierState state = CarrierState::UNKNOWN,
                         const DepositedCharge* deposited_charge = nullptr);

        /**
         * @brief Construct a set of propagated charges
         * @param local_position Local position of the propagated set of charges in the sensor
         * @param global_position Global position of the propagated set of charges in the sensor
         * @param type Type of the carrier to propagate
         * @param pulses Map of pulses induced at electrodes identified by their index
         * @param local_time Time of propagation arrival after energy deposition, local reference frame
         * @param global_time Total time of propagation arrival after event start, global reference frame
         * @param state State of the charge carrier when reaching its position
         * @param deposited_charge Optional pointer to related deposited charge
         */
        PropagatedCharge(ROOT::Math::XYZPoint local_position,
                         ROOT::Math::XYZPoint global_position,
                         CarrierType type,
                         std::map<Pixel::Index, Pulse> pulses,
                         double local_time,
                         double global_time,
                         CarrierState state = CarrierState::UNKNOWN,
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
         * @brief Get state of the charge carrier
         * @return Charge carrier state
         */
        CarrierState getState() const;

        /**
         * @brief Print an ASCII representation of PropagatedCharge to the given stream
         * @param out Stream to print to
         */
        void print(std::ostream& out) const override;

        /**
         * @brief ROOT class definition
         */
        ClassDefOverride(PropagatedCharge, 7); // NOLINT
        /**
         * @brief Default constructor for ROOT I/O
         */
        PropagatedCharge() = default;

        void loadHistory() override;
        void petrifyHistory() override;

    private:
        PointerWrapper<DepositedCharge> deposited_charge_;
        PointerWrapper<MCParticle> mc_particle_;

        std::map<Pixel::Index, Pulse> pulses_;

        CarrierState state_{CarrierState::UNKNOWN};
    };

    /**
     * @brief Typedef for message carrying propagated charges
     */
    using PropagatedChargeMessage = Message<PropagatedCharge>;
} // namespace allpix

#endif /* ALLPIX_PROPAGATED_CHARGE_H */
