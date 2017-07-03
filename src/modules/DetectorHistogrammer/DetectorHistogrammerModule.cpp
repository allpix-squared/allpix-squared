/**
 * @file
 * @brief Implementation of detector histogramming module
 * @copyright MIT License
 */

#include "DetectorHistogrammerModule.hpp"

#include <memory>
#include <string>
#include <utility>

#include "core/geometry/HybridPixelDetectorModel.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/utils/log.h"

#include "tools/ROOT.h"

using namespace allpix;

DetectorHistogrammerModule::DetectorHistogrammerModule(Configuration config,
                                                       Messenger* messenger,
                                                       std::shared_ptr<Detector> detector)
    : Module(config, detector), config_(std::move(config)), detector_(std::move(detector)), pixels_message_(nullptr) {
    // Bind pixel hits message
    messenger->bindSingle(this, &DetectorHistogrammerModule::pixels_message_, MsgFlags::REQUIRED);
}

void DetectorHistogrammerModule::init() {
    // Fetch detector model
    auto model = detector_->getModel();

    // Create histogram of hitmap
    LOG(TRACE) << "Creating histograms";
    std::string histogram_name = "histogram";
    std::string histogram_title = "Hitmap for " + detector_->getName() + ";x (pixels);y (pixels)";
    histogram = new TH2I(histogram_name.c_str(),
                         histogram_title.c_str(),
                         model->getNPixels().x(),
                         -0.5,
                         model->getNPixels().x() - 0.5,
                         model->getNPixels().y(),
                         -0.5,
                         model->getNPixels().y() - 0.5);

    // Create cluster size plot
    std::string cluster_size_name = "cluster";
    std::string cluster_size_title = "Cluster size for " + detector_->getName() + ";size;number";
    cluster_size = new TH1I(cluster_size_name.c_str(),
                            cluster_size_title.c_str(),
                            model->getNPixels().x() * model->getNPixels().y(),
                            0.5,
                            model->getNPixels().x() * model->getNPixels().y() + 0.5);
}

void DetectorHistogrammerModule::run(unsigned int) {
    LOG(DEBUG) << "Adding hits in " << pixels_message_->getData().size() << " pixels";

    // Fill 2D hitmap histogram
    for(auto& pixel_charge : pixels_message_->getData()) {
        auto pixel = pixel_charge.getPixel();

        // Add pixel
        histogram->Fill(pixel.x(), pixel.y());

        // Update statistics
        total_vector_ += pixel;
        total_hits_ += 1;
    }

    // Fill cluster histogram
    cluster_size->Fill(static_cast<double>(pixels_message_->getData().size()));
}

void DetectorHistogrammerModule::finalize() {
    // Print statistics
    if(total_hits_ != 0) {
        LOG(INFO) << "Plotted " << total_hits_ << " hits in total, mean position is "
                  << total_vector_ / static_cast<double>(total_hits_);
    } else {
        LOG(WARNING) << "No hits plotted";
    }

    // FIXME Set more useful spacing maximum for cluster size histogram
    auto xmax = std::ceil(cluster_size->GetBinCenter(cluster_size->FindLastBinAbove()) + 1);
    cluster_size->GetXaxis()->SetRangeUser(0, xmax);
    // Set cluster size axis spacing
    if(static_cast<int>(xmax) < 10) {
        cluster_size->GetXaxis()->SetNdivisions(static_cast<int>(xmax) + 1, 0, 0, true);
    }

    // Set default drawing option histogram for hitmap
    histogram->SetOption("colz");
    // Set histogram axis spacing
    if(static_cast<int>(histogram->GetXaxis()->GetXmax()) < 10) {
        histogram->GetXaxis()->SetNdivisions(static_cast<int>(histogram->GetXaxis()->GetXmax()) + 1, 0, 0, true);
    }
    if(static_cast<int>(histogram->GetYaxis()->GetXmax()) < 10) {
        histogram->GetYaxis()->SetNdivisions(static_cast<int>(histogram->GetYaxis()->GetXmax()) + 1, 0, 0, true);
    }

    // Write histograms
    LOG(TRACE) << "Writing histograms to file";
    histogram->Write();
    cluster_size->Write();
}
