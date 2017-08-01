/**
 * @file
 * @brief Implementation of object with a cluster of PixelHits
 * @copyright MIT License
 */

#include "Cluster.hpp"

#include <set>

#include "core/utils/log.h"

using namespace allpix;

Cluster::Cluster(const PixelHit* seedPixelHit) : seedPixelHit_(seedPixelHit) {
    pixelHits_.insert(seedPixelHit);
    clusterCharge_ = seedPixelHit->getSignal();
}

bool Cluster::addPixelHit(const PixelHit* pixelHit) {
    auto ret = pixelHits_.insert(pixelHit);
    if(ret.second == true) {
        clusterCharge_ += pixelHit->getSignal();
        return true;
    }
    return false;
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
