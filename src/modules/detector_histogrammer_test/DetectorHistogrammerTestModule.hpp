/*
 * Histogram hits in a very simplified way
 */

#ifndef ALLPIX_MODULE_DETECTOR_HISTOGRAMMER_H
#define ALLPIX_MODULE_DETECTOR_HISTOGRAMMER_H

#include <memory>
#include <string>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

#include "objects/ChargeDeposit.hpp"

namespace allpix {
    class DepositionMessage;

    // define the module to inherit from the module base class
    class DetectorHistogrammerModule : public Module {
    public:
        // name of the module
        static const std::string name;

        // constructor and destructor
        DetectorHistogrammerModule(Configuration, Messenger*, std::shared_ptr<Detector>);
        ~DetectorHistogrammerModule() override;

        // create and plot the histograms
        void run() override;

    private:
        // configuration for this module
        Configuration config_;

        // attached detector
        std::shared_ptr<Detector> detector_;

        // list of the messages
        std::shared_ptr<ChargeDepositMessage> deposit_message_;
    };
}

#endif /* ALLPIX_MODULE_DETECTOR_HISTOGRAMMER_H */
