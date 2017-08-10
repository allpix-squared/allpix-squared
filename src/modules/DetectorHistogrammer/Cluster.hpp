/**
 * @file
 * @brief Definition of object with a cluster containing several PixelHits
 * @copyright MIT License
 */

#ifndef ALLPIX_DETECTOR_HISTOGRAMMER_CLUSTER_H
#define ALLPIX_DETECTOR_HISTOGRAMMER_CLUSTER_H

#include <Math/DisplacementVector2D.h>

#include <set>

#include "objects/Pixel.hpp"
#include "objects/PixelHit.hpp"

namespace allpix {

    class Cluster {
    public:
        /**
         * @brief Construct a cluster
         * @param seedPixelHit PixelHit to start the cluster with
         */
        Cluster(const PixelHit* seedPixelHit);

        /**
         * @brief Get the signal data for the hit
         * @return Digitized signal
         */
        double getClusterCharge() const { return clusterCharge_; }

        /**
         * @brief Add PixelHit to the cluster
         * @param pixelHit PixelHit to be added
         */
        bool addPixelHit(const PixelHit* pixelHit);

        /**
         * @brief Get the cluster size
         * @return cluster size
         */
        unsigned long int getClusterSize() const { return pixelHits_.size(); }

        /**
         * @brief Get the cluster sizes in x and y
         * @return pair of cluster size x and y
         */
        std::pair<unsigned int, unsigned int> getClusterSizeXY();

        /**
         * @brief Get the weighted mean cluster position
         * @return weighted mean cluster position
         */
        ROOT::Math::XYVectorD getClusterPosition();

        /**
         * @brief Get the seed PixelHit
         * @return Pointer to PixelHit
         */
        const PixelHit* getSeedPixelHit() const { return seedPixelHit_; }

        /**
         * @brief Get the PixelHits contained in this cluster
         * @return List of all contained PixelHits
         */
        std::set<const PixelHit*> getPixelHits() const { return pixelHits_; }

    private:
        const PixelHit* seedPixelHit_;

        std::set<const PixelHit*> pixelHits_;
        double clusterCharge_{};

        unsigned int minX_, minY_, maxX_, maxY_;
    };
}
#endif
