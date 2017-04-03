/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "DetectorHistogrammerTestModule.hpp"

#include <memory>
#include <string>
#include <utility>

#include <TApplication.h>
#include <TFile.h>
#include <TH2D.h>

#include "core/geometry/PixelDetectorModel.hpp"

#include "core/messenger/Messenger.hpp"
#include "core/utils/log.h"

using namespace allpix;

const std::string DetectorHistogrammerModule::name = "detector_histogrammer_test";

DetectorHistogrammerModule::DetectorHistogrammerModule(Configuration config,
                                                       Messenger* messenger,
                                                       std::shared_ptr<Detector> detector)
    : Module(detector), config_(std::move(config)), detector_(std::move(detector)), deposits_message_(nullptr) {
    // fetch deposit for single module
    messenger->bindSingle(this, &DetectorHistogrammerModule::deposits_message_);
}
DetectorHistogrammerModule::~DetectorHistogrammerModule() = default;

// histogram the deposits
void DetectorHistogrammerModule::run() {
    // check if we got any deposits
    if(deposits_message_ == nullptr) {
        LOG(WARNING) << "Detector " << detector_->getName() << " did not get any deposits... skipping!";
        return;
    }

    // get detector model
    auto model = std::dynamic_pointer_cast<PixelDetectorModel>(detector_->getModel());
    if(model == nullptr) {
        // FIXME: exception can be more appropriate here
        LOG(CRITICAL) << "Detector " << detector_->getName()
                      << " is not a PixelDetectorModel: ignored as other types are currently unsupported!";
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

    for(auto& deposit : deposits_message_->getData()) {
        auto vec = deposit.getPosition();
        auto charge = deposit.getCharge();

        histogram->Fill(vec.x(), vec.y(), charge);
    }

    histogram->Write();

    file->Close();
}
