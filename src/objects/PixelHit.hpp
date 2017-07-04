/**
 * @file
 * @brief Definition of object with digitized pixel hit
 * @copyright MIT License
 */

#ifndef ALLPIX_PIXEL_HIT_H
#define ALLPIX_PIXEL_HIT_H

#include <Math/DisplacementVector2D.h>

#include <TRef.h>

#include "MCParticle.hpp"
#include "Object.hpp"
#include "PixelCharge.hpp"

namespace allpix {
    /**
     * @brief Pixel triggered in an event after digitization
     */
    class PixelHit : public Object {
    public:
        using Pixel = ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>>;

        /**
         * @brief Construct a digitized pixel hit
         * @param pixel Location of the pixel
         * @param time Timing of the occurence of the hit
         * @param signal Signal data produced by the digitizer
         * @param pixel_charge Optional pointer to the related pixel charge
         */
        PixelHit(Pixel pixel, double time, double signal, const PixelCharge* pixel_charge = nullptr);

        /**
         * @brief Get the pixel hit
         * @return Pixel indices in the grid
         */
        PixelHit::Pixel getPixel() const;
        /**
         * @brief Get the timing data of the hit
         * @return Timestamp
         */
        double getTime() const { return time_; }
        /**
         * @brief Get the signal data for the hit
         * @return Digitized signal
         */
        double getSignal() const { return signal_; }

        /**
         * @brief Get related pixel charge
         * @return Possible related pixel charge
         */
        const PixelCharge* getPixelCharge() const;
        /**
         * @brief Get the Monte-Carlo particles resulting in this pixel hit
         * @return List of all related Monte-Carlo particles
         */
        std::vector<const MCParticle*> getMCParticles() const;

        /**
         * @brief ROOT class definition
         */
        ClassDef(PixelHit, 1);
        /**
         * @brief Default constructor for ROOT I/O
         */
        PixelHit() = default;

    private:
        Pixel pixel_;
        double time_{};
        double signal_{};

        TRef pixel_charge_;
    };

    /**
     * @brief Typedef for message carrying pixel hits
     */
    using PixelHitMessage = Message<PixelHit>;
} // namespace allpix

#endif
