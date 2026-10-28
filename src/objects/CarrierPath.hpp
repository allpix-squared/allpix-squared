/**
 * @file
 * @brief Definition of carrier path object
 *
 * @copyright Copyright (c) 2026 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_CARRIER_PATH_H
#define ALLPIX_CARRIER_PATH_H

#include <map>

#include "DepositedCharge.hpp"
#include "Object.hpp"
#include "PropagatedCharge.hpp"

namespace allpix {

    /**
     * @ingroup Objects
     * @brief Path of a set of charges through the sensor
     */
    class CarrierPath : public Object {
    public:
        /**
         * @brief Construct a carrier path object
         * @param charge Total charge propagated
         * @param type Type of the carrier to propagate
         * @param state State of the charge carrier when reaching its position
         * @param deposited_charge Optional pointer to related deposited charge
         */
        CarrierPath(unsigned int charge,
                    CarrierType type,
                    CarrierState state = CarrierState::UNKNOWN,
                    const DepositedCharge* deposited_charge = nullptr);

        /**
         * @brief Get related deposited charge
         * @return Pointer to possible deposited charge
         */
        const DepositedCharge* getDepositedCharge() const;

        /**
         * @brief Get related propagated charge
         * @return Pointer to possible propagated charge
         */
        const PropagatedCharge* getPropagatedCharge() const;

        /**
         * @brief Set the propagated charge
         * @param propagated_charge The propagated charge
         * @warning Special method because PropagatedCharge is only known after deposit creation, do not be replaced later.
         */
        void setPropagatedCharge(const PropagatedCharge* propagated_charge);

        /**
         * @brief Update the current state of the carrier
         * @param state New state of the carrier
         */
        void updateState(CarrierState state) { state_ = state; }

        /**
         * @brief Add a point to the carrier trajectory
         * @param position New position in local coordinates to add to trajectory
         * @param time Time in local frame of reference when this position was reached
         */
        void addPoint(ROOT::Math::XYZPoint position, double time) { points_.emplace_back(std::move(position), time); }

        /**
         * @brief Obtain the deposition time of the carrier in local time reference
         * @details This method will use the deposition time of the related DepositedCharge object if available and return
         *          the first timestamp of the trajectory otherwise. If neither is available, zero is returned.
         * @return Deposition time of this charge carrier
         */
        double getDepositionTime() const;

        /**
         * @brief Get the total charge associated with this carrier path
         * @return Total charge
         */
        unsigned int getCharge() const { return charge_; }

        /**
         * @brief Get the type of charge carrier associated with this path
         * @return Carrier type
         */
        CarrierType getType() const { return type_; }

        /**
         * @brief Get the current state of the carrier associated with this path
         * @return State of the carrier
         */
        CarrierState getState() const { return state_; }

        /**
         * @brief Obtain all points of the carrier path
         * @return Reference to the vector of path points
         */
        const std::vector<std::pair<ROOT::Math::XYZPoint, double>>& getPoints() const { return points_; }

        /**
         * @brief Get the number of points stored on the path
         * @return Number of stored points
         */
        size_t getNPoints() const { return points_.size(); }

        /**
         * @brief Print an ASCII representation of CarrierPath to the given stream
         * @param out Stream to print to
         */
        void print(std::ostream& out) const override;

        /**
         * @brief ROOT class definition
         */
        ClassDefOverride(CarrierPath, 1); // NOLINT
        /**
         * @brief Default constructor for ROOT I/O
         */
        CarrierPath() = default;

        void loadHistory() override;
        void petrifyHistory() override;

    private:
        unsigned int charge_{};
        CarrierType type_{};
        CarrierState state_{};
        PointerWrapper<DepositedCharge> deposited_charge_;
        PointerWrapper<PropagatedCharge> propagated_charge_;
        std::vector<std::pair<ROOT::Math::XYZPoint, double>> points_;
    };

    /**
     * @brief Typedef for message carrying carrier paths
     */
    using CarrierPathMessage = Message<CarrierPath>;
} // namespace allpix

#endif /* ALLPIX_CARRIER_PATH_H */
