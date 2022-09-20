/**
 * @file
 * @brief Implementation of detector histogramming module
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
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
#include "core/utils/distributions.h"
#include "core/utils/log.h"

#include "tools/ROOT.h"

using namespace allpix;

DetectorHistogrammerModule::DetectorHistogrammerModule(Configuration& config,
                                                       Messenger* messenger,
                                                       std::shared_ptr<Detector> detector)
    : Module(config, detector), messenger_(messenger), detector_(std::move(detector)) {
    using namespace ROOT::Math;

    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();

    // Bind messages
    messenger_->bindSingle<PixelHitMessage>(this);
    messenger_->bindSingle<MCParticleMessage>(this, MsgFlags::REQUIRED);

    auto model = detector_->getModel();
    config_.setDefault<XYVector>("matching_cut", model->getPixelSize() * 3);
    config_.setDefault<XYVector>("track_resolution", ROOT::Math::XYVector(Units::get(2.0, "um"), Units::get(2.0, "um")));

    config_.setDefault<DisplacementVector2D<Cartesian2D<int>>>(
        "granularity",
        DisplacementVector2D<Cartesian2D<int>>(static_cast<int>(Units::convert(model->getPixelSize().x(), "um")),
                                               static_cast<int>(Units::convert(model->getPixelSize().y(), "um"))));
    config_.setDefault<double>("max_cluster_charge", Units::get(50., "ke"));

    matching_cut_ = config_.get<XYVector>("matching_cut");
    track_resolution_ = config_.get<XYVector>("track_resolution");
}

void DetectorHistogrammerModule::initialize() {
    using namespace ROOT::Math;

    // Fetch detector model
    auto model = detector_->getModel();
    auto pitch_x = static_cast<double>(Units::convert(model->getPixelSize().x(), "um"));
    auto pitch_y = static_cast<double>(Units::convert(model->getPixelSize().y(), "um"));

    auto xpixels = static_cast<int>(model->getNPixels().x());
    auto ypixels = static_cast<int>(model->getNPixels().y());

    // Create histogram of hitmap
    LOG(TRACE) << "Creating histograms";
    std::string hit_map_title = "Hitmap (" + detector_->getName() + ");x (pixels);y (pixels);hits";
    hit_map =
        CreateHistogram<TH2D>("hit_map", hit_map_title.c_str(), xpixels, -0.5, xpixels - 0.5, ypixels, -0.5, ypixels - 0.5);

    std::string charge_map_title = "Pixel charge map (" + detector_->getName() + ");x (pixels);y (pixels); charge [ke]";
    charge_map = CreateHistogram<TH2D>(
        "charge_map", charge_map_title.c_str(), xpixels, -0.5, xpixels - 0.5, ypixels, -0.5, ypixels - 0.5);

    // Create histogram of cluster map
    std::string cluster_map_title = "Cluster map (" + detector_->getName() + ");x (pixels);y (pixels); clusters";
    cluster_map = CreateHistogram<TH2D>(
        "cluster_map", cluster_map_title.c_str(), xpixels, -0.5, xpixels - 0.5, ypixels, -0.5, ypixels - 0.5);

    // Calculate the granularity of in-pixel maps:
    auto inpixel_bins = config_.get<DisplacementVector2D<Cartesian2D<int>>>("granularity");
    if(inpixel_bins.x() * inpixel_bins.y() > 250000) {
        LOG(WARNING) << "Selected plotting granularity of " << inpixel_bins << " bins creates very large histograms."
                     << std::endl
                     << "Consider reducing the number of bins using the granularity parameter.";
    } else {
        LOG(DEBUG) << "In-pixel plot granularity: " << inpixel_bins;
    }

    // Create histogram of cluster map
    std::string cluster_size_map_title =
        "Cluster size as function of in-pixel impact position (" + detector_->getName() + ");x%pitch [#mum];y%pitch [#mum]";
    cluster_size_map = CreateHistogram<TProfile2D>(
        "cluster_size_map", cluster_size_map_title.c_str(), inpixel_bins.x(), 0., pitch_x, inpixel_bins.y(), 0., pitch_y);

    std::string cluster_size_x_map_title = "Cluster size in X as function of in-pixel impact position (" +
                                           detector_->getName() + ");x%pitch [#mum];y%pitch [#mum]";
    cluster_size_x_map = CreateHistogram<TProfile2D>("cluster_size_x_map",
                                                     cluster_size_x_map_title.c_str(),
                                                     inpixel_bins.x(),
                                                     0.,
                                                     pitch_x,
                                                     inpixel_bins.y(),
                                                     0.,
                                                     pitch_y);

    std::string cluster_size_y_map_title = "Cluster size in Y as function of in-pixel impact position (" +
                                           detector_->getName() + ");x%pitch [#mum];y%pitch [#mum]";
    cluster_size_y_map = CreateHistogram<TProfile2D>("cluster_size_y_map",
                                                     cluster_size_y_map_title.c_str(),
                                                     inpixel_bins.x(),
                                                     0.,
                                                     pitch_x,
                                                     inpixel_bins.y(),
                                                     0.,
                                                     pitch_y);

    // Charge maps:
    std::string cluster_charge_map_title = "Cluster charge as function of in-pixel impact position (" +
                                           detector_->getName() + ");x%pitch [#mum];y%pitch [#mum];<cluster charge> [ke]";
    cluster_charge_map = CreateHistogram<TProfile2D>("cluster_charge_map",
                                                     cluster_charge_map_title.c_str(),
                                                     inpixel_bins.x(),
                                                     0.,
                                                     pitch_x,
                                                     inpixel_bins.y(),
                                                     0.,
                                                     pitch_y);
    std::string seed_charge_map_title = "Seed pixel charge as function of in-pixel impact position (" +
                                        detector_->getName() + ");x%pitch [#mum];y%pitch [#mum];<seed pixel charge> [ke]";
    seed_charge_map = CreateHistogram<TProfile2D>(
        "seed_charge_map", seed_charge_map_title.c_str(), inpixel_bins.x(), 0., pitch_x, inpixel_bins.y(), 0., pitch_y);

    // Create cluster size plots, preventing unphysically low bin numbers
    int max_cluster_size = std::max(10, xpixels * ypixels / 10);
    std::string cluster_size_title = "Cluster size (" + detector_->getName() + ");cluster size [px];clusters";
    cluster_size =
        CreateHistogram<TH1D>("cluster_size", cluster_size_title.c_str(), max_cluster_size, 0.5, max_cluster_size + 0.5);

    std::string cluster_size_x_title = "Cluster size in X (" + detector_->getName() + ");cluster size x [px];clusters";
    cluster_size_x = CreateHistogram<TH1D>("cluster_size_x", cluster_size_x_title.c_str(), xpixels, 0.5, xpixels + 0.5);

    std::string cluster_size_y_title = "Cluster size in Y (" + detector_->getName() + ");cluster size y [px];clusters";
    cluster_size_y = CreateHistogram<TH1D>("cluster_size_y", cluster_size_y_title.c_str(), ypixels, 0.5, ypixels + 0.5);

    // Create event size plot
    std::string event_size_title = "Pixel hits per event (" + detector_->getName() + ");# pixels;events";
    event_size = CreateHistogram<TH1D>(
        "event_size_pixels", event_size_title.c_str(), xpixels * ypixels, 0.5, xpixels * ypixels + 0.5);

    // Create residual plots
    std::string residual_x_title = "Residual in X (" + detector_->getName() + ");x_{track} - x_{cluster} [#mum];events";
    residual_x = CreateHistogram<TH1D>(
        "residual_x", residual_x_title.c_str(), static_cast<int>(12 * pitch_x), -2 * pitch_x, 2 * pitch_x);
    std::string residual_y_title = "Residual in Y (" + detector_->getName() + ");y_{track} - y_{cluster} [#mum];events";
    residual_y = CreateHistogram<TH1D>(
        "residual_y", residual_y_title.c_str(), static_cast<int>(12 * pitch_y), -2 * pitch_y, 2 * pitch_y);

    // Residual projections
    std::string residual_x_vs_x_title = "Mean absolute deviation of residual in X as function of in-pixel X position (" +
                                        detector_->getName() + ");x%pitch [#mum];MAD(#Deltax) [#mum]";
    residual_x_vs_x =
        CreateHistogram<TProfile>("residual_x_vs_x", residual_x_vs_x_title.c_str(), inpixel_bins.x(), 0., pitch_x);
    std::string residual_y_vs_y_title = "Mean absolute deviation of residual in Y as function of in-pixel Y position (" +
                                        detector_->getName() + ");y%pitch [#mum];MAD(#Deltay) [#mum]";
    residual_y_vs_y =
        CreateHistogram<TProfile>("residual_y_vs_y", residual_y_vs_y_title.c_str(), inpixel_bins.y(), 0., pitch_y);
    std::string residual_x_vs_y_title = "Mean absolute deviation of residual in X as function of in-pixel Y position (" +
                                        detector_->getName() + ");y%pitch [#mum];MAD(#Deltax) [#mum]";
    residual_x_vs_y =
        CreateHistogram<TProfile>("residual_x_vs_y", residual_x_vs_y_title.c_str(), inpixel_bins.y(), 0., pitch_y);
    std::string residual_y_vs_x_title = "Mean absolute deviation of residual in Y as function of in-pixel X position (" +
                                        detector_->getName() + ");x%pitch [#mum];MAD(#Deltay) [#mum]";
    residual_y_vs_x =
        CreateHistogram<TProfile>("residual_y_vs_x", residual_y_vs_x_title.c_str(), inpixel_bins.x(), 0., pitch_x);

    // Residual maps
    std::string residual_map_title = "Mean absolute deviation of residual as function of in-pixel impact position (" +
                                     detector_->getName() +
                                     ");x%pitch [#mum];y%pitch [#mum];MAD(#sqrt{#Deltax^{2}+#Deltay^{2}}) [#mum]";
    residual_map = CreateHistogram<TProfile2D>(
        "residual_map", residual_map_title.c_str(), inpixel_bins.x(), 0., pitch_x, inpixel_bins.y(), 0., pitch_y);
    std::string residual_detector_title = "Mean absolute deviation of residual (" + detector_->getName() +
                                          ");x (pixels);y (pixels);MAD(#sqrt{#Deltax^{2}+#Deltay^{2}}) [#mum]";
    residual_detector = CreateHistogram<TProfile2D>(
        "residual_detector", residual_detector_title.c_str(), xpixels, -0.5, xpixels - 0.5, ypixels, -0.5, ypixels - 0.5);

    std::string residual_x_map_title = "Mean absolute deviation of residual in X as function of in-pixel impact position (" +
                                       detector_->getName() + ");x%pitch [#mum];y%pitch [#mum];MAD(#Deltax) [#mum]";
    residual_x_map = CreateHistogram<TProfile2D>(
        "residual_x_map", residual_x_map_title.c_str(), inpixel_bins.x(), 0., pitch_x, inpixel_bins.y(), 0., pitch_y);
    std::string residual_x_detector_title =
        "Mean absolute deviation of residual in X (" + detector_->getName() + ");x (pixels);y (pixels);MAD(#Deltax) [#mum]";
    residual_x_detector = CreateHistogram<TProfile2D>("residual_x_detector",
                                                      residual_x_detector_title.c_str(),
                                                      xpixels,
                                                      -0.5,
                                                      xpixels - 0.5,
                                                      ypixels,
                                                      -0.5,
                                                      ypixels - 0.5);

    std::string residual_y_map_title = "Mean absolute deviation of residual in Y as function of in-pixel impact position (" +
                                       detector_->getName() + ");x%pitch [#mum];y%pitch [#mum];MAD(#Deltay) [#mum]";
    residual_y_map = CreateHistogram<TProfile2D>(
        "residual_y_map", residual_y_map_title.c_str(), inpixel_bins.x(), 0., pitch_x, inpixel_bins.y(), 0., pitch_y);
    std::string residual_y_detector_title =
        "Mean absolute deviation of residual in Y (" + detector_->getName() + ");x (pixels);y (pixels);MAD(#Deltay) [#mum]";
    residual_y_detector = CreateHistogram<TProfile2D>("residual_y_detector",
                                                      residual_y_detector_title.c_str(),
                                                      xpixels,
                                                      -0.5,
                                                      xpixels - 0.5,
                                                      ypixels,
                                                      -0.5,
                                                      ypixels - 0.5);

    // Efficiency maps:
    std::string efficiency_map_title = "Efficiency as function of in-pixel impact position (" + detector_->getName() +
                                       ");x%pitch [#mum];y%pitch [#mum];efficiency";
    efficiency_map = CreateHistogram<TProfile2D>(
        "efficiency_map", efficiency_map_title.c_str(), inpixel_bins.x(), 0, pitch_x, inpixel_bins.y(), 0, pitch_y, 0, 1);
    std::string efficiency_detector_title = "Efficiency of " + detector_->getName() + ";x (pixels);y (pixels);efficiency";
    efficiency_detector = CreateHistogram<TProfile2D>("efficiency_detector",
                                                      efficiency_detector_title.c_str(),
                                                      xpixels,
                                                      -0.5,
                                                      xpixels - 0.5,
                                                      ypixels,
                                                      -0.5,
                                                      ypixels - 0.5,
                                                      0,
                                                      1);
    // Efficiency projections
    std::string efficiency_vs_x_title =
        "Efficiency as function of in-pixel X position (" + detector_->getName() + ");x%pitch [#mum];efficiency";
    efficiency_vs_x =
        CreateHistogram<TProfile>("efficiency_vs_x", efficiency_vs_x_title.c_str(), inpixel_bins.x(), 0., pitch_x, 0, 1);
    std::string efficiency_vs_y_title =
        "Efficiency as function of in-pixel Y position (" + detector_->getName() + ");y%pitch [#mum];efficiency";
    efficiency_vs_y =
        CreateHistogram<TProfile>("efficiency_vs_y", efficiency_vs_y_title.c_str(), inpixel_bins.y(), 0., pitch_y, 0, 1);

    // Create number of clusters plot
    std::string n_cluster_title = "Clusters per event (" + detector_->getName() + ");# clusters;events";
    n_cluster = CreateHistogram<TH1D>(
        "event_size_clusters", n_cluster_title.c_str(), xpixels * ypixels, 0.5, xpixels * ypixels + 0.5);

    // Create cluster charge plot
    auto max_cluster_charge = Units::convert(config_.get<double>("max_cluster_charge"), "ke");
    std::string cluster_charge_title = "Cluster charge (" + detector_->getName() + ");cluster charge [ke];clusters";
    cluster_charge = CreateHistogram<TH1D>(
        "cluster_charge", cluster_charge_title.c_str(), 1000, 0., static_cast<double>(max_cluster_charge));

    std::string cluster_seed_charge_title = "Seed pixel charge (" + detector_->getName() + ");seed charge [ke];clusters";
    cluster_seed_charge = CreateHistogram<TH1D>(
        "seed_charge", cluster_seed_charge_title.c_str(), 1000, 0., static_cast<double>(max_cluster_charge));

    std::string pixel_charge_title = "Pixel charge (" + detector_->getName() + ");pixel charge [ke];pixels";
    pixel_charge =
        CreateHistogram<TH1D>("pixel_charge", pixel_charge_title.c_str(), 1000, 0., static_cast<double>(max_cluster_charge));

    std::string total_charge_title = "Total charge per event (" + detector_->getName() + ");total charge [ke];events";
    total_charge = CreateHistogram<TH1D>(
        "total_charge", total_charge_title.c_str(), 1000, 0., static_cast<double>(max_cluster_charge * 4));
}

void DetectorHistogrammerModule::run(Event* event) {
    using namespace ROOT::Math;

    std::shared_ptr<PixelHitMessage> pixels_message{nullptr};
    auto mcparticle_message = messenger_->fetchMessage<MCParticleMessage>(this, event);

    // Fetch detector model
    auto model = detector_->getModel();

    // Check that we actually received pixel hits - we might have none and just received MCParticles!
    try {
        pixels_message = messenger_->fetchMessage<PixelHitMessage>(this, event);
        LOG(DEBUG) << "Received " << pixels_message->getData().size() << " pixel hits";

        // Fill 2D hitmap histogram
        for(const auto& pixel_hit : pixels_message->getData()) {
            auto pixel_idx = pixel_hit.getPixel().getIndex();

            // Add pixel
            hit_map->Fill(pixel_idx.x(), pixel_idx.y());
            charge_map->Fill(pixel_idx.x(), pixel_idx.y(), static_cast<double>(Units::convert(pixel_hit.getSignal(), "ke")));
            pixel_charge->Fill(static_cast<double>(Units::convert(pixel_hit.getSignal(), "ke")));

            // Update statistics
            total_hits_ += 1;
        }
    } catch(const MessageNotFoundException&) {
    }

    // Perform a clustering
    std::vector<Cluster> clusters = (pixels_message != nullptr ? doClustering(pixels_message) : std::vector<Cluster>());

    // Lambda for smearing the Monte Carlo truth position with the track resolution
    auto track_smearing = [&](auto residuals) {
        double dx = allpix::normal_distribution<double>(0, residuals.x())(event->getRandomEngine());
        double dy = allpix::normal_distribution<double>(0, residuals.y())(event->getRandomEngine());
        return DisplacementVector3D<Cartesian3D<double>>(dx, dy, 0);
    };

    // Retrieve all MC particles in this detector which are primary particles (not produced within the sensor):
    auto primary_particles = getPrimaryParticles(mcparticle_message);
    LOG(DEBUG) << "Found " << primary_particles.size() << " primary particles in this event";

    // Evaluate the clusters
    double charge_sum = 0;
    for(const auto& clus : clusters) {
        // Fill cluster histograms
        cluster_size->Fill(static_cast<double>(clus.getSize()));
        auto clusSizesXY = clus.getSizeXY();
        cluster_size_x->Fill(clusSizesXY.first);
        cluster_size_y->Fill(clusSizesXY.second);

        auto [cluster_x, cluster_y] = clus.getIndex();
        auto clusterPos = clus.getPosition();
        LOG(DEBUG) << "Cluster at indices " << cluster_x << ", " << cluster_y << "(" << clusterPos
                   << " local coordinates) with charge " << Units::display(clus.getCharge(), "ke");
        cluster_map->Fill(cluster_x, cluster_y);
        cluster_charge->Fill(static_cast<double>(Units::convert(clus.getCharge(), "ke")));
        charge_sum += clus.getCharge();

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
        for(const auto& particle : intersection) {
            auto pitch = model->getPixelSize();

            auto particlePos = particle->getLocalReferencePoint() + track_smearing(track_resolution_);
            LOG(DEBUG) << "MCParticle at " << Units::display(particlePos, {"mm", "um"});

            auto inPixelPos = XYVector(std::fmod(particlePos.x() + pitch.x() / 2, pitch.x()),
                                       std::fmod(particlePos.y() + pitch.y() / 2, pitch.y()));
            LOG(TRACE) << "MCParticle in pixel at " << Units::display(inPixelPos, {"mm", "um"});

            auto inPixel_um_x = static_cast<double>(Units::convert(inPixelPos.x(), "um"));
            auto inPixel_um_y = static_cast<double>(Units::convert(inPixelPos.y(), "um"));

            cluster_size_map->Fill(inPixel_um_x, inPixel_um_y, static_cast<double>(clus.getSize()));
            cluster_size_x_map->Fill(inPixel_um_x, inPixel_um_y, clusSizesXY.first);
            cluster_size_y_map->Fill(inPixel_um_x, inPixel_um_y, clusSizesXY.second);

            // Charge maps:
            cluster_charge_map->Fill(
                inPixel_um_x, inPixel_um_y, static_cast<double>(Units::convert(clus.getCharge(), "ke")));

            // Find the nearest pixel
            auto xpixel = static_cast<unsigned int>(std::round(particlePos.x() / pitch.x()));
            auto ypixel = static_cast<unsigned int>(std::round(particlePos.y() / pitch.y()));

            // Retrieve the seed pixel:
            const auto* seed_pixel = clus.getSeedPixelHit();
            seed_charge_map->Fill(
                inPixel_um_x, inPixel_um_y, static_cast<double>(Units::convert(seed_pixel->getSignal(), "ke")));
            cluster_seed_charge->Fill(static_cast<double>(Units::convert(seed_pixel->getSignal(), "ke")));

            // Calculate residual with cluster position:
            auto residual_um_x = static_cast<double>(Units::convert(particlePos.x() - clusterPos.x(), "um"));
            auto residual_um_y = static_cast<double>(Units::convert(particlePos.y() - clusterPos.y(), "um"));
            residual_x->Fill(residual_um_x);
            residual_y->Fill(residual_um_y);
            residual_x_vs_x->Fill(inPixel_um_x, std::fabs(residual_um_x));
            residual_y_vs_y->Fill(inPixel_um_y, std::fabs(residual_um_y));
            residual_x_vs_y->Fill(inPixel_um_y, std::fabs(residual_um_x));
            residual_y_vs_x->Fill(inPixel_um_x, std::fabs(residual_um_y));
            residual_map->Fill(
                inPixel_um_x, inPixel_um_y, std::sqrt(residual_um_x * residual_um_x + residual_um_y * residual_um_y));
            residual_x_map->Fill(inPixel_um_x, inPixel_um_y, std::fabs(residual_um_x));
            residual_y_map->Fill(inPixel_um_x, inPixel_um_y, std::fabs(residual_um_y));
            residual_detector->Fill(
                xpixel, ypixel, std::sqrt(residual_um_x * residual_um_x + residual_um_y * residual_um_y));
            residual_x_detector->Fill(xpixel, ypixel, std::fabs(residual_um_x));
            residual_y_detector->Fill(xpixel, ypixel, std::fabs(residual_um_y));
        }
    }

    // Store total charge in event:
    total_charge->Fill(static_cast<double>(Units::convert(charge_sum, "ke")));

    // Calculate efficiency: search for matching clusters for all primary MCParticles
    for(auto& particle : primary_particles) {
        auto pitch = model->getPixelSize();

        // Calculate 2D local position of particle:
        auto particlePos = particle->getLocalReferencePoint() + track_smearing(track_resolution_);

        auto noOfPixels = model->getNPixels();

        if(particlePos.x() < -pitch.x() / 2 || particlePos.x() > noOfPixels.x() * pitch.x() - pitch.x() / 2 ||
           particlePos.y() < -pitch.y() / 2 || particlePos.y() > noOfPixels.y() * pitch.y() - pitch.y() / 2) {
            LOG(DEBUG) << "Particle at local coordinates x = " << Units::display(particlePos.x(), {"mm", "um"})
                       << ", y = " << Units::display(particlePos.y(), {"mm", "um"})
                       << " hit in the sensor excess; removing from efficiency calculation.";
            continue;
        }

        auto inPixelPos = XYVector(std::fmod(particlePos.x() + pitch.x() / 2, pitch.x()),
                                   std::fmod(particlePos.y() + pitch.y() / 2, pitch.y()));
        auto inPixel_um_x = static_cast<double>(Units::convert(inPixelPos.x(), "um"));
        auto inPixel_um_y = static_cast<double>(Units::convert(inPixelPos.y(), "um"));

        // Find the nearest pixel
        auto xpixel = static_cast<unsigned int>(std::round(particlePos.x() / pitch.x()));
        auto ypixel = static_cast<unsigned int>(std::round(particlePos.y() / pitch.y()));

        auto matched_cluster = std::find_if(clusters.begin(), clusters.end(), [this, &particlePos](const Cluster& clus) {
            return (std::fabs(clus.getPosition().x() - particlePos.x()) < matching_cut_.x()) &&
                   (std::fabs(clus.getPosition().y() - particlePos.y()) < matching_cut_.y());
        });

        // Do we have a match?
        bool matched = matched_cluster != clusters.end();
        LOG(DEBUG) << "Particle at " << Units::display(particlePos, {"mm", "um"})
                   << (matched ? " has a matching cluster" : " has no matching cluster");

        efficiency_vs_x->Fill(inPixel_um_x, static_cast<double>(matched));
        efficiency_vs_y->Fill(inPixel_um_y, static_cast<double>(matched));
        efficiency_map->Fill(inPixel_um_x, inPixel_um_y, static_cast<double>(matched));
        efficiency_detector->Fill(xpixel, ypixel, static_cast<double>(matched));
    }

    // Fill further histograms
    event_size->Fill(pixels_message != nullptr ? static_cast<double>(pixels_message->getData().size()) : 0.);
    n_cluster->Fill(static_cast<double>(clusters.size()));
}

void DetectorHistogrammerModule::finalize() {
    // Print statistics
    if(total_hits_ != 0) {
        LOG(INFO) << "Plotted " << total_hits_ << " hits in total";
    }

    // Merge histograms that were possibly filled in parallel in order to change drawing options on the final object
    auto hit_map_histogram = hit_map->Merge();
    auto charge_map_histogram = charge_map->Merge();
    auto cluster_map_histogram = cluster_map->Merge();
    auto cluster_size_map_histogram = cluster_size_map->Merge();
    auto cluster_size_x_map_histogram = cluster_size_x_map->Merge();
    auto cluster_size_y_map_histogram = cluster_size_y_map->Merge();
    auto cluster_size_histogram = cluster_size->Merge();
    auto cluster_size_x_histogram = cluster_size_x->Merge();
    auto cluster_size_y_histogram = cluster_size_y->Merge();
    auto event_size_histogram = event_size->Merge();
    auto residual_x_histogram = residual_x->Merge();
    auto residual_y_histogram = residual_y->Merge();
    auto residual_x_vs_x_histogram = residual_x_vs_x->Merge();
    auto residual_y_vs_y_histogram = residual_y_vs_y->Merge();
    auto residual_x_vs_y_histogram = residual_x_vs_y->Merge();
    auto residual_y_vs_x_histogram = residual_y_vs_x->Merge();
    auto residual_map_histogram = residual_map->Merge();
    auto residual_x_map_histogram = residual_x_map->Merge();
    auto residual_y_map_histogram = residual_y_map->Merge();
    auto residual_detector_histogram = residual_detector->Merge();
    auto residual_x_detector_histogram = residual_x_detector->Merge();
    auto residual_y_detector_histogram = residual_y_detector->Merge();
    auto efficiency_vs_x_histogram = efficiency_vs_x->Merge();
    auto efficiency_vs_y_histogram = efficiency_vs_y->Merge();
    auto efficiency_detector_histogram = efficiency_detector->Merge();
    auto efficiency_map_histogram = efficiency_map->Merge();
    auto n_cluster_histogram = n_cluster->Merge();
    auto cluster_charge_histogram = cluster_charge->Merge();
    auto cluster_seed_charge_histogram = cluster_seed_charge->Merge();
    auto cluster_charge_map_histogram = cluster_charge_map->Merge();
    auto seed_charge_map_histogram = seed_charge_map->Merge();
    auto pixel_charge_histogram = pixel_charge->Merge();
    auto total_charge_histogram = total_charge->Merge();

    // FIXME Set more useful spacing maximum for cluster size histogram
    auto xmax = std::ceil(cluster_size_histogram->GetBinCenter(cluster_size_histogram->FindLastBinAbove()) + 1);
    cluster_size_histogram->GetXaxis()->SetRangeUser(0, xmax);
    // Set cluster size axis spacing
    if(static_cast<int>(xmax) < 10) {
        cluster_size_histogram->GetXaxis()->SetNdivisions(static_cast<int>(xmax) + 1, 0, 0, true);
    }

    xmax = std::ceil(cluster_size_x_histogram->GetBinCenter(cluster_size_x_histogram->FindLastBinAbove()) + 1);
    cluster_size_x_histogram->GetXaxis()->SetRangeUser(0, xmax);
    // Set cluster size_x axis spacing
    if(static_cast<int>(xmax) < 10) {
        cluster_size_x_histogram->GetXaxis()->SetNdivisions(static_cast<int>(xmax) + 1, 0, 0, true);
    }

    xmax = std::ceil(cluster_size_y_histogram->GetBinCenter(cluster_size_y_histogram->FindLastBinAbove()) + 1);
    cluster_size_y_histogram->GetXaxis()->SetRangeUser(0, xmax);
    // Set cluster size_y axis spacing
    if(static_cast<int>(xmax) < 10) {
        cluster_size_y_histogram->GetXaxis()->SetNdivisions(static_cast<int>(xmax) + 1, 0, 0, true);
    }

    // FIXME Set more useful spacing maximum for event size histogram
    xmax = std::ceil(event_size_histogram->GetBinCenter(event_size_histogram->FindLastBinAbove()) + 1);
    event_size_histogram->GetXaxis()->SetRangeUser(0, xmax);
    // Set event size axis spacing
    if(static_cast<int>(xmax) < 10) {
        event_size_histogram->GetXaxis()->SetNdivisions(static_cast<int>(xmax) + 1, 0, 0, true);
    }

    // FIXME Set more useful spacing maximum for n_cluster histogram
    xmax = std::ceil(n_cluster_histogram->GetBinCenter(n_cluster_histogram->FindLastBinAbove()) + 1);
    n_cluster_histogram->GetXaxis()->SetRangeUser(0, xmax);
    // Set cluster size axis spacing
    if(static_cast<int>(xmax) < 10) {
        n_cluster_histogram->GetXaxis()->SetNdivisions(static_cast<int>(xmax) + 1, 0, 0, true);
    }

    // FIXME Set more useful spacing maximum for cluster_charge histogram
    xmax = std::ceil(cluster_charge_histogram->GetBinCenter(cluster_charge_histogram->FindLastBinAbove()) + 1);
    cluster_charge_histogram->GetXaxis()->SetRangeUser(0, xmax);
    // Set cluster size axis spacing
    if(static_cast<int>(xmax) < 10) {
        cluster_charge_histogram->GetXaxis()->SetNdivisions(static_cast<int>(xmax) + 1, 0, 0, true);
    }

    // Set default drawing option histogram for hitmap
    hit_map_histogram->SetOption("colz");
    // Set hit_map axis spacing
    if(static_cast<int>(hit_map_histogram->GetXaxis()->GetXmax()) < 10) {
        hit_map_histogram->GetXaxis()->SetNdivisions(
            static_cast<int>(hit_map_histogram->GetXaxis()->GetXmax()) + 1, 0, 0, true);
    }
    if(static_cast<int>(hit_map_histogram->GetYaxis()->GetXmax()) < 10) {
        hit_map_histogram->GetYaxis()->SetNdivisions(
            static_cast<int>(hit_map_histogram->GetYaxis()->GetXmax()) + 1, 0, 0, true);
    }

    charge_map_histogram->SetOption("colz");
    // Set hit_map axis spacing
    if(static_cast<int>(charge_map_histogram->GetXaxis()->GetXmax()) < 10) {
        charge_map_histogram->GetXaxis()->SetNdivisions(
            static_cast<int>(charge_map_histogram->GetXaxis()->GetXmax()) + 1, 0, 0, true);
    }
    if(static_cast<int>(charge_map_histogram->GetYaxis()->GetXmax()) < 10) {
        charge_map_histogram->GetYaxis()->SetNdivisions(
            static_cast<int>(charge_map_histogram->GetYaxis()->GetXmax()) + 1, 0, 0, true);
    }

    cluster_map_histogram->SetOption("colz");
    // Set cluster_map axis spacing
    if(static_cast<int>(cluster_map_histogram->GetXaxis()->GetXmax()) < 10) {
        cluster_map_histogram->GetXaxis()->SetNdivisions(
            static_cast<int>(cluster_map_histogram->GetXaxis()->GetXmax()) + 1, 0, 0, true);
    }
    if(static_cast<int>(cluster_map_histogram->GetYaxis()->GetXmax()) < 10) {
        cluster_map_histogram->GetYaxis()->SetNdivisions(
            static_cast<int>(cluster_map_histogram->GetYaxis()->GetXmax()) + 1, 0, 0, true);
    }

    // Write histograms
    LOG(TRACE) << "Writing histograms to file";
    event_size_histogram->Write();
    n_cluster_histogram->Write();
    hit_map_histogram->Write();

    getROOTDirectory()->mkdir("cluster_size")->cd();
    cluster_size_histogram->Write();
    cluster_size_x_histogram->Write();
    cluster_size_y_histogram->Write();
    cluster_map_histogram->Write();
    cluster_size_map_histogram->Write();
    cluster_size_x_map_histogram->Write();
    cluster_size_y_map_histogram->Write();

    getROOTDirectory()->mkdir("charge")->cd();
    pixel_charge_histogram->Write();
    cluster_charge_histogram->Write();
    cluster_seed_charge_histogram->Write();
    total_charge_histogram->Write();
    charge_map_histogram->Write();
    cluster_charge_map_histogram->Write();
    seed_charge_map_histogram->Write();

    getROOTDirectory()->mkdir("residuals")->cd();
    residual_x_histogram->Write();
    residual_y_histogram->Write();
    residual_detector_histogram->Write();
    residual_x_detector_histogram->Write();
    residual_y_detector_histogram->Write();
    residual_x_vs_x_histogram->Write();
    residual_y_vs_y_histogram->Write();
    residual_x_vs_y_histogram->Write();
    residual_y_vs_x_histogram->Write();
    residual_map_histogram->Write();
    residual_x_map_histogram->Write();
    residual_y_map_histogram->Write();

    getROOTDirectory()->mkdir("efficiency")->cd();
    efficiency_detector_histogram->Write();
    efficiency_map_histogram->Write();
    efficiency_vs_x_histogram->Write();
    efficiency_vs_y_histogram->Write();
}

/**
 * @brief Perform a sparse clustering on the PixelHits
 */
std::vector<Cluster> DetectorHistogrammerModule::doClustering(std::shared_ptr<PixelHitMessage>& pixels_message) {
    std::vector<Cluster> clusters;
    std::map<const PixelHit*, bool> usedPixel;

    auto pixel_it = pixels_message->getData().begin();
    for(; pixel_it != pixels_message->getData().end(); pixel_it++) {
        const PixelHit* pixel_hit = &(*pixel_it);

        // Check if the pixel has been used:
        if(usedPixel[pixel_hit]) {
            continue;
        }

        // Create new cluster
        Cluster cluster(pixel_hit);
        usedPixel[pixel_hit] = true;
        LOG(TRACE) << "Creating new cluster with seed: " << pixel_hit->getPixel().getIndex();

        auto touching = [&](const PixelHit* pixel) {
            auto pxi1 = pixel->getIndex();
            for(const auto& cluster_pixel : cluster.getPixelHits()) {

                auto distance = [](unsigned int lhs, unsigned int rhs) { return (lhs > rhs ? lhs - rhs : rhs - lhs); };

                auto pxi2 = cluster_pixel->getIndex();
                if(distance(pxi1.x(), pxi2.x()) <= 1 && distance(pxi1.y(), pxi2.y()) <= 1) {
                    return true;
                }
            }
            return false;
        };

        // Keep adding pixels to the cluster:
        for(auto other_pixel = pixel_it + 1; other_pixel != pixels_message->getData().end(); other_pixel++) {
            const PixelHit* neighbor = &(*other_pixel);

            // Check if neighbor has been used or if it touches the current cluster:
            if(usedPixel[neighbor] || !touching(neighbor)) {
                continue;
            }

            cluster.addPixelHit(neighbor);
            LOG(TRACE) << "Adding pixel: " << neighbor->getPixel().getIndex();
            usedPixel[neighbor] = true;
            other_pixel = pixel_it;
        }
        clusters.push_back(cluster);
    }
    return clusters;
}

std::vector<const MCParticle*>
DetectorHistogrammerModule::getPrimaryParticles(std::shared_ptr<MCParticleMessage>& mcparticle_message) {
    std::vector<const MCParticle*> primaries;

    // Loop over all MCParticles available
    for(const auto& mc_particle : mcparticle_message->getData()) {
        // Check for possible parents:
        const auto* parent = mc_particle.getParent();
        if(parent != nullptr) {
            LOG(TRACE) << "MCParticle " << mc_particle.getParticleID();
            continue;
        }

        // This particle has no parent particles in the regarded sensor, return it.
        LOG(TRACE) << "MCParticle " << mc_particle.getParticleID() << " (primary)";
        primaries.push_back(&mc_particle);
    }

    return primaries;
}
