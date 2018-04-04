/**
 * @file
 * @brief Definition of detector histogramming module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_MODULE_DETECTOR_HISTOGRAMMER_H
#define ALLPIX_MODULE_DETECTOR_HISTOGRAMMER_H

#include <memory>

#include <string>
#include <vector>

#include <TH1I.h>
#include <TH2I.h>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

#include "Cluster.hpp"
#include "objects/PixelHit.hpp"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to plot the final digitized pixel data
     *
     * Generates a hitmap of all the produced pixel hits, together with a histogram of the cluster size
     */
    class DetectorHistogrammerModule : public Module {
    public:
        /**
         * @brief Constructor for this detector-specific module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param detector Pointer to the detector for this module instance
         */
        DetectorHistogrammerModule(Configuration& config, Messenger*, std::shared_ptr<Detector>);

        /**
         * @brief Initialize the histograms
         */
        void init() override;

        /**
         * @brief Fill the histograms
         */
        void run(unsigned int) override;

        /**
         * @brief Write the histograms to the modules file
         */
        void finalize() override;

    private:
        /**
         * @brief Perform a sparse clustering on the PixelHits
         */
        std::vector<Cluster> doClustering();

        std::shared_ptr<Detector> detector_;

        // List of pixel hits
        std::shared_ptr<PixelHitMessage> pixels_message_;

        // Statistics to compute mean position
        ROOT::Math::XYVector total_vector_{};
        unsigned long total_hits_{};

        // Histograms to output
        TH2D* hit_map;
        TH2D* cluster_map;
        TH1D* event_size;
        TH1D* cluster_size;
        TH1D* cluster_size_x;
        TH1D* cluster_size_y;
        TH1D* n_cluster;
        TH1D* cluster_charge;
    };
} // namespace allpix

#endif /* ALLPIX_MODULE_DETECTOR_HISTOGRAMMER_H */
