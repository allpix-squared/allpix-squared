// SPDX-FileCopyrightText: 2021-2023 CERN and the Allpix Squared authors
// SPDX-License-Identifier: MIT

#include <TCanvas.h>
#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TSystem.h>
#include <TTree.h>

#include <memory>

#include "../../src/modules/DetectorHistogrammer/Cluster.hpp"
#include "../../src/objects/MCParticle.hpp"
#include "../../src/objects/PixelCharge.hpp"
#include "../../src/objects/PixelHit.hpp"
#include "../../src/objects/PropagatedCharge.hpp"

std::vector<allpix::Cluster> doClustering(const std::vector<allpix::PixelHit*>& input_hits) {
    std::vector<allpix::Cluster> clusters;
    std::map<const allpix::PixelHit*, bool> usedPixel;

    auto pixel_it = input_hits.begin();
    for(; pixel_it != input_hits.end(); pixel_it++) {
        const allpix::PixelHit* pixel_hit = *pixel_it;

        // Check if the pixel has been used:
        if(usedPixel[pixel_hit]) {
            continue;
        }

        // Create new cluster
        allpix::Cluster cluster(pixel_hit);
        usedPixel[pixel_hit] = true;
        // std::cout << "Creating new cluster with seed: " << pixel_hit->getPixel().getIndex().X() << ", " <<
        // pixel_hit->getPixel().getIndex().Y() << std::endl;

        auto touching = [&](const allpix::PixelHit* pixel) {
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
        for(auto other_pixel = pixel_it + 1; other_pixel != input_hits.end(); other_pixel++) {
            const allpix::PixelHit* neighbour = *other_pixel;

            // Check if neighbour has been used or if it touches the current cluster:
            if(usedPixel[neighbour] || !touching(neighbour)) {
                continue;
            }

            cluster.addPixelHit(neighbour);
            // std::cout << "Adding pixel: " << neighbour->getPixel().getIndex().X() << ", " <<
            // neighbour->getPixel().getIndex().Y() << std::endl;
            usedPixel[neighbour] = true;
            other_pixel = pixel_it;
        }
        clusters.push_back(cluster);
    }
    return clusters;
}

void EtaCalculationModule::calculate_eta(const MCParticle* track, Cluster* cluster) {
    // Ignore single pixel clusters
    if(cluster->getSize() == 1) {
        LOG(DEBUG) << "Cluster size 1, skipping";
        return;
    }

    auto model = m_detector->getModel();
    auto localIntercept = track->getLocalReferencePoint();
    auto [size_x, size_y] = cluster->getSizeXY();

    if(size_x == 2) {
        LOG(DEBUG) << "Size in X is 2";
        auto min_center = std::numeric_limits<double>::max(), max_center = std::numeric_limits<double>::lowest();

        for(const auto& pixel : cluster->getPixelHits()) {
            auto idx = pixel->getIndex();
            auto center = model->getPixelCenter(idx.x(), idx.y());

            if(center.x() > max_center) {
                max_center = center.x();
            }
            if(center.x() < min_center) {
                min_center = center.x();
            }
        }
        auto reference_X = max_center - (max_center - min_center) / 2.;
        LOG(DEBUG) << "min = " << min_center;
        LOG(DEBUG) << "max = " << max_center;
        LOG(DEBUG) << "ref = " << reference_X;
        auto xmod_cluster = cluster->getPosition().X() - reference_X;
        auto xmod_track = localIntercept.X() - reference_X;
        LOG(DEBUG) << "xmod_cluster = " << xmod_cluster << " xmod_track = " << xmod_track;
        m_etaDistributionX->Fill(xmod_cluster, xmod_track);
        m_etaDistributionXprofile->Fill(xmod_cluster, xmod_track);
    }
    if(size_y == 2) {
        LOG(DEBUG) << "Size in Y is 2";
        auto min_center = std::numeric_limits<double>::max(), max_center = std::numeric_limits<double>::lowest();

        for(const auto& pixel : cluster->getPixelHits()) {
            auto idx = pixel->getIndex();
            auto center = model->getPixelCenter(idx.x(), idx.y());

            if(center.y() > max_center) {
                max_center = center.y();
            }
            if(center.y() < min_center) {
                min_center = center.y();
            }
        }
        auto reference_Y = max_center - (max_center - min_center) / 2.;
        auto ymod_cluster = cluster->getPosition().Y() - reference_Y;
        auto ymod_track = localIntercept.Y() - reference_Y;
        LOG(DEBUG) << "ymod_cluster = " << ymod_cluster << " ymod_track = " << ymod_track;
        m_etaDistributionY->Fill(ymod_cluster, ymod_track);
        m_etaDistributionYprofile->Fill(ymod_cluster, ymod_track);
    }
}

// Give the fucntion, the fit range (min and max), and the TProfile to fit
std::string EtaCalculationModule::fit(const std::string& fname, double minRange, double maxRange, TProfile* profile) {

    auto formula = config_.get<std::string>(fname);
    auto function = new TF1(fname.c_str(), formula.c_str(), minRange, maxRange);

    if(!function->IsValid()) {
        throw InvalidValueError(config_, fname, "The provided function is not a valid ROOT::TFormula expression");
    }

    // Get the eta distribution profiles and fit them to extract the correction parameters
    profile->Fit(function, "q R");
    TF1* fit = profile->GetFunction(fname.c_str());

    std::stringstream parameters;
    for(int i = 0; i < fit->GetNumberFreeParameters(); i++) {
        parameters << " " << fit->GetParameter(i);
    }
    return parameters.str();
}

/**
 * An example of how to perform an eta correction of residuals, by fitting of a fifth order polynomial
 */
void etaCorrectionResiduals(TFile* file, std::string detector) {

    // Initialise reading of the PixelHit TTrees
    TTree* pixel_hit_tree = static_cast<TTree*>(file->Get("PixelHit"));
    if(!pixel_hit_tree) {
        std::cout << "Could not read tree PixelHit, cannot continue." << std::endl;
        return;
    }
    TBranch* pixel_hit_branch = pixel_hit_tree->FindBranch(detector.c_str());
    if(!pixel_hit_branch) {
        std::cout << "Could not find the branch on tree PixelHit for the corresponding detector, cannot continue."
                  << std::endl;
        return;
    }
    // Bind the information to a predefined vector
    std::vector<allpix::PixelHit*> input_hits;
    pixel_hit_branch->SetObject(&input_hits);

    // Initialise reading of the MCParticle TTrees
    TTree* mc_particle_tree = static_cast<TTree*>(file->Get("MCParticle"));
    if(!mc_particle_tree) {
        std::cout << "Could not read tree MCParticle" << std::endl;
        return;
    }
    TBranch* mc_particle_branch = mc_particle_tree->FindBranch(detector.c_str());
    if(!mc_particle_branch) {
        std::cout << "Could not find the branch on tree MCParticle for the corresponding detector, cannot continue"
                  << std::endl;
        return;
    }
    // Bind the information to a predefined vector
    std::vector<allpix::MCParticle*> input_particles;
    mc_particle_branch->SetObject(&input_particles);

    // Initialise histograms
    // Hitmap with arbitrary field of view
    TH2D* hitmap = new TH2D("hitmap", "Hitmap; x [mm]; y [mm]; hits", 200, 0, 20, 200, 0, 20);

    // Residuals:
    // first for all hits with the mean position of all MCParticles,
    // then only using the MCParticles that are part of the PixelHit history
    TH1D* residual_x = new TH1D("residual_x", "residual x; x_{MC} - x_{hit} [mm]; hits", 200, -5, 5);
    TH1D* residual_x_related =
        new TH1D("residual_x_related", "residual X, related hits; x_{MC} - x_{hit} [mm]; hits", 200, -5, 5);
    TH1D* residual_y = new TH1D("residual_y", "residual y; y_{MC} - y_{hit} [mm]; hits", 200, -5, 5);
    TH1D* residual_y_related =
        new TH1D("residual_y_related", "residual Y, related hits; y_{MC} - y_{hit} [mm]; hits", 200, -5, 5);

    // Spectrum of the PixelHit signal
    TH1D* spectrum = new TH1D("spectrum", "PixelHit signal spectrum; signal; hits", 200, 0, 100000);

    // Iterate over all events
    for(int i = 0; i < pixel_hit_tree->GetEntries(); ++i) {
        if(i % 100 == 0) {
            std::cout << "Processing event " << i << std::endl;
        }

        // Access next event. Pushes information into input_*
        pixel_hit_tree->GetEntry(i);
        mc_particle_tree->GetEntry(i);

        auto pixels_message = messenger_->fetchMessage<PixelHitMessage>(this, event);
        auto mcparticle_message = messenger_->fetchMessage<MCParticleMessage>(this, event);

        // Perform clustering
        std::vector<Cluster> clusters = doClustering(pixels_message);

        // Loop over all tracks and look at the associated clusters to plot the eta distribution
        for(auto& cluster : clusters) {
            auto mcp = cluster.getMCParticles();

            // Calculate eta for first primary particle:
            for(const auto& m : mcp) {
                if(m->isPrimary()) {
                    calculate_eta(m, &cluster);
                    break;
                }
            }
        }

        // Calculate the mean position of all MCParticles in this event
        double position_mcparts_x = 0.;
        double position_mcparts_y = 0.;
        for(auto& mc_part : input_particles) {
            position_mcparts_x += mc_part->getLocalReferencePoint().x();
            position_mcparts_y += mc_part->getLocalReferencePoint().y();
        }
        position_mcparts_x /= input_particles.size();
        position_mcparts_y /= input_particles.size();

        // Iterate over all PixelHits
        for(auto& hit : input_hits) {
            // Retrieve information using Allpix Squared methods
            double position_hit_x = hit->getPixel().getLocalCenter().x();
            double position_hit_y = hit->getPixel().getLocalCenter().y();
            double charge = hit->getSignal();

            // Access history of the PixelHit
            auto parts = hit->getMCParticles();

            // Calculate the mean position of all MCParticles related to this hit
            double position_mcparts_related_x = 0.;
            double position_mcparts_related_y = 0.;
            for(auto& part : parts) {
                position_mcparts_related_x += part->getLocalReferencePoint().x();
                position_mcparts_related_y += part->getLocalReferencePoint().y();
            }
            position_mcparts_related_x /= parts.size();
            position_mcparts_related_y /= parts.size();

            // Fill histograms
            hitmap->Fill(position_hit_x, position_hit_y);

            residual_x->Fill(position_mcparts_x - position_hit_x);
            residual_x_related->Fill(position_mcparts_related_x - position_hit_x);
            residual_y->Fill(position_mcparts_y - position_hit_y);
            residual_y_related->Fill(position_mcparts_related_y - position_hit_y);

            spectrum->Fill(charge);
        }
    }

    // Draw histograms
    TCanvas* c0 = new TCanvas("c0", "Hitmap", 600, 400);
    hitmap->Draw("colz");

    TCanvas* c1 = new TCanvas("c1", "Residuals", 1200, 800);
    c1->Divide(2, 2);

    c1->cd(1);
    residual_x->Draw();
    c1->cd(2);
    residual_y->Draw();

    c1->cd(3);
    residual_x_related->Draw();
    c1->cd(4);
    residual_y_related->Draw();

    TCanvas* c2 = new TCanvas("c2", "Signal spectrum", 600, 400);
    spectrum->Draw();
}
