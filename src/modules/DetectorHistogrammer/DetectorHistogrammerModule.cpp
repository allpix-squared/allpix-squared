/**
 * @file
 * @brief Implementation of detector histogramming module
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "DetectorHistogrammerModule.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "core/geometry/HexagonalPixelDetectorModel.hpp"
#include "core/geometry/RadialStripDetectorModel.hpp"
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
    config_.setDefault<XYVector>("track_resolution", ROOT::Math::XYVector(Units::get(0.0, "um"), Units::get(0.0, "um")));

    config_.setDefault<DisplacementVector2D<Cartesian2D<int>>>(
        "granularity",
        DisplacementVector2D<Cartesian2D<int>>(static_cast<int>(Units::convert(model->getPixelSize().x(), "um")),
                                               static_cast<int>(Units::convert(model->getPixelSize().y(), "um"))));
    config_.setDefault<DisplacementVector2D<Cartesian2D<int>>>("granularity_local", {1, 1});
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

    // Calculate the granularity of in-pixel maps:
    auto inpixel_bins = config_.get<DisplacementVector2D<Cartesian2D<int>>>("granularity");
    if(inpixel_bins.x() * inpixel_bins.y() > 250000) {
        LOG(WARNING) << "Selected plotting granularity of " << inpixel_bins << " bins creates very large histograms."
                     << std::endl
                     << "Consider reducing the number of bins using the granularity parameter.";
    } else {
        LOG(DEBUG) << "In-pixel plot granularity: " << inpixel_bins;
    }

    // Create histogram of hitmap
    LOG(TRACE) << "Creating histograms";
    std::string hit_map_title = "Hitmap (" + detector_->getName() + ");x (pixels);y (pixels);hits";
    hit_map =
        CreateHistogram<TH2D>("hit_map", hit_map_title.c_str(), xpixels, -0.5, xpixels - 0.5, ypixels, -0.5, ypixels - 0.5);

    std::string hit_map_global_title = "Hitmap (" + detector_->getName() + ")  in global coord.;x [mm];y [mm];hits";
    auto global_ll = detector_->getGlobalPosition(model->getSensorCenter() - model->getSensorSize() / 2);
    auto global_ur = detector_->getGlobalPosition(model->getSensorCenter() + model->getSensorSize() / 2);
    auto global_lr = detector_->getGlobalPosition(
        (model->getSensorCenter() + ROOT::Math::XYZVector(model->getSensorSize().x(), -model->getSensorSize().y(), 0) / 2));
    auto global_ul = detector_->getGlobalPosition(
        (model->getSensorCenter() + ROOT::Math::XYZVector(-model->getSensorSize().x(), model->getSensorSize().y(), 0) / 2));
    hit_map_global = CreateHistogram<TH2D>("hit_map_global",
                                           hit_map_global_title.c_str(),
                                           static_cast<int>(model->getSensorSize().x()) * 10,
                                           std::min({global_ll.x(), global_ur.x(), global_lr.x(), global_ul.x()}),
                                           std::max({global_ll.x(), global_ur.x(), global_lr.x(), global_ul.x()}),
                                           static_cast<int>(model->getSensorSize().y()) * 10,
                                           std::min({global_ll.y(), global_ur.y(), global_lr.y(), global_ul.y()}),
                                           std::max({global_ll.y(), global_ur.y(), global_lr.y(), global_ul.y()}));

    std::string hit_map_local_title = "Hitmap (" + detector_->getName() + ") in local coord.;x (mm);y (mm);hits";
    hit_map_local = CreateHistogram<TH2D>("hit_map_local",
                                          hit_map_local_title.c_str(),
                                          static_cast<int>(model->getMatrixSize().x() / model->getPixelSize().x()),
                                          -model->getPixelSize().x() / 2,
                                          model->getMatrixSize().x() - model->getPixelSize().x() / 2,
                                          static_cast<int>(model->getMatrixSize().y() / model->getPixelSize().y()),
                                          -model->getPixelSize().y() / 2,
                                          model->getMatrixSize().y() - model->getPixelSize().y() / 2);

    auto local_inpixel_bins = config_.get<DisplacementVector2D<Cartesian2D<int>>>("granularity_local");
    std::string hit_map_local_mc_title =
        "MCParticle position hitmap (" + detector_->getName() + ") in local coord.;x (mm);y (mm);hits";
    hit_map_local_mc = CreateHistogram<TH2D>(
        "hit_map_local_mc",
        hit_map_local_mc_title.c_str(),
        static_cast<int>(model->getMatrixSize().x() / model->getPixelSize().x()) * local_inpixel_bins.x(),
        -model->getPixelSize().x() / 2,
        model->getMatrixSize().x() - model->getPixelSize().x() / 2,
        static_cast<int>(model->getMatrixSize().y() / model->getPixelSize().y()) * local_inpixel_bins.y(),
        -model->getPixelSize().y() / 2,
        model->getMatrixSize().y() - model->getPixelSize().y() / 2);

    std::string charge_map_title = "Pixel charge map (" + detector_->getName() + ");x (pixels);y (pixels); charge [ke]";
    charge_map = CreateHistogram<TH2D>(
        "charge_map", charge_map_title.c_str(), xpixels, -0.5, xpixels - 0.5, ypixels, -0.5, ypixels - 0.5);

    // Create histogram of cluster map
    std::string cluster_map_title = "Cluster map (" + detector_->getName() + ");x (pixels);y (pixels); clusters";
    cluster_map = CreateHistogram<TH2D>(
        "cluster_map", cluster_map_title.c_str(), xpixels, -0.5, xpixels - 0.5, ypixels, -0.5, ypixels - 0.5);

    // Create histogram of cluster map
    std::string cluster_size_map_local_title =
        "Cluster size as function of MCParticle impact position (" + detector_->getName() + ");x [mm];y [mm]";
    cluster_size_map_local = CreateHistogram<TProfile2D>(
        "cluster_size_map_local",
        cluster_size_map_local_title.c_str(),
        static_cast<int>(model->getMatrixSize().x() / model->getPixelSize().x()) * local_inpixel_bins.x(),
        -model->getPixelSize().x() / 2,
        model->getMatrixSize().x() - model->getPixelSize().x() / 2,
        static_cast<int>(model->getMatrixSize().y() / model->getPixelSize().y()) * local_inpixel_bins.y(),
        -model->getPixelSize().y() / 2,
        model->getMatrixSize().y() - model->getPixelSize().y() / 2);

    std::string cluster_size_map_title =
        "Cluster size as function of in-pixel impact position (" + detector_->getName() + ");x%pitch [#mum];y%pitch [#mum]";
    cluster_size_map = CreateHistogram<TProfile2D>("cluster_size_map",
                                                   cluster_size_map_title.c_str(),
                                                   inpixel_bins.x(),
                                                   -pitch_x / 2,
                                                   pitch_x / 2,
                                                   inpixel_bins.y(),
                                                   -pitch_y / 2,
                                                   pitch_y / 2);

    std::string cluster_size_x_map_title = "Cluster size in X as function of in-pixel impact position (" +
                                           detector_->getName() + ");x%pitch [#mum];y%pitch [#mum]";
    cluster_size_x_map = CreateHistogram<TProfile2D>("cluster_size_x_map",
                                                     cluster_size_x_map_title.c_str(),
                                                     inpixel_bins.x(),
                                                     -pitch_x / 2,
                                                     pitch_x / 2,
                                                     inpixel_bins.y(),
                                                     -pitch_y / 2,
                                                     pitch_y / 2);

    std::string cluster_size_y_map_title = "Cluster size in Y as function of in-pixel impact position (" +
                                           detector_->getName() + ");x%pitch [#mum];y%pitch [#mum]";
    cluster_size_y_map = CreateHistogram<TProfile2D>("cluster_size_y_map",
                                                     cluster_size_y_map_title.c_str(),
                                                     inpixel_bins.x(),
                                                     -pitch_x / 2,
                                                     pitch_x / 2,
                                                     inpixel_bins.y(),
                                                     -pitch_y / 2,
                                                     pitch_y / 2);

    // Charge maps:
    std::string cluster_charge_map_title = "Cluster charge as function of in-pixel impact position (" +
                                           detector_->getName() + ");x%pitch [#mum];y%pitch [#mum];<cluster charge> [ke]";
    cluster_charge_map = CreateHistogram<TProfile2D>("cluster_charge_map",
                                                     cluster_charge_map_title.c_str(),
                                                     inpixel_bins.x(),
                                                     -pitch_x / 2,
                                                     pitch_x / 2,
                                                     inpixel_bins.y(),
                                                     -pitch_y / 2,
                                                     pitch_y / 2);
    std::string seed_charge_map_title = "Seed pixel charge as function of in-pixel impact position (" +
                                        detector_->getName() + ");x%pitch [#mum];y%pitch [#mum];<seed pixel charge> [ke]";
    seed_charge_map = CreateHistogram<TProfile2D>("seed_charge_map",
                                                  seed_charge_map_title.c_str(),
                                                  inpixel_bins.x(),
                                                  -pitch_x / 2,
                                                  pitch_x / 2,
                                                  inpixel_bins.y(),
                                                  -pitch_y / 2,
                                                  pitch_y / 2);

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
    residual_x_vs_x = CreateHistogram<TProfile>(
        "residual_x_vs_x", residual_x_vs_x_title.c_str(), inpixel_bins.x(), -pitch_x / 2, pitch_x / 2);
    std::string residual_y_vs_y_title = "Mean absolute deviation of residual in Y as function of in-pixel Y position (" +
                                        detector_->getName() + ");y%pitch [#mum];MAD(#Deltay) [#mum]";
    residual_y_vs_y = CreateHistogram<TProfile>(
        "residual_y_vs_y", residual_y_vs_y_title.c_str(), inpixel_bins.y(), -pitch_y / 2, pitch_y / 2);
    std::string residual_x_vs_y_title = "Mean absolute deviation of residual in X as function of in-pixel Y position (" +
                                        detector_->getName() + ");y%pitch [#mum];MAD(#Deltax) [#mum]";
    residual_x_vs_y = CreateHistogram<TProfile>(
        "residual_x_vs_y", residual_x_vs_y_title.c_str(), inpixel_bins.y(), -pitch_y / 2, pitch_y / 2);
    std::string residual_y_vs_x_title = "Mean absolute deviation of residual in Y as function of in-pixel X position (" +
                                        detector_->getName() + ");x%pitch [#mum];MAD(#Deltay) [#mum]";
    residual_y_vs_x = CreateHistogram<TProfile>(
        "residual_y_vs_x", residual_y_vs_x_title.c_str(), inpixel_bins.x(), -pitch_x / 2, pitch_x / 2);

    // Residual maps
    std::string residual_map_title = "Mean absolute deviation of residual as function of in-pixel impact position (" +
                                     detector_->getName() +
                                     ");x%pitch [#mum];y%pitch [#mum];MAD(#sqrt{#Deltax^{2}+#Deltay^{2}}) [#mum]";
    residual_map = CreateHistogram<TProfile2D>("residual_map",
                                               residual_map_title.c_str(),
                                               inpixel_bins.x(),
                                               -pitch_x / 2,
                                               pitch_x / 2,
                                               inpixel_bins.y(),
                                               -pitch_y / 2,
                                               pitch_y / 2);
    std::string residual_detector_title = "Mean absolute deviation of residual (" + detector_->getName() +
                                          ");x (pixels);y (pixels);MAD(#sqrt{#Deltax^{2}+#Deltay^{2}}) [#mum]";
    residual_detector = CreateHistogram<TProfile2D>(
        "residual_detector", residual_detector_title.c_str(), xpixels, -0.5, xpixels - 0.5, ypixels, -0.5, ypixels - 0.5);

    std::string residual_x_map_title = "Mean absolute deviation of residual in X as function of in-pixel impact position (" +
                                       detector_->getName() + ");x%pitch [#mum];y%pitch [#mum];MAD(#Deltax) [#mum]";
    residual_x_map = CreateHistogram<TProfile2D>("residual_x_map",
                                                 residual_x_map_title.c_str(),
                                                 inpixel_bins.x(),
                                                 -pitch_x / 2,
                                                 pitch_x / 2,
                                                 inpixel_bins.y(),
                                                 -pitch_y / 2,
                                                 pitch_y / 2);
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
    residual_y_map = CreateHistogram<TProfile2D>("residual_y_map",
                                                 residual_y_map_title.c_str(),
                                                 inpixel_bins.x(),
                                                 -pitch_x / 2,
                                                 pitch_x / 2,
                                                 inpixel_bins.y(),
                                                 -pitch_y / 2,
                                                 pitch_y / 2);
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
    efficiency_map = CreateHistogram<TProfile2D>("efficiency_map",
                                                 efficiency_map_title.c_str(),
                                                 inpixel_bins.x(),
                                                 -pitch_x / 2,
                                                 pitch_x / 2,
                                                 inpixel_bins.y(),
                                                 -pitch_y / 2,
                                                 pitch_y / 2,
                                                 0,
                                                 1);
    std::string efficiency_local_title =
        "Efficiency (" + detector_->getName() + ") MCParticle positions, local coord.;x (mm);y (mm);efficiency";
    efficiency_local = CreateHistogram<TProfile2D>(
        "efficiency_local",
        efficiency_local_title.c_str(),
        static_cast<int>(model->getMatrixSize().x() / model->getPixelSize().x()) * local_inpixel_bins.x(),
        -model->getPixelSize().x() / 2,
        model->getMatrixSize().x() - model->getPixelSize().x() / 2,
        static_cast<int>(model->getMatrixSize().y() / model->getPixelSize().y()) * local_inpixel_bins.y(),
        -model->getPixelSize().y() / 2,
        model->getMatrixSize().y() - model->getPixelSize().y() / 2,
        0,
        1);

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
    efficiency_vs_x = CreateHistogram<TProfile>(
        "efficiency_vs_x", efficiency_vs_x_title.c_str(), inpixel_bins.x(), -pitch_x / 2, pitch_x / 2, 0, 1);
    std::string efficiency_vs_y_title =
        "Efficiency as function of in-pixel Y position (" + detector_->getName() + ");y%pitch [#mum];efficiency";
    efficiency_vs_y = CreateHistogram<TProfile>(
        "efficiency_vs_y", efficiency_vs_y_title.c_str(), inpixel_bins.y(), -pitch_y / 2, pitch_y / 2, 0, 1);

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

    // Create residual plots in polar coordinates for radial_strip detectors
    auto radial_model = std::dynamic_pointer_cast<RadialStripDetectorModel>(model);
    if(radial_model != nullptr) {
        auto max_angle = radial_model->getRowAngleMax();
        auto max_pitch = static_cast<double>(Units::convert(radial_model->getAngularPitchMax(), "mrad"));
        auto stereo_angle = radial_model->getStereoAngle();
        // Use the row radii to define bin widths of the polar hitmap
        auto row_radii = radial_model->getRowRadii();

        std::string polar_hit_map_title = "Polar hitmap (" + detector_->getName() + ");#varphi (rad);r [mm];hits";
        polar_hit_map = CreateHistogram<TH2D>("polar_hit_map",
                                              polar_hit_map_title.c_str(),
                                              xpixels,
                                              -max_angle / 2 - stereo_angle,
                                              max_angle / 2 - stereo_angle,
                                              ypixels,
                                              row_radii.data());

        std::string residual_r_title = "Residual in r (" + detector_->getName() + ");r_{track} - r_{cluster} [#mum];events";
        residual_r = CreateHistogram<TH1D>("residual_r", residual_r_title.c_str(), 1000, -2 * pitch_y, 2 * pitch_y);

        std::string residual_phi_title =
            "Residual in #varphi (" + detector_->getName() + ");#varphi_{track} - #varphi_{cluster} [mrad];events";
        residual_phi =
            CreateHistogram<TH1D>("residual_phi", residual_phi_title.c_str(), 1000, -2 * max_pitch, 2 * max_pitch);
    } else {
        auto max_pitch = std::max(model->getPixelSize().X(), model->getPixelSize().Y());
        max_pitch *= (model->is<HexagonalPixelDetectorModel>() ? std::sqrt(3) / 2 : 1);

        std::string residual_r_title = "Residual in r (" + detector_->getName() + ");r_{track} - r_{cluster} [#mum];events";
        residual_r = CreateHistogram<TH1D>("residual_r",
                                           residual_r_title.c_str(),
                                           static_cast<int>(12 * Units::convert(max_pitch, "um")),
                                           0,
                                           static_cast<double>(Units::convert(max_pitch, "um")));
    }
}

void DetectorHistogrammerModule::run(Event* event) {
    using namespace ROOT::Math;

    std::shared_ptr<PixelHitMessage> pixels_message{nullptr};
    auto mcparticle_message = messenger_->fetchMessage<MCParticleMessage>(this, event);

    // Fetch detector model
    auto model = detector_->getModel();

    auto radial_model = std::dynamic_pointer_cast<RadialStripDetectorModel>(model);

    // Check that we actually received pixel hits - we might have none and just received MCParticles!
    try {
        pixels_message = messenger_->fetchMessage<PixelHitMessage>(this, event);
        LOG(DEBUG) << "Received " << pixels_message->getData().size() << " pixel hits";

        // Fill 2D hitmap histogram
        for(const auto& pixel_hit : pixels_message->getData()) {
            auto pixel_idx = pixel_hit.getPixel().getIndex();
            auto global_pos = pixel_hit.getPixel().getGlobalCenter();
            auto local_pos = pixel_hit.getPixel().getLocalCenter();

            // Add pixel
            hit_map->Fill(pixel_idx.x(), pixel_idx.y());
            hit_map_global->Fill(global_pos.x(), global_pos.y());
            hit_map_local->Fill(local_pos.x(), local_pos.y());
            charge_map->Fill(pixel_idx.x(), pixel_idx.y(), static_cast<double>(Units::convert(pixel_hit.getSignal(), "ke")));
            pixel_charge->Fill(static_cast<double>(Units::convert(pixel_hit.getSignal(), "ke")));
            // For radial_strip models also fill the polar hit map
            if(radial_model != nullptr) {
                auto hit_pos = radial_model->getPositionPolar(pixel_hit.getPixel().getLocalCenter());
                polar_hit_map->Fill(hit_pos.phi(), hit_pos.r());
            }

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

        auto clusterPos = clus.getPosition();
        auto [cluster_x, cluster_y] = model->getPixelIndex(clusterPos);
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
            auto particlePos = particle->getLocalReferencePoint();
            // Plot hist in global coordinates of the associated MCParticles:
            hit_map_local_mc->Fill(particlePos.x(), particlePos.y());
            // Add track smearing to the particle position:
            particlePos += track_smearing(track_resolution_);
            LOG(DEBUG) << "MCParticle at " << Units::display(particlePos, {"mm", "um"});

            // Find the nearest pixel
            auto [xpixel, ypixel] = model->getPixelIndex(particlePos);

            auto inPixelPos = particlePos - model->getPixelCenter(xpixel, ypixel);
            LOG(TRACE) << "MCParticle in pixel at " << Units::display(inPixelPos, {"mm", "um"});

            // Calculate residual with cluster position:
            auto residual_um_x = static_cast<double>(Units::convert(particlePos.x() - clusterPos.x(), "um"));
            auto residual_um_y = static_cast<double>(Units::convert(particlePos.y() - clusterPos.y(), "um"));
            auto residual_um_r = std::sqrt(residual_um_x * residual_um_x + residual_um_y * residual_um_y);

            // If model is radial_strip, calculate polar residuals and in-pixel positions
            if(radial_model != nullptr) {
                // Transform coordinates to polar representation
                auto strip_polar = radial_model->getPositionPolar(model->getPixelCenter(xpixel, ypixel));
                auto particle_polar = radial_model->getPositionPolar(particlePos);
                auto cluster_polar = radial_model->getPositionPolar(clusterPos);

                // Calculate r and phi residuals
                residual_um_r = static_cast<double>(Units::convert(particle_polar.r() - cluster_polar.r(), "um"));

                auto residual_mrad_phi =
                    static_cast<double>(Units::convert(particle_polar.phi() - cluster_polar.phi(), "mrad"));
                residual_phi->Fill(residual_mrad_phi);

                // Recalculate in-pixel positions
                auto delta_phi = particle_polar.phi() - strip_polar.phi();
                inPixelPos = {particle_polar.r() * sin(delta_phi), particle_polar.r() * cos(delta_phi) - strip_polar.r(), 0};
            }

            auto inPixel_um_x = static_cast<double>(Units::convert(inPixelPos.x(), "um"));
            auto inPixel_um_y = static_cast<double>(Units::convert(inPixelPos.y(), "um"));

            cluster_size_map->Fill(inPixel_um_x, inPixel_um_y, static_cast<double>(clus.getSize()));
            cluster_size_map_local->Fill(particlePos.x(), particlePos.y(), static_cast<double>(clus.getSize()));
            cluster_size_x_map->Fill(inPixel_um_x, inPixel_um_y, clusSizesXY.first);
            cluster_size_y_map->Fill(inPixel_um_x, inPixel_um_y, clusSizesXY.second);

            // Charge maps:
            cluster_charge_map->Fill(
                inPixel_um_x, inPixel_um_y, static_cast<double>(Units::convert(clus.getCharge(), "ke")));

            // Retrieve the seed pixel:
            const auto* seed_pixel = clus.getSeedPixelHit();
            seed_charge_map->Fill(
                inPixel_um_x, inPixel_um_y, static_cast<double>(Units::convert(seed_pixel->getSignal(), "ke")));
            cluster_seed_charge->Fill(static_cast<double>(Units::convert(seed_pixel->getSignal(), "ke")));

            residual_x->Fill(residual_um_x);
            residual_y->Fill(residual_um_y);
            residual_r->Fill(residual_um_r);
            residual_x_vs_x->Fill(inPixel_um_x, std::fabs(residual_um_x));
            residual_y_vs_y->Fill(inPixel_um_y, std::fabs(residual_um_y));
            residual_x_vs_y->Fill(inPixel_um_y, std::fabs(residual_um_x));
            residual_y_vs_x->Fill(inPixel_um_x, std::fabs(residual_um_y));
            residual_map->Fill(inPixel_um_x, inPixel_um_y, residual_um_r);
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
        // Calculate 2D local position of particle:
        auto particlePos = particle->getLocalReferencePoint() + track_smearing(track_resolution_);

        // Check whether the particle position is in the sensor excess, and exclude it from the efficiency calculation if so
        if(!model->isWithinMatrix(particlePos)) {
            LOG(DEBUG) << "Particle at local coordinate " << Units::display(particlePos, {"mm", "um"}) << ", pixel index ("
                       << model->getPixelIndex(particlePos).first << "," << model->getPixelIndex(particlePos).second
                       << "), hit in the sensor excess; removing from efficiency calculation.";
            continue;
        }

        // Find the nearest pixel
        auto [xpixel, ypixel] = model->getPixelIndex(particlePos);

        auto inPixelPos = particlePos - model->getPixelCenter(xpixel, ypixel);

        // If model is radial_strip, recalculate inPixelPos
        if(radial_model != nullptr) {
            // Transform coordinates to polar representation
            auto strip_polar = radial_model->getPositionPolar(model->getPixelCenter(xpixel, ypixel));
            auto particle_polar = radial_model->getPositionPolar(particlePos);

            // Overwrite inPixelPos with correct values
            auto delta_phi = particle_polar.phi() - strip_polar.phi();
            inPixelPos = {particle_polar.r() * sin(delta_phi), particle_polar.r() * cos(delta_phi) - strip_polar.r(), 0};
        }

        auto inPixel_um_x = static_cast<double>(Units::convert(inPixelPos.x(), "um"));
        auto inPixel_um_y = static_cast<double>(Units::convert(inPixelPos.y(), "um"));

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
        efficiency_local->Fill(particlePos.x(), particlePos.y(), static_cast<double>(matched));
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
    auto hit_map_global_histogram = hit_map_global->Merge();
    auto hit_map_local_histogram = hit_map_local->Merge();
    auto hit_map_local_mc_histogram = hit_map_local_mc->Merge();
    auto charge_map_histogram = charge_map->Merge();
    auto cluster_map_histogram = cluster_map->Merge();
    auto cluster_size_map_histogram = cluster_size_map->Merge();
    auto cluster_size_map_local_histogram = cluster_size_map_local->Merge();
    auto cluster_size_x_map_histogram = cluster_size_x_map->Merge();
    auto cluster_size_y_map_histogram = cluster_size_y_map->Merge();
    auto cluster_size_histogram = cluster_size->Merge();
    auto cluster_size_x_histogram = cluster_size_x->Merge();
    auto cluster_size_y_histogram = cluster_size_y->Merge();
    auto event_size_histogram = event_size->Merge();
    auto residual_x_histogram = residual_x->Merge();
    auto residual_y_histogram = residual_y->Merge();
    auto residual_r_histogram = residual_r->Merge();
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
    auto efficiency_local_histogram = efficiency_local->Merge();
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
    hit_map_global_histogram->SetOption("colz");
    hit_map_local_histogram->SetOption("colz");
    hit_map_local_mc_histogram->SetOption("colz");
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
    cluster_size_map_histogram->SetOption("colz");
    cluster_size_map_local_histogram->SetOption("colz");
    cluster_size_x_map_histogram->SetOption("colz");
    cluster_size_y_map_histogram->SetOption("colz");
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
    hit_map_global_histogram->Write();
    hit_map_local_histogram->Write();
    hit_map_local_mc_histogram->Write();

    getROOTDirectory()->mkdir("cluster_size")->cd();
    cluster_size_histogram->Write();
    cluster_size_x_histogram->Write();
    cluster_size_y_histogram->Write();
    cluster_map_histogram->Write();
    cluster_size_map_histogram->Write();
    cluster_size_map_local_histogram->Write();
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

    // Write additional histograms if radial_strip model is used
    if(detector_->getModel()->is<RadialStripDetectorModel>()) {
        getROOTDirectory()->mkdir("polar")->cd();
        auto polar_hit_map_histogram = polar_hit_map->Merge();
        auto residual_phi_histogram = residual_phi->Merge();
        polar_hit_map_histogram->Write();
        residual_r_histogram->Write();
        residual_phi_histogram->Write();
    } else {
        residual_r_histogram->Write();
    }

    getROOTDirectory()->mkdir("efficiency")->cd();
    efficiency_detector_histogram->Write();
    efficiency_map_histogram->Write();
    efficiency_local_histogram->Write();
    efficiency_vs_x_histogram->Write();
    efficiency_vs_y_histogram->Write();
}

/**
 * @brief Perform a sparse clustering on the PixelHits
 */
std::vector<Cluster> DetectorHistogrammerModule::doClustering(std::shared_ptr<PixelHitMessage>& pixels_message) const {
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
            for(const auto& cluster_pixel : cluster.getPixelHits()) {
                if(detector_->getModel()->areNeighbors(cluster_pixel->getIndex(), pixel->getIndex(), 1)) {
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
