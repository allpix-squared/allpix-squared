/**
 * @file
 * @brief Definition of  of pulse transfer module
 * @copyright Copyright (c) 2019 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <string>

#include "core/config/Configuration.hpp"
#include "core/geometry/DetectorModel.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

#include "objects/PropagatedCharge.hpp"

#include <TH1D.h>

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to combine all charges induced at every pixel.
     *
     * This modules takes pulses created by the TransientPropagation module and compines the individual induced charges from
     * each of the PropagatedCharges at every pixel into PixelCharges.
     */
    class PulseTransferModule : public Module {
    public:
        /**
         * @brief Constructor for the PulseTransfer module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param detector Pointer to the detector for this module instance
         */
        PulseTransferModule(Configuration& config, Messenger* messenger, const std::shared_ptr<Detector>& detector);

        /**
         * @brief Initialize optional ROOT histograms
         */
        void init(std::mt19937_64&) override;

        /**
         * @brief Combine pulses from propagated charges and transfer them to the pixels
         */
        void run(Event*) override;

        /**
         * @brief Finalize and write optional histograms
         */
        void finalize() override;

    private:
        bool output_plots_{}, output_pulsegraphs_{};

        // General module members
        std::shared_ptr<Detector> detector_;

        /**
         * @brief Compare two pixels, necessary to store them in the a std::map
         */
        struct pixel_cmp {
            bool operator()(const Pixel::Index& p1, const Pixel::Index& p2) const {
                if(p1.x() == p2.x()) {
                    return p1.y() < p2.y();
                }
                return p1.x() < p2.x();
            }
        };

        // Output histograms
        TH1D *h_total_induced_charge_{}, *h_induced_pixel_charge_{};
    };
} // namespace allpix
