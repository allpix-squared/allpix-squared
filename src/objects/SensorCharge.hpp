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
     * @ingroup Objects
     * @brief Flags to distinguish between eletron and hole charge carriers
     */
    enum class CarrierType : int8_t { ELECTRON = -1, HOLE = 1 };

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
         * @param charge Total charge at position
         * @param event_time Total time after event start
         */
        SensorCharge(ROOT::Math::XYZPoint local_position,
                     ROOT::Math::XYZPoint global_position,
                     unsigned int charge,
                     double event_time);

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
        ClassDef(SensorCharge, 2);
        /**
         * @brief Default constructor for ROOT I/O
         */
        SensorCharge() = default;

    private:
        ROOT::Math::XYZPoint local_position_;
        ROOT::Math::XYZPoint global_position_;

        CarrierType type_{};
        unsigned int charge_{};
        double event_time_{};
    };
} // namespace allpix

#endif /* ALLPIX_SENSOR_CHARGE_H */
