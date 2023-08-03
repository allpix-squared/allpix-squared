// SPDX-FileCopyrightText: 2021-2023 CERN and the Allpix Squared authors
// SPDX-License-Identifier: MIT

#include <TCanvas.h>
#include <TF1.h>
#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TProfile.h>
#include <TSystem.h>
#include <TTree.h>
#include <vector>

#include <memory>

//#include "../../src/modules/DetectorHistogrammer/Cluster.hpp"
//#include "../../src/objects/MCParticle.hpp"
//#include "../../src/objects/PixelCharge.hpp"
//#include "../../src/objects/PixelHit.hpp"
//#include "../../src/objects/PropagatedCharge.hpp"
//#include "../../src/core/utils/unit.h"

#include "/home/hawk/apVersions/ap2p4p0/src/objects/MCParticle.hpp"
#include "/home/hawk/apVersions/ap2p4p0/src/objects/PixelCharge.hpp"
#include "/home/hawk/apVersions/ap2p4p0/src/objects/PixelHit.hpp"
#include "/home/hawk/apVersions/ap2p4p0/src/objects/PropagatedCharge.hpp"

#include "/home/hawk/apVersions/ap2p4p0/src/modules/DetectorHistogrammer/Cluster.hpp"

#include "/home/hawk/apVersions/ap2p4p0/src/tools/units.h"

// Global things, for simplicity
auto pitch_x = 25 / 1000.0;
auto pitch_y = 25 / 1000.0;

// Initialise histograms
std::string mod_axes_x = "in-2pixel x_{cluster} [mm];in-2pixel x_{track} [mm];";
std::string mod_axes_y = "in-2pixel y_{cluster} [mm];in-2pixel y_{track} [mm];";

auto etaDistributionX = new TH2F("etaDistributionX",
                                 std::string("2D #eta distribution X;" + mod_axes_x + "No. entries").c_str(),
                                 pitch_x * 1000 * 2,
                                 -pitch_x / 2,
                                 pitch_x / 2,
                                 pitch_x * 1000 * 2,
                                 -pitch_x / 2,
                                 pitch_x / 2);
auto etaDistributionY = new TH2F("etaDistributionY",
                                 std::string("2D #eta distribution Y;" + mod_axes_y + "No. entries").c_str(),
                                 pitch_y * 1000 * 2,
                                 -pitch_y / 2,
                                 pitch_y / 2,
                                 pitch_y * 1000 * 2,
                                 -pitch_y / 2,
                                 pitch_y / 2);
auto etaDistributionXprofile = new TProfile("etaDistributionXprofile",
                                            std::string("#eta distribution X;" + mod_axes_x).c_str(),
                                            pitch_x * 1000 * 2,
                                            -pitch_x / 2,
                                            pitch_x / 2,
                                            -pitch_x / 2,
                                            pitch_x / 2);
auto etaDistributionYprofile = new TProfile("etaDistributionYprofile",
                                            std::string("#eta distribution Y;" + mod_axes_y).c_str(),
                                            pitch_y * 1000 * 2,
                                            -pitch_y / 2,
                                            pitch_y / 2,
                                            -pitch_y / 2,
                                            pitch_y / 2);

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

std::vector<const allpix::MCParticle*> getPrimaryMCParticles(const std::vector<allpix::MCParticle*> allMCparticles) {
    std::vector<const allpix::MCParticle*> primaries;

    // Loop over all MCParticles available
    for(const auto& mc_particle : allMCparticles) {
        // Check for possible parents:
        const auto* parent = mc_particle->getParent();
        if(parent != nullptr) {
            continue;
        }
        // This particle has no parent particles in the regarded sensor, return it.
        primaries.push_back(mc_particle);
    }
    return primaries;
}

void calculate_eta(const allpix::MCParticle* track, allpix::Cluster* cluster) {
    // Ignore single pixel clusters
    if(cluster->getSize() == 1) {
        // std::cout << "Cluster size 1, skipping" << std::endl;
        return;
    }
    // std::cout << "Cluster size larger than 1, doing eta correction" << std::endl;

    auto localIntercept = track->getLocalReferencePoint();
    auto [size_x, size_y] = cluster->getSizeXY();

    if(size_x == 2) {
        // std::cout << "Size in X is 2" << std::endl;
        auto min_center = std::numeric_limits<double>::max(), max_center = std::numeric_limits<double>::lowest();

        for(const auto& pixel : cluster->getPixelHits()) {
            auto center = pixel->getPixel().getLocalCenter();

            if(center.x() > max_center) {
                max_center = center.x();
            }
            if(center.x() < min_center) {
                min_center = center.x();
            }
        }
        auto reference_X = max_center - (max_center - min_center) / 2.;
        // std::cout << "min = " << min_center << std::endl;
        // std::cout << "max = " << max_center << std::endl;
        // std::cout << "ref = " << reference_X << std::endl;
        auto xmod_cluster = cluster->getPosition().X() - reference_X;
        auto xmod_track = localIntercept.X() - reference_X;
        // std::cout << "xmod_cluster = " << xmod_cluster << " xmod_track = " << xmod_track << std::endl;
        etaDistributionX->Fill(xmod_cluster, xmod_track);
        etaDistributionXprofile->Fill(xmod_cluster, xmod_track);
    }
    if(size_y == 2) {
        // std::cout << "Size in Y is 2" << std::endl;
        auto min_center = std::numeric_limits<double>::max(), max_center = std::numeric_limits<double>::lowest();

        for(const auto& pixel : cluster->getPixelHits()) {
            auto center = pixel->getPixel().getLocalCenter();

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
        // std::cout << "ymod_cluster = " << ymod_cluster << " ymod_track = " << ymod_track << std::endl;
        etaDistributionY->Fill(ymod_cluster, ymod_track);
        etaDistributionYprofile->Fill(ymod_cluster, ymod_track);
    }
}

// Give the function, the fit range (min and max), and the TProfile to fit
TF1* fit(const std::string& fname, double minRange, double maxRange, TProfile* profile) {

    auto formula = "[0] + [1]*x + [2]*x^2 + [3]*x^3 + [4]*x^4 + [5]*x^5";
    auto function = new TF1(fname.c_str(), formula, minRange, maxRange);

    // Get the eta distribution profiles and fit them to extract the correction parameters
    profile->Fit(function, "q R");
    TF1* fit = profile->GetFunction(fname.c_str());

    return fit;
}

// Applies correction, and returns the updated cluster position
ROOT::Math::XYZPoint applyEtaCorrection(const allpix::Cluster& cluster, TF1* eta_corrector_x, TF1* eta_corrector_y) {
    // Store old cluster position
    auto old_position = cluster.getPosition();
    // Get cluster size
    auto sizeXY = cluster.getSizeXY();
    // Ignore clusters where neither directional size is 2; return old position
    if(sizeXY.first != 2 && sizeXY.second != 2) {
        return {old_position.x(), old_position.y(), old_position.z()};
    }
    double new_x = old_position.x();
    double new_y = old_position.y();

    if(sizeXY.first == 2) {
        auto min_center = std::numeric_limits<double>::max(), max_center = std::numeric_limits<double>::lowest();
        for(const auto& pixel_hit : cluster.getPixelHits()) {
            auto center = pixel_hit->getPixel().getLocalCenter();
            if(center.x() > max_center) {
                max_center = center.x();
            }
            if(center.x() < min_center) {
                min_center = center.x();
            }
        }
        auto reference_X = max_center - (max_center - min_center) / 2.0;
        auto xmod_cluster = old_position.x() - reference_X;
        new_x = eta_corrector_x->Eval(xmod_cluster) + reference_X;
    }

    if(sizeXY.second == 2) {
        auto min_center = std::numeric_limits<double>::max(), max_center = std::numeric_limits<double>::lowest();
        for(const auto& pixel_hit : cluster.getPixelHits()) {
            auto center = pixel_hit->getPixel().getLocalCenter();
            if(center.y() > max_center) {
                max_center = center.y();
            }
            if(center.y() < min_center) {
                min_center = center.y();
            }
        }
        auto reference_Y = max_center - (max_center - min_center) / 2.0;
        auto ymod_cluster = old_position.y() - reference_Y;
        new_y = eta_corrector_y->Eval(ymod_cluster) + reference_Y;
    }

    ROOT::Math::XYZPoint new_position(new_x, new_y, old_position.z());
    return new_position;
}

/**
 * An example of how to perform an eta correction of residuals, by fitting of a fifth order polynomial
 */
// void etaCorrectionResiduals(TFile* file, std::string detector) {
void etaCorrectionResiduals(std::string fileName, std::string detector) {

    auto file = new TFile(fileName.c_str());

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

    // Define histograms
    int noOfResBinsXY = 200;
    double resHalfRange = 50.0 / 1000.0;
    // Residuals from clustering; in micrometres!
    TH1D* residual_x = new TH1D("residual_x",
                                "Residual x; x_{MC} - x_{cluster} [#mum]; hits",
                                noOfResBinsXY,
                                -resHalfRange * 1000,
                                resHalfRange * 1000);
    TH1D* residual_y = new TH1D("residual_y",
                                "Residual y; y_{MC} - y_{cluster} [#mum]; hits",
                                noOfResBinsXY,
                                -resHalfRange * 1000,
                                resHalfRange * 1000);
    TH1D* residual_x_corrected = new TH1D("residual_x",
                                          "Residual x, corrected; x_{MC} - x_{cluster} [#mum]; hits",
                                          noOfResBinsXY,
                                          -resHalfRange * 1000,
                                          resHalfRange * 1000);
    TH1D* residual_y_corrected = new TH1D("residual_y",
                                          "Residual y, corrected; y_{MC} - y_{cluster} [#mum]; hits",
                                          noOfResBinsXY,
                                          -resHalfRange * 1000,
                                          resHalfRange * 1000);
    // Mean absolute deviation, i.e. absolute value of residual, vs pixel position.
    TProfile* residual_x_vs_x =
        new TProfile("residual_x_vs_x",
                     "Mean absolute deviation in x, vs in-pixel position in x; x [#mum]; |x_{MC} - x_{cluster}| [#mum]",
                     noOfResBinsXY,
                     0,
                     pitch_x * 1000);
    TProfile* residual_y_vs_y =
        new TProfile("residual_y_vs_y",
                     "Mean absolute deviation in y, vs in-pixel position in y; y [#mum]; |y_{MC} - y_{cluster}| [#mum]",
                     noOfResBinsXY,
                     0,
                     pitch_y * 1000);
    // Mean absolute deviation, i.e. absolute value of residual, vs pixel position.
    TProfile* residual_x_vs_y =
        new TProfile("residual_x_vs_y",
                     "Mean absolute deviation in x, vs in-pixel position in y; y [#mum]; |x_{MC} - x_{cluster}| [#mum]",
                     noOfResBinsXY,
                     0,
                     pitch_y * 1000);
    TProfile* residual_y_vs_x =
        new TProfile("residual_y_vs_x",
                     "Mean absolute deviation in y, vs in-pixel position in x; x [#mum]; |y_{MC} - y_{cluster}| [#mum]",
                     noOfResBinsXY,
                     0,
                     pitch_x * 1000);
    // In 2D map form
    TProfile2D* residual_map_full =
        new TProfile2D("residual_map_full",
                       "Mean 2D residual vs particle hit position; x [#mum]; y [#mum]; Residual [#mum]",
                       noOfResBinsXY,
                       0,
                       pitch_x * 1000,
                       noOfResBinsXY,
                       0,
                       pitch_y * 1000);
    TProfile2D* residual_map_x =
        new TProfile2D("residual_map_x",
                       "Mean absolute deviation in x vs particle hit position; x [#mum]; y [#mum]; MAD_x [#mum]",
                       noOfResBinsXY,
                       0,
                       pitch_x * 1000,
                       noOfResBinsXY,
                       0,
                       pitch_y * 1000);
    TProfile2D* residual_map_y =
        new TProfile2D("residual_map_y",
                       "Mean absolute deviation in y vs particle hit position; x [#mum]; y [#mum]; MAD_y [#mum]",
                       noOfResBinsXY,
                       0,
                       pitch_x * 1000,
                       noOfResBinsXY,
                       0,
                       pitch_y * 1000);

    // Iterate over all events, for eta calculation
    for(int i = 0; i < pixel_hit_tree->GetEntries(); ++i) {
        if(i % 1000 == 0) {
            std::cout << "Processing event " << i << " for eta calculation" << std::endl;
        }

        // Access next event. Pushes information into input_*
        pixel_hit_tree->GetEntry(i);
        mc_particle_tree->GetEntry(i);

        // Perform clustering
        std::vector<allpix::Cluster> clusters = doClustering(input_hits);

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
    }

    // Fit and save the eta correction functions for x and y
    auto eta_corrector_x = fit("eta_corrector_x", -pitch_x / 2, pitch_x / 2, etaDistributionXprofile);
    auto eta_corrector_y = fit("eta_corrector_y", -pitch_y / 2, pitch_y / 2, etaDistributionYprofile);

    // Iterate over all events, to draw the rest of the owl
    for(int i = 0; i < pixel_hit_tree->GetEntries(); ++i) {
        if(i % 1000 == 0) {
            std::cout << "Processing event " << i << " for analysis" << std::endl;
        }

        // Access next event. Pushes information into input_*
        pixel_hit_tree->GetEntry(i);
        mc_particle_tree->GetEntry(i);

        // Perform clustering
        std::vector<allpix::Cluster> clusters = doClustering(input_hits);

        // Get all the primary MC particles for the current event
        std::vector<const allpix::MCParticle*> primaryMCparticles = getPrimaryMCParticles(input_particles);
        // std::cout << "Number of MC particles: " << input_particles.size() << " Number of primaries: " <<
        // primaryMCparticles.size() << std::endl;

        for(const auto& cluster : clusters) {

            // Find primaries conencted with cluster (should be the only one we have for these single-particle events, but
            // still) Might be zero though, if the cluster has been created by a secondary particle. So; not all clusters are
            // connected to primary particles. Delta rays, in this case
            std::set<const allpix::MCParticle*> cluster_particles = cluster.getMCParticles();
            std::vector<const allpix::MCParticle*> intersection;
            std::set_intersection(primaryMCparticles.begin(),
                                  primaryMCparticles.end(),
                                  cluster_particles.begin(),
                                  cluster_particles.end(),
                                  std::back_inserter(intersection));

            if(intersection.size() == 0) {
                // std::cout << "Cluster from secondary MC particle!" << std::endl;
                continue;
            }
            // NOTE: anything below this will only have clusters from primary particles
            const allpix::MCParticle* clusterOriginParticle = intersection.at(0);

            ROOT::Math::XYZPoint truthParticlePosition =
                clusterOriginParticle->getLocalReferencePoint(); // Gets the extrapolated position in the middle of the
                                                                 // sensor plane, in local coords

            // In um, in pixel coordinate system
            auto inPixelPos = ROOT::Math::XYVector(std::fmod(truthParticlePosition.x() + pitch_x / 2, pitch_x) * 1000,
                                                   std::fmod(truthParticlePosition.y() + pitch_y / 2, pitch_y) * 1000);

            // Units? Cluster position is now in local coordinates, in mm.
            // Then let's also do the whole thing in um. So, another factor of 1000.
            double residualXClustering = (truthParticlePosition.x() - cluster.getPosition().x()) * 1000;
            double residualYClustering = (truthParticlePosition.y() - cluster.getPosition().y()) * 1000;
            residual_x->Fill(residualXClustering);
            residual_y->Fill(residualYClustering);

            residual_x_vs_x->Fill(inPixelPos.x(), fabs(residualXClustering));
            residual_y_vs_y->Fill(inPixelPos.y(), fabs(residualYClustering));
            residual_x_vs_y->Fill(inPixelPos.y(), fabs(residualXClustering));
            residual_y_vs_x->Fill(inPixelPos.x(), fabs(residualYClustering));
            // Residual maps
            residual_map_full->Fill(
                inPixelPos.x(),
                inPixelPos.y(),
                sqrt(residualXClustering * residualXClustering + residualYClustering * residualYClustering));
            residual_map_x->Fill(inPixelPos.x(), inPixelPos.y(), fabs(residualXClustering));
            residual_map_y->Fill(inPixelPos.x(), inPixelPos.y(), fabs(residualYClustering));

            // Apply the eta correction; retrieve the updated cluster position
            auto updatedClusterPosition = applyEtaCorrection(cluster, eta_corrector_x, eta_corrector_y);
            double residualXClustering_corrected = (truthParticlePosition.x() - updatedClusterPosition.x()) * 1000;
            double residualYClustering_corrected = (truthParticlePosition.y() - updatedClusterPosition.y()) * 1000;
            residual_x_corrected->Fill(residualXClustering_corrected);
            residual_y_corrected->Fill(residualYClustering_corrected);
        }
    }

    // Draw histograms
    TCanvas* etaCorrectionCanvas = new TCanvas("etaCorrectionCanvas", "Eta correction", 1200, 800);
    etaCorrectionCanvas->Divide(2, 2);
    etaCorrectionCanvas->cd(1);
    etaDistributionX->Draw("colz");
    etaCorrectionCanvas->cd(2);
    etaDistributionXprofile->Draw();
    etaCorrectionCanvas->cd(3);
    etaDistributionY->Draw("colz");
    etaCorrectionCanvas->cd(4);
    etaDistributionYprofile->Draw();

    TCanvas* residualsCanvas = new TCanvas("residualsCanvas", "Residuals", 1200, 800);
    residualsCanvas->Divide(2, 2);
    residualsCanvas->cd(1);
    residual_x->Draw();
    residualsCanvas->cd(2);
    residual_x_corrected->Draw();
    residualsCanvas->cd(3);
    residual_y->Draw();
    residualsCanvas->cd(4);
    residual_y_corrected->Draw();
}
