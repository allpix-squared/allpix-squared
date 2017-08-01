/**
 * @file
 * @brief Implementation of detector histogramming module
 * @copyright MIT License
 */

#include "DetectorHistogrammerModule.hpp"

#include <algorithm>
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
    std::string hit_map_name = "hit_map";
    std::string hit_map_title = "Hitmap for " + detector_->getName() + ";x (pixels);y (pixels)";
    hit_map = new TH2I(hit_map_name.c_str(),
                       hit_map_title.c_str(),
                       model->getNPixels().x(),
                       -0.5,
                       model->getNPixels().x() - 0.5,
                       model->getNPixels().y(),
                       -0.5,
                       model->getNPixels().y() - 0.5);

    // Create histogram of cluster map
    std::string cluster_map_name = "cluster_map";
    std::string cluster_map_title = "Cluster map for " + detector_->getName() + ";x (pixels);y (pixels)";
    cluster_map = new TH2I(cluster_map_name.c_str(),
                           cluster_map_title.c_str(),
                           model->getNPixels().x(),
                           -0.5,
                           model->getNPixels().x() - 0.5,
                           model->getNPixels().y(),
                           -0.5,
                           model->getNPixels().y() - 0.5);

    // Create cluster size plot
    std::string cluster_size_name = "cluster_size";
    std::string cluster_size_title = "Cluster size for " + detector_->getName() + ";size;number";
    cluster_size = new TH1I(cluster_size_name.c_str(),
                            cluster_size_title.c_str(),
                            model->getNPixels().x() * model->getNPixels().y(),
                            0.5,
                            model->getNPixels().x() * model->getNPixels().y() + 0.5);

    // Create event size plot
    std::string event_size_name = "event_size";
    std::string event_size_title = "Event size for " + detector_->getName() + ";size;number";
    event_size = new TH1I(event_size_name.c_str(),
                          event_size_title.c_str(),
                          model->getNPixels().x() * model->getNPixels().y(),
                          0.5,
                          model->getNPixels().x() * model->getNPixels().y() + 0.5);

    // Create number of clusters plot
    std::string n_cluster_name = "n_cluster";
    std::string n_cluster_title = "Number of clusters for " + detector_->getName() + ";size;number";
    n_cluster = new TH1I(n_cluster_name.c_str(),
                         n_cluster_title.c_str(),
                         model->getNPixels().x() * model->getNPixels().y(),
                         0.5,
                         model->getNPixels().x() * model->getNPixels().y() + 0.5);

    // Create cluster charge plot
    std::string cluster_charge_name = "cluster_charge";
    std::string cluster_charge_title = "Cluster charge for " + detector_->getName() + ";size;number";
    cluster_charge = new TH1D(cluster_charge_name.c_str(), cluster_charge_title.c_str(), 200, 0., 100.);
}

void DetectorHistogrammerModule::run(unsigned int) {
    LOG(DEBUG) << "Adding hits in " << pixels_message_->getData().size() << " pixels";

    // Fill 2D hitmap histogram
    for(auto& pixel_charge : pixels_message_->getData()) {
        auto pixel_idx = pixel_charge.getPixel().getIndex();

        // Add pixel
        hit_map->Fill(pixel_idx.x(), pixel_idx.y());

        // Update statistics
        total_vector_ += pixel_idx;
        total_hits_ += 1;
    }

    // Perform a clustering
    doClustering();

    // Evaluate the clusters
    for(auto clus : clusters_) {
        LOG(DEBUG) << "Cluster, size " << clus->getClusterSize() << " :";
        for(auto& pixel : clus->getPixelHits()) {
            LOG(DEBUG) << pixel->getPixel().getIndex();
        }
        // Fill cluster histograms
        cluster_size->Fill(static_cast<double>(clus->getClusterSize()));
        auto clusPos = clus->getClusterPosition();
        cluster_map->Fill(clusPos.x(), clusPos.y());
        cluster_charge->Fill(clus->getClusterCharge() * 1.e-3);
    }

    // Fill further histograms
    event_size->Fill(static_cast<double>(pixels_message_->getData().size()));
    n_cluster->Fill(static_cast<double>(clusters_.size()));
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

    // FIXME Set more useful spacing maximum for event size histogram
    xmax = std::ceil(event_size->GetBinCenter(event_size->FindLastBinAbove()) + 1);
    event_size->GetXaxis()->SetRangeUser(0, xmax);
    // Set event size axis spacing
    if(static_cast<int>(xmax) < 10) {
        event_size->GetXaxis()->SetNdivisions(static_cast<int>(xmax) + 1, 0, 0, true);
    }

    // FIXME Set more useful spacing maximum for n_cluster histogram
    xmax = std::ceil(n_cluster->GetBinCenter(n_cluster->FindLastBinAbove()) + 1);
    n_cluster->GetXaxis()->SetRangeUser(0, xmax);
    // Set cluster size axis spacing
    if(static_cast<int>(xmax) < 10) {
        n_cluster->GetXaxis()->SetNdivisions(static_cast<int>(xmax) + 1, 0, 0, true);
    }

    // FIXME Set more useful spacing maximum for cluster_charge histogram
    xmax = std::ceil(cluster_charge->GetBinCenter(cluster_charge->FindLastBinAbove()) + 1);
    cluster_charge->GetXaxis()->SetRangeUser(0, xmax);
    // Set cluster size axis spacing
    if(static_cast<int>(xmax) < 10) {
        cluster_charge->GetXaxis()->SetNdivisions(static_cast<int>(xmax) + 1, 0, 0, true);
    }

    // Set default drawing option histogram for hitmap
    hit_map->SetOption("colz");
    // Set hit_map axis spacing
    if(static_cast<int>(hit_map->GetXaxis()->GetXmax()) < 10) {
        hit_map->GetXaxis()->SetNdivisions(static_cast<int>(hit_map->GetXaxis()->GetXmax()) + 1, 0, 0, true);
    }
    if(static_cast<int>(hit_map->GetYaxis()->GetXmax()) < 10) {
        hit_map->GetYaxis()->SetNdivisions(static_cast<int>(hit_map->GetYaxis()->GetXmax()) + 1, 0, 0, true);
    }

    cluster_map->SetOption("colz");
    // Set cluster_map axis spacing
    if(static_cast<int>(cluster_map->GetXaxis()->GetXmax()) < 10) {
        cluster_map->GetXaxis()->SetNdivisions(static_cast<int>(cluster_map->GetXaxis()->GetXmax()) + 1, 0, 0, true);
    }
    if(static_cast<int>(cluster_map->GetYaxis()->GetXmax()) < 10) {
        cluster_map->GetYaxis()->SetNdivisions(static_cast<int>(cluster_map->GetYaxis()->GetXmax()) + 1, 0, 0, true);
    }

    // Write histograms
    LOG(TRACE) << "Writing histograms to file";
    hit_map->Write();
    cluster_map->Write();
    cluster_size->Write();
    event_size->Write();
    n_cluster->Write();
    cluster_charge->Write();
}

void DetectorHistogrammerModule::doClustering() {
    for(auto& pixel_hit : pixels_message_->getData()) {
        Cluster* clus;
        if(not checkAdjacentPixels(&pixel_hit)) {
            LOG(DEBUG) << "Creating new cluster: " << pixel_hit.getPixel().getIndex();
            clus = new Cluster(&pixel_hit);
            clusters_.push_back(clus);
        }
    }
}

bool DetectorHistogrammerModule::isInCluster(const PixelHit* pixel_hit) {
    for(auto clus : clusters_) {
        for(auto& hit : clus->getPixelHits()) {
            if(pixel_hit == hit) {
                return true;
            }
        }
    }
    return false;
}

bool DetectorHistogrammerModule::checkAdjacentPixels(const PixelHit* pixel_hit) {
    auto hit_idx = pixel_hit->getPixel().getIndex();
    for(auto clus : clusters_) {
        for(auto& pixel_other : clus->getPixelHits()) {
            auto other_idx = pixel_other->getPixel().getIndex();
            auto distx = std::max(hit_idx.x(), other_idx.x()) - std::min(hit_idx.x(), other_idx.x());
            auto disty = std::max(hit_idx.y(), other_idx.y()) - std::min(hit_idx.y(), other_idx.y());
            if(distx <= 1 and disty <= 1) {
                LOG(DEBUG) << "Adding pixel to cluster: " << hit_idx;
                clus->addPixelHit(pixel_hit);
                return true;
            }
        }
    }
    return false;
}
