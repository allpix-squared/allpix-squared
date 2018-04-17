/**
 * @file
 * @brief Implementation of detector histogramming module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
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

DetectorHistogrammerModule::DetectorHistogrammerModule(Configuration& config,
                                                       Messenger* messenger,
                                                       std::shared_ptr<Detector> detector)
    : Module(config, detector), detector_(std::move(detector)), pixels_message_(nullptr) {
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
    hit_map = new TH2D(hit_map_name.c_str(),
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
    cluster_map = new TH2D(cluster_map_name.c_str(),
                           cluster_map_title.c_str(),
                           model->getNPixels().x(),
                           -0.5,
                           model->getNPixels().x() - 0.5,
                           model->getNPixels().y(),
                           -0.5,
                           model->getNPixels().y() - 0.5);

    // Create cluster size plots
    std::string cluster_size_name = "cluster_size";
    std::string cluster_size_title = "Cluster size for " + detector_->getName() + ";cluster size [px];clusters";
    cluster_size = new TH1D(cluster_size_name.c_str(),
                            cluster_size_title.c_str(),
                            model->getNPixels().x() * model->getNPixels().y() / 10,
                            0.5,
                            model->getNPixels().x() * model->getNPixels().y() / 10 + 0.5);

    std::string cluster_size_x_name = "cluster_size_x";
    std::string cluster_size_x_title = "Cluster size X for " + detector_->getName() + ";cluster size x [px];clusters";
    cluster_size_x = new TH1D(cluster_size_x_name.c_str(),
                              cluster_size_x_title.c_str(),
                              model->getNPixels().x(),
                              0.5,
                              model->getNPixels().x() + 0.5);

    std::string cluster_size_y_name = "cluster_size_y";
    std::string cluster_size_y_title = "Cluster size Y for " + detector_->getName() + ";cluster size y [px];clusters";
    cluster_size_y = new TH1D(cluster_size_y_name.c_str(),
                              cluster_size_y_title.c_str(),
                              model->getNPixels().y(),
                              0.5,
                              model->getNPixels().y() + 0.5);

    // Create event size plot
    std::string event_size_name = "event_size";
    std::string event_size_title = "Event size for " + detector_->getName() + ";event size [px];events";
    event_size = new TH1D(event_size_name.c_str(),
                          event_size_title.c_str(),
                          model->getNPixels().x() * model->getNPixels().y(),
                          0.5,
                          model->getNPixels().x() * model->getNPixels().y() + 0.5);

    // Create number of clusters plot
    std::string n_cluster_name = "n_cluster";
    std::string n_cluster_title = "Number of clusters for " + detector_->getName() + ";clusters;events";
    n_cluster = new TH1D(n_cluster_name.c_str(),
                         n_cluster_title.c_str(),
                         model->getNPixels().x() * model->getNPixels().y(),
                         0.5,
                         model->getNPixels().x() * model->getNPixels().y() + 0.5);

    // Create cluster charge plot
    std::string cluster_charge_name = "cluster_charge";
    std::string cluster_charge_title = "Cluster charge for " + detector_->getName() + ";cluster charge [ke];clusters";
    cluster_charge = new TH1D(cluster_charge_name.c_str(), cluster_charge_title.c_str(), 1000, 0., 50.);
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
    std::vector<Cluster> clusters = doClustering();

    // Evaluate the clusters
    for(auto clus : clusters) {
        // Fill cluster histograms
        cluster_size->Fill(static_cast<double>(clus.getClusterSize()));
        auto clusSizesXY = clus.getClusterSizeXY();
        cluster_size_x->Fill(clusSizesXY.first);
        cluster_size_y->Fill(clusSizesXY.second);

        auto clusPos = clus.getClusterPosition();
        cluster_map->Fill(clusPos.x(), clusPos.y());
        cluster_charge->Fill(clus.getClusterCharge() * 1.e-3);
    }

    // Fill further histograms
    event_size->Fill(static_cast<double>(pixels_message_->getData().size()));
    n_cluster->Fill(static_cast<double>(clusters.size()));
}

void DetectorHistogrammerModule::finalize() {
    // Print statistics
    if(total_hits_ != 0) {
        LOG(INFO) << "Plotted " << total_hits_ << " hits in total, mean position is "
                  << total_vector_ / static_cast<double>(total_hits_);
    }

    // FIXME Set more useful spacing maximum for cluster size histogram
    auto xmax = std::ceil(cluster_size->GetBinCenter(cluster_size->FindLastBinAbove()) + 1);
    cluster_size->GetXaxis()->SetRangeUser(0, xmax);
    // Set cluster size axis spacing
    if(static_cast<int>(xmax) < 10) {
        cluster_size->GetXaxis()->SetNdivisions(static_cast<int>(xmax) + 1, 0, 0, true);
    }

    xmax = std::ceil(cluster_size_x->GetBinCenter(cluster_size_x->FindLastBinAbove()) + 1);
    cluster_size_x->GetXaxis()->SetRangeUser(0, xmax);
    // Set cluster size_x axis spacing
    if(static_cast<int>(xmax) < 10) {
        cluster_size_x->GetXaxis()->SetNdivisions(static_cast<int>(xmax) + 1, 0, 0, true);
    }

    xmax = std::ceil(cluster_size_y->GetBinCenter(cluster_size_y->FindLastBinAbove()) + 1);
    cluster_size_y->GetXaxis()->SetRangeUser(0, xmax);
    // Set cluster size_y axis spacing
    if(static_cast<int>(xmax) < 10) {
        cluster_size_y->GetXaxis()->SetNdivisions(static_cast<int>(xmax) + 1, 0, 0, true);
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
    cluster_size_x->Write();
    cluster_size_y->Write();
    event_size->Write();
    n_cluster->Write();
    cluster_charge->Write();
}

std::vector<Cluster> DetectorHistogrammerModule::doClustering() {
    std::vector<Cluster> clusters;
    std::map<const PixelHit*, bool> usedPixel;

    auto pixel_it = pixels_message_->getData().begin();
    for(; pixel_it != pixels_message_->getData().end(); pixel_it++) {
        const PixelHit* pixel_hit = &(*pixel_it);

        // Check if the pixel has been used:
        if(usedPixel[pixel_hit]) {
            continue;
        }

        // Create new cluster
        Cluster cluster(pixel_hit);
        usedPixel[pixel_hit] = true;
        LOG(DEBUG) << "Creating new cluster with seed: " << pixel_hit->getPixel().getIndex();

        auto touching = [&](const PixelHit* pixel) {
            auto pxi1 = pixel->getIndex();
            for(auto& cluster_pixel : cluster.getPixelHits()) {

                auto distance = [](unsigned int lhs, unsigned int rhs) { return (lhs > rhs ? lhs - rhs : rhs - lhs); };

                auto pxi2 = cluster_pixel->getIndex();
                if(distance(pxi1.x(), pxi2.x()) <= 1 && distance(pxi1.y(), pxi2.y()) <= 1) {
                    return true;
                }
            }
            return false;
        };

        // Keep adding pixels to the cluster:
        for(auto other_pixel = pixel_it + 1; other_pixel != pixels_message_->getData().end(); other_pixel++) {
            const PixelHit* neighbor = &(*other_pixel);

            // Check if neighbor has been used or if it touches the current cluster:
            if(usedPixel[neighbor] || !touching(neighbor)) {
                continue;
            }

            cluster.addPixelHit(neighbor);
            LOG(DEBUG) << "Adding pixel: " << neighbor->getPixel().getIndex();
            usedPixel[neighbor] = true;
        }
        clusters.push_back(cluster);
    }
    return clusters;
}
