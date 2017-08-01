/**
 * @file
 * @brief Definition of object with a cluster containing several PixelHits
 * @copyright MIT License
 */

#ifndef ALLPIX_CLUSTER_H
#define ALLPIX_CLUSTER_H

#include <Math/DisplacementVector2D.h>

#include <TRef.h>

#include <set>

#include "MCParticle.hpp"
#include "Object.hpp"
#include "Pixel.hpp"
#include "PixelHit.hpp"

namespace allpix {
    /**
     * @ingroup Objects
     * @brief Cluster of PixelHits
     */
    class Cluster : public Object {
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
        void addPixelHit(const PixelHit* pixelHit);

        /**
             * @brief Get the cluster size
             * @return cluster size
             */
        unsigned long int getClusterSize() const { return pixelHits_.size(); }

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

        /**
         * @brief ROOT class definition
         */
        ClassDef(Cluster, 2);

        /**
         * @brief Default constructor for ROOT I/O
         */
        Cluster() = default;

    private:
        const PixelHit* seedPixelHit_;

        std::set<const PixelHit*> pixelHits_;
        double clusterCharge_{};
        double clusterPosition_{};
    };

} // namespace allpix

#endif
