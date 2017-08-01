/**
 * @file
 * @brief Implementation of object with a cluster of PixelHits
 * @copyright MIT License
 */

#include "Cluster.hpp"

#include <set>

#include "core/utils/log.h"
#include "exceptions.h"

using namespace allpix;

Cluster::Cluster(const PixelHit* seedPixelHit) : seedPixelHit_(seedPixelHit) {
    pixelHits_.insert(seedPixelHit);
    clusterCharge_ = seedPixelHit->getSignal();
}

void Cluster::addPixelHit(const PixelHit* pixelHit) {
    pixelHits_.insert(pixelHit);
    clusterCharge_ += pixelHit->getSignal();
}

ROOT::Math::XYVectorD Cluster::getClusterPosition() {
    ROOT::Math::XYVectorD meanPos = ROOT::Math::XYVectorD(0., 0.);
    for(auto& pixel : this->getPixelHits()) {
        meanPos += ROOT::Math::XYVectorD(pixel->getPixel().getIndex().x() * pixel->getSignal(),
                                         pixel->getPixel().getIndex().y() * pixel->getSignal());
    }
    meanPos /= getClusterCharge();
    return meanPos;
}

ClassImp(PixelHit)
