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
    // Bind messages
    messenger->bindSingle(this, &DetectorHistogrammerModule::pixels_message_, MsgFlags::REQUIRED);
    messenger->bindSingle(this, &DetectorHistogrammerModule::mcparticle_message_, MsgFlags::REQUIRED);
}

void DetectorHistogrammerModule::init() {
    using namespace ROOT::Math;

    // Fetch detector model
    auto model = detector_->getModel();
    auto pitch_x = static_cast<double>(Units::convert(model->getPixelSize().x(), "um"));
    auto pitch_y = static_cast<double>(Units::convert(model->getPixelSize().y(), "um"));

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

    // Calculate the granularity of in-pixel maps:
    auto inpixel_bins = config_.get<DisplacementVector2D<Cartesian2D<int>>>(
        "granularity", DisplacementVector2D<Cartesian2D<int>>(static_cast<int>(pitch_x), static_cast<int>(pitch_y)));
    if(inpixel_bins.x() * inpixel_bins.y() > 250000) {
        LOG(WARNING) << "Selected plotting granularity of " << inpixel_bins << " bins creates very large histograms."
                     << std::endl
                     << "Consider reducing the number of bins using the granularity parameter.";
    } else {
        LOG(DEBUG) << "In-pixel plot granularity: " << inpixel_bins;
    }

    // Create histogram of cluster map
    std::string cluster_size_map_name = "cluster_size_map";
    std::string cluster_size_map_title = "Cluster size as function of in-pixel impact position for " + detector_->getName() +
                                         ";x%pitch [#mum];y%pitch [#mum]";
    cluster_size_map = new TProfile2D(cluster_size_map_name.c_str(),
                                      cluster_size_map_title.c_str(),
                                      inpixel_bins.x(),
                                      0.,
                                      pitch_x,
                                      inpixel_bins.y(),
                                      0.,
                                      pitch_y);

    std::string cluster_size_x_map_name = "cluster_size_x_map";
    std::string cluster_size_x_map_title = "Cluster size in X as function of in-pixel impact position for " +
                                           detector_->getName() + ";x%pitch [#mum];y%pitch [#mum]";
    cluster_size_x_map = new TProfile2D(cluster_size_x_map_name.c_str(),
                                        cluster_size_x_map_title.c_str(),
                                        inpixel_bins.x(),
                                        0.,
                                        pitch_x,
                                        inpixel_bins.y(),
                                        0.,
                                        pitch_y);

    std::string cluster_size_y_map_name = "cluster_size_y_map";
    std::string cluster_size_y_map_title = "Cluster size in Y as function of in-pixel impact position for " +
                                           detector_->getName() + ";x%pitch [#mum];y%pitch [#mum]";
    cluster_size_y_map = new TProfile2D(cluster_size_y_map_name.c_str(),
                                        cluster_size_y_map_title.c_str(),
                                        inpixel_bins.x(),
                                        0.,
                                        pitch_x,
                                        inpixel_bins.y(),
                                        0.,
                                        pitch_y);

    // Charge maps:
    std::string cluster_charge_map_name = "cluster_charge_map";
    std::string cluster_charge_map_title = "Cluster charge as function of in-pixel impact position for " +
                                           detector_->getName() + ";x%pitch [#mum];y%pitch [#mum];<cluster charge> [ke]";
    cluster_charge_map = new TProfile2D(cluster_charge_map_name.c_str(),
                                        cluster_charge_map_title.c_str(),
                                        inpixel_bins.x(),
                                        0.,
                                        pitch_x,
                                        inpixel_bins.y(),
                                        0.,
                                        pitch_y);
    std::string seed_charge_map_name = "seed_charge_map";
    std::string seed_charge_map_title = "Seed pixel charge as function of in-pixel impact position for " +
                                        detector_->getName() + ";x%pitch [#mum];y%pitch [#mum];<seed pixel charge> [ke]";
    seed_charge_map = new TProfile2D(seed_charge_map_name.c_str(),
                                     seed_charge_map_title.c_str(),
                                     inpixel_bins.x(),
                                     0.,
                                     pitch_x,
                                     inpixel_bins.y(),
                                     0.,
                                     pitch_y);

    // Create cluster size plots, preventing a zero-bin histogram by scaling with integer ceiling: (x + y - 1) / y
    std::string cluster_size_name = "cluster_size";
    std::string cluster_size_title = "Cluster size for " + detector_->getName() + ";cluster size [px];clusters";
    cluster_size = new TH1D(cluster_size_name.c_str(),
                            cluster_size_title.c_str(),
                            (model->getNPixels().x() * model->getNPixels().y() + 9) / 10,
                            0.5,
                            (model->getNPixels().x() * model->getNPixels().y() + 9) / 10 + 0.5);

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

    // Create residual plots
    std::string residual_x_name = "residual_x";
    std::string residual_x_title = "Residual in X for " + detector_->getName() + ";x_{track} - x_{cluster} [#mum];events";
    residual_x = new TH1D(
        residual_x_name.c_str(), residual_x_title.c_str(), static_cast<int>(12 * pitch_x), -2 * pitch_x, 2 * pitch_x);
    std::string residual_y_name = "residual_y";
    std::string residual_y_title = "Residual in Y for " + detector_->getName() + ";y_{track} - y_{cluster} [#mum];events";
    residual_y = new TH1D(
        residual_y_name.c_str(), residual_y_title.c_str(), static_cast<int>(12 * pitch_y), -2 * pitch_y, 2 * pitch_y);

    // Residual maps
    std::string residual_map_name = "residual_map";
    std::string residual_map_title = "Mean absolute deviation of residual as function of in-pixel impact position for " +
                                     detector_->getName() +
                                     ";x%pitch [#mum];y%pitch [#mum];MAD(#sqrt{#Deltax^{2}+#Deltay^{2}}) [#mum]";
    residual_map = new TProfile2D(residual_map_name.c_str(),
                                  residual_map_title.c_str(),
                                  static_cast<int>(pitch_x),
                                  0.,
                                  pitch_x,
                                  static_cast<int>(pitch_y),
                                  0.,
                                  pitch_y);
    std::string residual_x_map_name = "residual_x_map";
    std::string residual_x_map_title =
        "Mean absolute deviation of residual in X as function of in-pixel impact position for " + detector_->getName() +
        ";x%pitch [#mum];y%pitch [#mum];MAD(#Deltax) [#mum]";
    residual_x_map = new TProfile2D(residual_x_map_name.c_str(),
                                    residual_x_map_title.c_str(),
                                    static_cast<int>(pitch_x),
                                    0.,
                                    pitch_x,
                                    static_cast<int>(pitch_y),
                                    0.,
                                    pitch_y);
    std::string residual_y_map_name = "residual_y_map";
    std::string residual_y_map_title =
        "Mean absolute deviation of residual in Y as function of in-pixel impact position for " + detector_->getName() +
        ";x%pitch [#mum];y%pitch [#mum];MAD(#Deltay) [#mum]";
    residual_y_map = new TProfile2D(residual_y_map_name.c_str(),
                                    residual_y_map_title.c_str(),
                                    static_cast<int>(pitch_x),
                                    0.,
                                    pitch_x,
                                    static_cast<int>(pitch_y),
                                    0.,
                                    pitch_y);

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
    using namespace ROOT::Math;
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

    // Retrieve all MC particles in this detector which are primary particles (not produced within the sensor):
    auto primary_particles = getPrimaryParticles();
    LOG(DEBUG) << "Found " << primary_particles.size() << " primary particles in this event";

    // Evaluate the clusters
    for(auto clus : clusters) {
        // Fill cluster histograms
        cluster_size->Fill(static_cast<double>(clus.getClusterSize()));
        auto clusSizesXY = clus.getClusterSizeXY();
        cluster_size_x->Fill(clusSizesXY.first);
        cluster_size_y->Fill(clusSizesXY.second);

        auto clusterPos = clus.getClusterPosition();
        LOG(TRACE) << "Cluster at coordinates " << clusterPos;
        cluster_map->Fill(clusterPos.x(), clusterPos.y());
        cluster_charge->Fill(static_cast<double>(Units::convert(clus.getClusterCharge(), "ke")));

        auto cluster_particles = clus.getMCParticles();
        LOG(DEBUG) << "This cluster is connected to " << cluster_particles.size() << " MC particles";

        // Find all particles connected to this cluster which are also primaries:
        std::vector<const MCParticle*> intersection;
        std::set_intersection(primary_particles.begin(),
                              primary_particles.end(),
                              cluster_particles.begin(),
                              cluster_particles.end(),
                              std::back_inserter(intersection));

        LOG(TRACE) << "Matching primaries: " << intersection.size();
        for(auto& particle : intersection) {
            auto pitch = detector_->getModel()->getPixelSize();

            auto particlePos = (static_cast<XYZVector>(particle->getLocalStartPoint()) + particle->getLocalEndPoint()) / 2.0;
            LOG(DEBUG) << "MCParticle at " << Units::display(particlePos, {"mm", "um"});

            auto inPixelPos = XYVector(std::fmod(particlePos.x() + pitch.x() / 2, pitch.x()),
                                       std::fmod(particlePos.y() + pitch.y() / 2, pitch.y()));
            LOG(TRACE) << "MCParticle in pixel at " << Units::display(inPixelPos, {"mm", "um"});

            auto inPixel_um_x = static_cast<double>(Units::convert(inPixelPos.x(), "um"));
            auto inPixel_um_y = static_cast<double>(Units::convert(inPixelPos.y(), "um"));
            cluster_size_map->Fill(inPixel_um_x, inPixel_um_y, static_cast<double>(clus.getClusterSize()));
            cluster_size_x_map->Fill(inPixel_um_x, inPixel_um_y, clusSizesXY.first);
            cluster_size_y_map->Fill(inPixel_um_x, inPixel_um_y, clusSizesXY.second);

            // Charge maps:
            cluster_charge_map->Fill(
                inPixel_um_x, inPixel_um_y, static_cast<double>(Units::convert(clus.getClusterCharge(), "ke")));

            // Find the nearest pixel
            auto xpixel = static_cast<unsigned int>(std::round(particlePos.x() / pitch.x()));
            auto ypixel = static_cast<unsigned int>(std::round(particlePos.y() / pitch.y()));

            // Retrieve the pixel to which this MCParticle points:
            auto pixel = clus.getPixelHit(xpixel, ypixel);
            if(pixel != nullptr) {
                seed_charge_map->Fill(
                    inPixel_um_x, inPixel_um_y, static_cast<double>(Units::convert(pixel->getSignal(), "ke")));
            }

            // Calculate residual with cluster position:
            auto residual_um_x = static_cast<double>(Units::convert(particlePos.x() - clusterPos.x() * pitch.x(), "um"));
            auto residual_um_y = static_cast<double>(Units::convert(particlePos.y() - clusterPos.y() * pitch.y(), "um"));
            residual_x->Fill(residual_um_x);
            residual_y->Fill(residual_um_y);
            residual_map->Fill(inPixel_um_x,
                               inPixel_um_y,
                               std::fabs(std::sqrt(residual_um_x * residual_um_x + residual_um_y * residual_um_y)));
            residual_x_map->Fill(inPixel_um_x, inPixel_um_y, std::fabs(residual_um_x));
            residual_y_map->Fill(inPixel_um_x, inPixel_um_y, std::fabs(residual_um_y));
        }
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
    cluster_size_map->Write();
    cluster_size_x_map->Write();
    cluster_size_y_map->Write();
    cluster_size->Write();
    cluster_size_x->Write();
    cluster_size_y->Write();
    event_size->Write();
    residual_x->Write();
    residual_y->Write();
    residual_map->Write();
    residual_x_map->Write();
    residual_y_map->Write();
    n_cluster->Write();
    cluster_charge->Write();
    cluster_charge_map->Write();
    seed_charge_map->Write();
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

std::vector<const MCParticle*> DetectorHistogrammerModule::getPrimaryParticles() const {
    std::vector<const MCParticle*> primaries;

    // Loop over all MCParticles available
    for(auto& mc_particle : mcparticle_message_->getData()) {
        // Check for possible parents:
        auto parent = mc_particle.getParent();
        if(parent != nullptr) {
            LOG(DEBUG) << "MCParticle " << mc_particle.getParticleID();
            continue;
        }

        // This particle has no parent particles in the regarded sensor, return it.
        LOG(DEBUG) << "MCParticle " << mc_particle.getParticleID() << " (primary)";
        primaries.push_back(&mc_particle);
    }

    return primaries;
}
