/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "DetectorHistogrammerModule.hpp"

#include <memory>
#include <string>
#include <utility>

#include <TApplication.h>
#include <TFile.h>

#include "core/geometry/PixelDetectorModel.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/utils/log.h"

using namespace allpix;

DetectorHistogrammerModule::DetectorHistogrammerModule(Configuration config,
                                                       Messenger* messenger,
                                                       std::shared_ptr<Detector> detector)
    : Module(detector), config_(std::move(config)), detector_(std::move(detector)), pixels_message_(nullptr) {
    // fetch deposit for single module
    messenger->bindSingle(this, &DetectorHistogrammerModule::pixels_message_);
}
DetectorHistogrammerModule::~DetectorHistogrammerModule() = default;

// create histograms
void DetectorHistogrammerModule::init() {
    // get detector model
    auto model = std::dynamic_pointer_cast<PixelDetectorModel>(detector_->getModel());
    if(model == nullptr) {
        // FIXME: exception can be more appropriate here
        LOG(ERROR) << "Detector " << detector_->getName()
                   << " is not a PixelDetectorModel: ignored as other types are currently unsupported!";
        return;
    }

    // create histogram
    LOG(INFO) << "Creating histograms";
    std::string histogram_name = "histogram_" + detector_->getName();
    std::string histogram_title = "Histogram for " + detector_->getName();
    histogram = new TH2I(histogram_name.c_str(),
                         histogram_title.c_str(),
                         model->getNPixelsX(),
                         0,
                         model->getNPixelsX(),
                         model->getNPixelsY(),
                         0,
                         model->getNPixelsY());

    // create cluster size plot
    std::string cluster_size_name = "cluster_" + detector_->getName();
    std::string cluster_size_title = "Cluster size for " + detector_->getName();
    cluster_size = new TH1I(cluster_size_name.c_str(),
                            cluster_size_title.c_str(),
                            model->getNPixelsX() * model->getNPixelsY(),
                            0,
                            model->getNPixelsX() * model->getNPixelsY());
}

// fill the histograms
void DetectorHistogrammerModule::run() {
    // check if we got any deposits
    if(pixels_message_ == nullptr) {
        LOG(WARNING) << "Detector " << detector_->getName() << " did not get any deposits... skipping!";
        return;
    }

    LOG(DEBUG) << "got charges in " << pixels_message_->getData().size() << " pixels";

    // fill 2d histogram
    for(auto& pixel_charge : pixels_message_->getData()) {
        auto pixel = pixel_charge.getPixel();
        auto charge = pixel_charge.getCharge();

        histogram->Fill(pixel.x(), pixel.y(), charge);
    }

    // fill cluster histogram
    cluster_size->Fill(static_cast<double>(pixels_message_->getData().size()));
}

// create file and write the histograms to it
void DetectorHistogrammerModule::finalize() {
    // create root file
    std::string file_name = config_.get<std::string>("file_prefix") + "_" + detector_->getName() + ".root";
    TFile file(file_name.c_str(), "RECREATE");

    // write histograms
    LOG(INFO) << "Writing histograms to file";
    histogram->Write();
    cluster_size->Write();

    // close the file
    file.Close();
}
