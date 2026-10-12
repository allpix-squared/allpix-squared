/**
 * @file
 * @brief Implementation of object with a cluster of PixelHits
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "Cluster.hpp"

#include <algorithm>
#include <cmath>
#include <set>
#include <utility>

#include <Math/Point3Dfwd.h>
#include <Math/Vector3Dfwd.h>

#include "objects/PixelHit.hpp"

using namespace allpix;

Cluster::Cluster(const PixelHit* seed_pixel_hit)
    : seed_pixel_hit_(seed_pixel_hit), cluster_charge_(seed_pixel_hit->getSignal()) {
    pixel_hits_.insert(seed_pixel_hit);

    min_x_ = seed_pixel_hit->getPixel().getIndex().x();
    max_x_ = min_x_; // NOLINT
    min_y_ = seed_pixel_hit->getPixel().getIndex().y();
    max_y_ = min_y_; // NOLINT

    for(const auto* mc_particle : seed_pixel_hit->getMCParticles()) {
        mc_particles_.insert(mc_particle);
    }
}

bool Cluster::addPixelHit(const PixelHit* pixel_hit) {
    auto ret = pixel_hits_.insert(pixel_hit);
    if(ret.second) {
        cluster_charge_ += pixel_hit->getSignal();
        auto pixX = pixel_hit->getPixel().getIndex().x();
        auto pixY = pixel_hit->getPixel().getIndex().y();
        min_x_ = std::min(pixX, min_x_);
        max_x_ = std::max(pixX, max_x_);
        min_y_ = std::min(pixY, min_y_);
        max_y_ = std::max(pixY, max_y_);

        // Update seed pixel if new charge is larger:
        if(std::signbit(seed_pixel_hit_->getSignal()) == std::signbit(pixel_hit->getSignal()) &&
           std::abs(seed_pixel_hit_->getSignal()) < std::abs(pixel_hit->getSignal())) {
            seed_pixel_hit_ = pixel_hit;
        }

        for(const auto* mc_particle : pixel_hit->getMCParticles()) {
            mc_particles_.insert(mc_particle);
        }

        return true;
    }
    return false;
}

ROOT::Math::XYZPoint Cluster::getPosition() const {
    ROOT::Math::XYZVector meanPos;
    for(const auto& pixel : this->getPixelHits()) {
        meanPos = pixel->getPixel().getLocalCenter() * pixel->getSignal() + meanPos;
    }
    meanPos /= getCharge();
    return static_cast<ROOT::Math::XYZPoint>(meanPos);
}

std::pair<unsigned int, unsigned int> Cluster::getSizeXY() const {
    std::pair<unsigned int, unsigned int> sizes = std::make_pair(max_x_ - min_x_ + 1, max_y_ - min_y_ + 1);
    return sizes;
}

const PixelHit* Cluster::getPixelHit(int x, int y) const {
    for(const auto& pixel : this->getPixelHits()) {
        if(pixel->getPixel().getIndex().x() == x && pixel->getPixel().getIndex().y() == y) {
            return pixel;
        }
    }
    return nullptr;
}
