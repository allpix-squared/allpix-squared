/**
 * @file
 * @brief Implementation of object for charges in sensor
 * @copyright MIT License
 */

#ifndef ALLPIX_SENSOR_CHARGE_H
#define ALLPIX_SENSOR_CHARGE_H

#include <Math/Point3D.h>

#include "Object.hpp"

namespace allpix {
    /**
     * @brief Base object for charge deposits and propagated charges in the sensor
     */
    class SensorCharge : public Object {
    public:
        /**
         * @brief Construct a set of charges in a sensor
         * @param position Local position of the set of charges in the sensor
         * @param charge Total charge at position
         * @param event_time Total time after event start
         */
        SensorCharge(ROOT::Math::XYZPoint position, unsigned int charge, double eventTime);

        /**
         * @brief Get local position of set of charges in the sensor
         * @return Local position of charges
         */
        ROOT::Math::XYZPoint getPosition() const;
        /**
         * @brief Get total amount of charges stored
         * @return Total charge stored
         */
        unsigned int getCharge() const;
        /**
         * @brief Get time after start of event
         * @return Time from start event
         */
        double getEventTime() const;

        /**
         * @brief ROOT class definition
         */
        ClassDef(SensorCharge, 1);
        /**
         * @brief Default constructor for ROOT I/O
         */
        SensorCharge() = default;

    private:
        ROOT::Math::XYZPoint position_;
        unsigned int charge_{};
        double event_time_{};
    };
} // namespace allpix

#endif /* ALLPIX_SENSOR_CHARGE_H */
