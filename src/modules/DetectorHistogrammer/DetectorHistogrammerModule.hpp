/**
 * @file
 * @brief Definition of detector histogramming module
 * @copyright MIT License
 */

#ifndef ALLPIX_MODULE_DETECTOR_HISTOGRAMMER_H
#define ALLPIX_MODULE_DETECTOR_HISTOGRAMMER_H

#include <memory>

#include <string>

#include <TH1I.h>
#include <TH2I.h>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

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
        DetectorHistogrammerModule(Configuration, Messenger*, std::shared_ptr<Detector>);

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

<<<<<<< HEAD
=======
        /**
         * @brief Perform a sparse clustering on the PixelHits
         */
        void doClustering();

        /**
         * @brief Checks for clusters containing this PixelHit
         * @param PixelHit to check
         */
        bool isInCluster(const PixelHit*);

        /**
         * @brief Checks the adjacent pixels for PixelHits
         * @param PixelHit to check
         */
        bool checkAdjacentPixels(const PixelHit*);

>>>>>>> 4c311d6... Fix formatting issues

    private:
        Configuration config_;
        std::shared_ptr<Detector> detector_;

        // List of pixel hits
        std::shared_ptr<PixelHitMessage> pixels_message_;

        // Statistics to compute mean position
        ROOT::Math::XYVector total_vector_{};
        unsigned long total_hits_{};

        // Histograms to output
        TH2I* histogram; // FIXME: bad name
        TH1I* cluster_size;
    };
} // namespace allpix

#endif /* ALLPIX_MODULE_DETECTOR_HISTOGRAMMER_H */
