/**
 * @file
 * @brief Implementation of object with a cluster of PixelHits
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "Cluster.hpp"

#include <algorithm>
#include <set>

#include "core/utils/log.h"
#include "objects/exceptions.h"

using namespace allpix;

Cluster::Cluster(const PixelHit* seedPixelHit) : seedPixelHit_(seedPixelHit) {
    pixelHits_.insert(seedPixelHit);
    clusterCharge_ = seedPixelHit->getSignal();

    minX_ = seedPixelHit->getPixel().getIndex().x();
    maxX_ = minX_;
    minY_ = seedPixelHit->getPixel().getIndex().y();
    maxY_ = minY_;

    for(const auto* mc_particle : seedPixelHit->getMCParticles()) {
        mc_particles_.insert(mc_particle);
    }
}

bool Cluster::addPixelHit(const PixelHit* pixelHit) {
    auto ret = pixelHits_.insert(pixelHit);
    if(ret.second == true) {
        clusterCharge_ += pixelHit->getSignal();
        unsigned int pixX = pixelHit->getPixel().getIndex().x();
        unsigned int pixY = pixelHit->getPixel().getIndex().y();
        minX_ = std::min(pixX, minX_);
        maxX_ = std::max(pixX, maxX_);
        minY_ = std::min(pixY, minY_);
        maxY_ = std::max(pixY, maxY_);

        for(const auto* mc_particle : pixelHit->getMCParticles()) {
            mc_particles_.insert(mc_particle);
        }

        return true;
    }
    return false;
}

ROOT::Math::XYVectorD Cluster::getPosition() const {
    ROOT::Math::XYVectorD meanPos = ROOT::Math::XYVectorD(0., 0.);
    for(const auto& pixel : this->getPixelHits()) {
        meanPos += ROOT::Math::XYVectorD(pixel->getPixel().getIndex().x() * pixel->getSignal(),
                                         pixel->getPixel().getIndex().y() * pixel->getSignal());
    }
    meanPos /= getCharge();
    return meanPos;
}

std::pair<unsigned int, unsigned int> Cluster::getSizeXY() const {
    std::pair<unsigned int, unsigned int> sizes = std::make_pair(maxX_ - minX_ + 1, maxY_ - minY_ + 1);
    return sizes;
}

/**
 * MCParticles can only be fetched if the full history of objects are in scope and stored
 */
std::set<const MCParticle*> Cluster::getMCParticles() const {
    return mc_particles_;
}

const PixelHit* Cluster::getPixelHit(unsigned int x, unsigned int y) const {
    for(const auto& pixel : this->getPixelHits()) {
        if(pixel->getPixel().getIndex().x() == x && pixel->getPixel().getIndex().y() == y) {
            return pixel;
        }
    }
    return nullptr;
}
