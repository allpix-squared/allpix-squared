/**
 * @file
 * @brief Implementation of object for charges in sensor
 *
 * @copyright Copyright (c) 2017-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_SENSOR_CHARGE_H
#define ALLPIX_SENSOR_CHARGE_H

#include <Math/Point3D.h>

#include "Object.hpp"

namespace allpix {
    /**
     * @ingroup Objects
     * @brief Flags to distinguish between electron and hole charge carriers
     */
    enum class CarrierType : int8_t { ELECTRON = -1, HOLE = 1 };

    inline std::ostream& operator<<(std::ostream& os, const CarrierType type) {
        os << (type == CarrierType::ELECTRON ? "\"e\"" : "\"h\"");
        return os;
    }

    /**
     * @brief Invert the type of a charge carrier
     * @param type Initial type of the charge carrier
     * @return Inverted type of the charge carrier
     */
    inline CarrierType invertCarrierType(const CarrierType& type) {
        return (type == CarrierType::ELECTRON ? CarrierType::HOLE : CarrierType::ELECTRON);
    }

    /**
     * @ingroup Objects
     * @brief Base object for charge deposits and propagated charges in the sensor
     */
    class SensorCharge : public Object {
    public:
        /**
         * @brief Construct a set of charges in a sensor
         * @param local_position Local position of the set of charges in the sensor
         * @param global_position Global position of the set of charges in the sensor
         * @param type Type of the carrier
         * @param charge Total charge at position
         * @param local_time Time in local sensor reference
         * @param global_time Total time after event start in global reference system
         */
        SensorCharge(ROOT::Math::XYZPoint local_position,
                     ROOT::Math::XYZPoint global_position,
                     CarrierType type,
                     unsigned int charge,
                     double local_time,
                     double global_time);

        /**
         * @brief Get local position of the set of charges in the sensor
         * @return Local position of charges
         */
        ROOT::Math::XYZPoint getLocalPosition() const;

        /**
         * @brief Get the global position of the set of charges in the sensor
         */
        ROOT::Math::XYZPoint getGlobalPosition() const;

        /**
         * @brief Get the type of charge carrier
         * @return Type of charge carrier
         */
        CarrierType getType() const;
        /**
         * @brief Get total amount of charges stored
         * @return Total charge stored
         */
        unsigned int getCharge() const;

        /**
         * @brief Get the sign of the charge for set of charge carriers
         * @return Sign of the charge
         */
        long getSign() const;

        /**
         * @brief Get time after start of event in global reference frame
         * @return Time from start event
         */
        double getGlobalTime() const;

        /**
         * @brief Get local time in the sensor
         * @return Time with respect to local sensor
         */
        double getLocalTime() const;

        /**
         * @brief Print an ASCII representation of SensorCharge to the given stream
         * @param out Stream to print to
         */
        void print(std::ostream& out) const override;

        /**
         * @brief ROOT class definition
         */
        ClassDefOverride(SensorCharge, 3); // NOLINT
        /**
         * @brief Default constructor for ROOT I/O
         */
        SensorCharge() = default;

    private:
        ROOT::Math::XYZPoint local_position_;
        ROOT::Math::XYZPoint global_position_;

        double local_time_{};
        double global_time_{};

        CarrierType type_{};
        unsigned int charge_{};
    };
} // namespace allpix

#endif /* ALLPIX_SENSOR_CHARGE_H */
