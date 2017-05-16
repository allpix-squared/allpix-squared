/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "DetectorHistogrammerModule.hpp"

#include <memory>
#include <string>
#include <utility>

#include <TFile.h>

#include "core/geometry/PixelDetectorModel.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/utils/log.h"

using namespace allpix;

DetectorHistogrammerModule::DetectorHistogrammerModule(Configuration config,
                                                       Messenger* messenger,
                                                       std::shared_ptr<Detector> detector)
    : Module(detector), config_(std::move(config)), detector_(std::move(detector)), pixels_message_(nullptr),
      output_file_(nullptr) {
    // fetch deposit for single module
    messenger->bindSingle(this, &DetectorHistogrammerModule::pixels_message_);
}

// create histograms
void DetectorHistogrammerModule::init() {
    // get detector model
    auto model = std::dynamic_pointer_cast<PixelDetectorModel>(detector_->getModel());
    if(model == nullptr) {
        throw ModuleError("Detector model of " + detector_->getName() +
                          " is not a PixelDetectorModel: other models are not supported by this module!");
    }

    // create root file
    std::string file_name = getOutputPath(config_.get<std::string>("file_name", "histogram") + ".root");
    output_file_ = std::make_unique<TFile>(file_name.c_str(), "RECREATE");
    output_file_->cd();

    // create histogram
    LOG(INFO) << "Creating histograms";
    std::string histogram_name = "histogram_" + getUniqueName();
    std::string histogram_title = "Histogram for " + detector_->getName();
    histogram = new TH2I(histogram_name.c_str(),
                         histogram_title.c_str(),
                         model->getNPixelsX(),
                         -0.5,
                         model->getNPixelsX() - 0.5,
                         model->getNPixelsY(),
                         -0.5,
                         model->getNPixelsY() - 0.5);

    // create cluster size plot
    std::string cluster_size_name = "cluster_" + detector_->getName();
    std::string cluster_size_title = "Cluster size for " + detector_->getName();
    cluster_size = new TH1I(cluster_size_name.c_str(),
                            cluster_size_title.c_str(),
                            model->getNPixelsX() * model->getNPixelsY(),
                            0.5,
                            model->getNPixelsX() * model->getNPixelsY() + 0.5);
}

// fill the histograms
void DetectorHistogrammerModule::run(unsigned int) {
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
    // go to output file
    output_file_->cd();

    // set more useful spacing maximum for cluster size histogram
    auto xmax = std::ceil(cluster_size->GetBinCenter(cluster_size->FindLastBinAbove()) + 1);
    cluster_size->GetXaxis()->SetRangeUser(0, xmax);
    // set cluster size axis spacing (FIXME: worth to do?)
    if(static_cast<int>(xmax) < 10) {
        cluster_size->GetXaxis()->SetNdivisions(static_cast<int>(xmax) + 1, 0, 0, true);
    }

    // set default drawing option histogram
    histogram->SetOption("colz");
    // set histogram axis spacing (FIXME: worth to do?)
    if(static_cast<int>(histogram->GetXaxis()->GetXmax()) < 10) {
        histogram->GetXaxis()->SetNdivisions(static_cast<int>(histogram->GetXaxis()->GetXmax()) + 1, 0, 0, true);
    }
    if(static_cast<int>(histogram->GetYaxis()->GetXmax()) < 10) {
        histogram->GetYaxis()->SetNdivisions(static_cast<int>(histogram->GetYaxis()->GetXmax()) + 1, 0, 0, true);
    }

    // write histograms
    LOG(INFO) << "Writing histograms to file";
    histogram->Write();
    cluster_size->Write();

    // close the file
    output_file_->Close();
}
