/**
 * @file
 * @brief Definition of object with a cluster containing several PixelHits
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_DETECTOR_HISTOGRAMMER_CLUSTER_H
#define ALLPIX_DETECTOR_HISTOGRAMMER_CLUSTER_H

#include <Math/Vector3D.h>

#include <set>

#include "objects/Pixel.hpp"
#include "objects/PixelHit.hpp"

namespace allpix {

    class Cluster {
    public:
        /**
         * @brief Construct a cluster
         * @param seed_pixel_hit PixelHit to start the cluster with
         */
        explicit Cluster(const PixelHit* seed_pixel_hit);

        /**
         * @brief Get the total accumulated signal of the cluster
         * @return Total cluster charge
         */
        double getCharge() const { return cluster_charge_; }

        /**
         * @brief Add a PixelHit to the cluster
         * @param pixel_hit PixelHit to be added
         */
        bool addPixelHit(const PixelHit* pixel_hit);

        /**
         * @brief Get the cluster size
         * @return cluster size
         */
        unsigned long int getSize() const { return pixel_hits_.size(); }

        /**
         * @brief Get the cluster sizes in x and y
         * @return pair of cluster size x and y
         */
        std::pair<unsigned int, unsigned int> getSizeXY() const;

        /**
         * @brief Get the charge-weighted mean cluster position in local coordinates
         * @return weighted mean cluster position
         */
        ROOT::Math::XYZPoint getPosition() const;

        /**
         * @brief Get the seed PixelHit, i.e. the PixelHit with the largest charge
         * @return Pointer to PixelHit
         */
        const PixelHit* getSeedPixelHit() const { return seed_pixel_hit_; }

        /**
         * @brief Get the PixelHit at coordinates x and y
         * @param  x Coordinate of the pixel in x direction
         * @param  y Coordinate of the pixel in y direction
         * @return Pointer to matching PixelHit if available or a nullptr if not part of the cluster
         */
        const PixelHit* getPixelHit(int x, int y) const;

        /**
         * @brief Get the PixelHits contained in this cluster
         * @return List of all contained PixelHits
         */
        const std::set<const PixelHit*>& getPixelHits() const { return pixel_hits_; }

        /**
         * @brief Get all MCParticles related to the cluster
         * @note MCParticles can only be fetched if the full history of objects are in scope and stored
         * @return Vector of all related MCParticles
         */
        const std::set<const MCParticle*>& getMCParticles() const { return mc_particles_; }

    private:
        const PixelHit* seed_pixel_hit_;

        std::set<const PixelHit*> pixel_hits_;
        std::set<const MCParticle*> mc_particles_;

        double cluster_charge_{};

        int minX_, minY_, maxX_, maxY_;
    };
} // namespace allpix
#endif /*ALLPIX_DETECTOR_HISTOGRAMMER_CLUSTER_H */
