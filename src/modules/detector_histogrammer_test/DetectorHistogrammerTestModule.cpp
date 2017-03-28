/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "DetectorHistogrammerTestModule.hpp"

#include "core/geometry/PixelDetectorModel.hpp"

#include "core/messenger/Messenger.hpp"
#include "core/utils/log.h"

#include "messages/DepositionMessage.hpp"

#include <TApplication.h>
#include <TFile.h>
#include <TH2D.h>

using namespace allpix;

const std::string DetectorHistogrammerModule::name = "detector_histogrammer_test";

DetectorHistogrammerModule::DetectorHistogrammerModule(Configuration config,
                                                       Messenger* messenger,
                                                       std::shared_ptr<Detector> detector)
    : Module(detector), config_(std::move(config)), detector_(detector), deposit_messages_() {
    messenger->bindMulti(this, &DetectorHistogrammerModule::deposit_messages_);
}
DetectorHistogrammerModule::~DetectorHistogrammerModule() = default;

// run the deposition
void DetectorHistogrammerModule::run() {
    // get detector model
    auto model = std::dynamic_pointer_cast<PixelDetectorModel>(detector_->getModel());
    if(model == nullptr) {
        // FIXME: exception can be more appropriate here
        LOG(CRITICAL) << "Detector '" << detector_->getName()
                      << "' is not a PixelDetectorModel: ignored as other types are currently unsupported!";
        return;
    }

    // create root file
    std::string file_name = config_.get<std::string>("file_prefix") + "_" + detector_->getName() + ".root";
    auto file = new TFile(file_name.c_str(), "RECREATE");

    // create histogram
    std::string plot_name = "plot_" + detector_->getName();
    std::string plot_title = "Histogram for " + detector_->getName();
    auto histogram = new TH2F(plot_name.c_str(),
                              plot_title.c_str(),
                              model->getNPixelsX(),
                              -model->getHalfSensorSizeX(),
                              model->getHalfSensorSizeX(),
                              model->getNPixelsY(),
                              -model->getHalfSensorSizeY(),
                              model->getHalfSensorSizeY());

    // FIXME: bind single when this works
    for(auto& message : deposit_messages_) {
        for(auto& deposit : message->getDeposits()) {
            auto vec = deposit.getPosition();
            double energy = deposit.getEnergy();

            histogram->Fill(vec.x(), vec.y(), energy);
        }
    }

    histogram->Write();

    file->Close();
}
