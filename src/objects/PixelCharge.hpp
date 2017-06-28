/**
 * Message for a charge at a pixel in the detector
 *
 * FIXME: better name
 *
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_PIXEL_CHARGE_H
#define ALLPIX_PIXEL_CHARGE_H

#include <Math/DisplacementVector2D.h>

#include <TRefArray.h>

#include "Object.hpp"
#include "PropagatedCharge.hpp"

namespace allpix {
    // object definition
    class PixelCharge : public Object {
    public:
        using Pixel = ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>>;

        PixelCharge(Pixel pixel,
                    unsigned int charge,
                    std::vector<const PropagatedCharge*> propagated_charges = std::vector<const PropagatedCharge*>());

        PixelCharge::Pixel getPixel() const;
        unsigned int getCharge() const;

        std::vector<const PropagatedCharge*> getPropagatedCharges() const;

        ClassDef(PixelCharge, 1);
        PixelCharge() = default;

    private:
        Pixel pixel_;
        unsigned int charge_{};

        TRefArray propagated_charges_;
    };

    // link to the carrying message
    using PixelChargeMessage = Message<PixelCharge>;
} // namespace allpix

#endif
