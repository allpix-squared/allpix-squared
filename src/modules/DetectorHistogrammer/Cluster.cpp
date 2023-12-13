/**
 * @file
 * @brief Implementation of object with a cluster of PixelHits
 *
 * @copyright Copyright (c) 2017-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "Cluster.hpp"

#include <algorithm>
#include <cmath>
#include <set>

#include "core/utils/log.h"
#include "objects/exceptions.h"

using namespace allpix;

Cluster::Cluster(const PixelHit* seed_pixel_hit) : seed_pixel_hit_(seed_pixel_hit) {
    pixel_hits_.insert(seed_pixel_hit);
    cluster_charge_ = seed_pixel_hit->getSignal();

    minX_ = seed_pixel_hit->getPixel().getIndex().x();
    maxX_ = minX_; // NOLINT
    minY_ = seed_pixel_hit->getPixel().getIndex().y();
    maxY_ = minY_; // NOLINT

    for(const auto* mc_particle : seed_pixel_hit->getMCParticles()) {
        mc_particles_.insert(mc_particle);
    }
}

bool Cluster::addPixelHit(const PixelHit* pixel_hit) {
    auto ret = pixel_hits_.insert(pixel_hit);
    if(ret.second == true) {
        cluster_charge_ += pixel_hit->getSignal();
        auto pixX = pixel_hit->getPixel().getIndex().x();
        auto pixY = pixel_hit->getPixel().getIndex().y();
        minX_ = std::min(pixX, minX_);
        maxX_ = std::max(pixX, maxX_);
        minY_ = std::min(pixY, minY_);
        maxY_ = std::max(pixY, maxY_);

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
    std::pair<unsigned int, unsigned int> sizes = std::make_pair(maxX_ - minX_ + 1, maxY_ - minY_ + 1);
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
