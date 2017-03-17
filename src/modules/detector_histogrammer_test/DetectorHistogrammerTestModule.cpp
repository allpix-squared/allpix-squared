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

DetectorHistogrammerModule::DetectorHistogrammerModule(AllPix* apx, Configuration config)
    : Module(apx), config_(std::move(config)), deposit_messages_() {
    getMessenger()->bindMulti(this, &DetectorHistogrammerModule::deposit_messages_);
}
DetectorHistogrammerModule::~DetectorHistogrammerModule() = default;

// run the deposition
void DetectorHistogrammerModule::run() {
    // get detector model
    auto model = std::dynamic_pointer_cast<PixelDetectorModel>(getDetector()->getModel());
    if(model == nullptr) {
        // FIXME: exception can be more appropriate here
        LOG(CRITICAL) << "Detector '" << getDetector()->getName()
                      << "' is not a PixelDetectorModel: ignored as other types are currently unsupported!";
        return;
    }

    // create root file
    std::string file_name = config_.get<std::string>("file_prefix") + "_" + getDetector()->getName() + ".root";
    auto file = new TFile(file_name.c_str(), "RECREATE");

    // create histogram
    std::string plot_name = "plot_" + getDetector()->getName();
    std::string plot_title = "Histogram for " + getDetector()->getName();
    auto histogram = new TH2F(plot_name.c_str(),
                              plot_title.c_str(),
                              model->GetNPixelsX(),
                              -model->GetHalfSensorX(),
                              model->GetHalfSensorX(),
                              model->GetNPixelsY(),
                              -model->GetHalfSensorY(),
                              model->GetHalfSensorY());

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
