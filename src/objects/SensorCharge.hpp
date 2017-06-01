/**
 * Message for a charge in a sensor
 *
 * NOTE: should not be directly used?
 *
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_SENSOR_CHARGE_H
#define ALLPIX_SENSOR_CHARGE_H

#include <Math/Point3D.h>

#include "Object.hpp"

namespace allpix {
    // type of the deposits
    // FIXME: better name here?
    class SensorCharge : public Object {
    public:
        // constructor and destructor
        SensorCharge(ROOT::Math::XYZPoint position, unsigned int charge, long double eventTime);
        ~SensorCharge() override;

        SensorCharge(const SensorCharge&);
        SensorCharge& operator=(const SensorCharge&);

        SensorCharge(SensorCharge&&) noexcept;
        SensorCharge& operator=(SensorCharge&&) noexcept;

        // FIXME: should position be in local coordinates or global coordinates?
        ROOT::Math::XYZPoint getPosition() const;
        unsigned int getCharge() const;

        long double getEventTime() const;

        SensorCharge() = default;

    private:
        ROOT::Math::XYZPoint position_;
        unsigned int charge_;
        long double event_time_;

        ClassDef(SensorCharge, 1);
    };
} // namespace allpix

#endif /* ALLPIX_SENSOR_CHARGE_H */
