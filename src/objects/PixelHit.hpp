/**
 * Message for a charge at a pixel in the detector
 *
 * FIXME: better name
 *
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_PIXEL_HIT_H
#define ALLPIX_PIXEL_HIT_H

#include <Math/DisplacementVector2D.h>

#include <TRef.h>

#include "MCParticle.hpp"
#include "Object.hpp"
#include "PixelCharge.hpp"

namespace allpix {
    // object definition
    class PixelHit : public Object {
    public:
        using Pixel = ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>>;

        explicit PixelHit(Pixel pixel, double time, double signal, const PixelCharge* pixel_charge = nullptr);

        PixelHit::Pixel getPixel() const;
        double getTime() const { return time_; }
        double getSignal() const { return signal_; }

        const PixelCharge* getPixelCharge() const;
        std::vector<const MCParticle*> getMCParticles() const;

        ClassDef(PixelHit, 1);
        PixelHit() = default;

    private:
        Pixel pixel_;
        double time_{};
        double signal_{};

        TRef pixel_charge_;
    };

    // link to the carrying message
    using PixelHitMessage = Message<PixelHit>;
} // namespace allpix

#endif
