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

#include "core/messenger/Message.hpp"

#include "Object.hpp"

namespace allpix {
    // type of the deposits
    // FIXME: better name here?
    class SensorCharge : public Object {
    public:
        // constructor and destructor
        SensorCharge(ROOT::Math::XYZPoint position, unsigned int charge);

        // FIXME: should position be in local coordinates or global coordinates?
        ROOT::Math::XYZPoint getPosition() const;
        unsigned int getCharge() const;

    private:
        ROOT::Math::XYZPoint position_;
        unsigned int charge_;
    };
} // namespace allpix

#endif /* ALLPIX_SENSOR_CHARGE_H */
