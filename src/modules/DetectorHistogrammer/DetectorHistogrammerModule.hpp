/**
 * @file
 * @brief Definition of detector histogramming module
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_MODULE_DETECTOR_HISTOGRAMMER_H
#define ALLPIX_MODULE_DETECTOR_HISTOGRAMMER_H

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include <TH1I.h>
#include <TH2I.h>
#include <TProfile.h>
#include <TProfile2D.h>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Event.hpp"
#include "core/module/Module.hpp"

#include "Cluster.hpp"
#include "objects/PixelHit.hpp"
#include "tools/ROOT.h"

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
        void initialize() override;

        /**
         * @brief Fill the histograms
         */
        void run(Event*) override;

        /**
         * @brief Write the histograms to the modules file
         */
        void finalize() override;

    private:
        /**
         * @brief Perform a sparse clustering on the PixelHits
         */
        std::vector<Cluster> doClustering(std::shared_ptr<PixelHitMessage>& pixels_message) const;

        /**
         * @brief analyze the available MCParticles and return the all particles identified as primary (i.e. that do not have
         * a parent). This might be several particles.
         */
        static std::vector<const MCParticle*> getPrimaryParticles(std::shared_ptr<MCParticleMessage>& mcparticle_message);

        Messenger* messenger_;

        std::shared_ptr<Detector> detector_;

        // Statistics
        std::atomic<unsigned long> total_hits_;

        // Cut criteria for efficiency measurement:
        ROOT::Math::XYVector matching_cut_;

        // Reference track resolution
        ROOT::Math::XYVector track_resolution_;

        // Histograms to output
        Histogram<TH2D> hit_map, hit_map_global, hit_map_global_mc, hit_map_local, hit_map_local_mc, charge_map, cluster_map,
            polar_hit_map;
        Histogram<TProfile2D> cluster_size_map_local, cluster_size_map, cluster_size_x_map, cluster_size_y_map;
        Histogram<TProfile2D> cluster_charge_map, seed_charge_map;
        Histogram<TProfile2D> residual_map, residual_x_map, residual_y_map, residual_detector, residual_x_detector,
            residual_y_detector;
        Histogram<TH1D> residual_x, residual_y, residual_r, residual_phi;
        Histogram<TProfile> residual_x_vs_x, residual_y_vs_y, residual_x_vs_y, residual_y_vs_x;
        Histogram<TProfile2D> efficiency_map, efficiency_local, efficiency_detector;
        Histogram<TProfile> efficiency_vs_x, efficiency_vs_y;
        Histogram<TH1D> event_size;
        Histogram<TH1D> cluster_size, cluster_size_x, cluster_size_y;
        Histogram<TH1D> n_cluster;
        Histogram<TH1D> cluster_charge, pixel_charge, total_charge, cluster_seed_charge;
    };
} // namespace allpix

#endif /* ALLPIX_MODULE_DETECTOR_HISTOGRAMMER_H */
