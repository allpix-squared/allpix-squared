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
    class DetectorHistogrammerModule : public DetectorModule {
    public:
        /**
         * @brief Constructor for this detector-specific module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param detector Pointer to the detector for this module instance
         */
        DetectorHistogrammerModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector);

        /**
         * @brief Initialize the histograms
         */
        void initialize() override;

        /**
         * @brief Fill the histograms
         * @param event Pointer to the event the module is running
         */
        void run(Event* event) override;

        /**
         * @brief Write the histograms to the modules file
         */
        void finalize() override;

    private:
        /**
         * @brief Perform a sparse clustering on the PixelHits
         */
        std::vector<Cluster> do_clustering(std::shared_ptr<PixelHitMessage>& pixels_message) const;

        /**
         * @brief analyze the available MCParticles and return the all particles identified as primary (i.e. that do not have
         * a parent). This might be several particles.
         */
        static std::vector<const MCParticle*> get_primary_particles(std::shared_ptr<MCParticleMessage>& mcparticle_message);

        // Statistics
        std::atomic<unsigned long> total_hits_;

        // Cut criteria for efficiency measurement:
        ROOT::Math::XYVector matching_cut_;

        // Reference track resolution
        ROOT::Math::XYVector track_resolution_;

        // Plotting option
        bool output_maps_;

        // Histograms to output
        Histogram<TH2D> hit_map_, hit_map_global_, hit_map_global_mc_, hit_map_local_, hit_map_local_mc_, charge_map_,
            cluster_map_, polar_hit_map_;
        Histogram<TProfile2D> cluster_size_map_local_, cluster_size_map_, cluster_size_x_map_, cluster_size_y_map_;
        Histogram<TProfile2D> cluster_charge_map_, seed_charge_map_;
        Histogram<TProfile2D> residual_map_, residual_x_map_, residual_y_map_, residual_detector_, residual_x_detector_,
            residual_y_detector_;
        Histogram<TH1D> residual_x_, residual_y_, residual_r_, residual_phi_;
        Histogram<TProfile> residual_x_vs_x_, residual_y_vs_y_, residual_x_vs_y_, residual_y_vs_x_;
        Histogram<TProfile2D> efficiency_map_, efficiency_local_, efficiency_detector_;
        Histogram<TProfile> efficiency_vs_x_, efficiency_vs_y_;
        Histogram<TH1D> event_size_;
        Histogram<TH1D> cluster_size_, cluster_size_x_, cluster_size_y_;
        Histogram<TH1D> n_cluster_;
        Histogram<TH1D> cluster_charge_, pixel_charge_, total_charge_, cluster_seed_charge_;
    };
} // namespace allpix

#endif /* ALLPIX_MODULE_DETECTOR_HISTOGRAMMER_H */
