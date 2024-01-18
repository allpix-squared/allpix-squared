// SPDX-FileCopyrightText: 2021-2024 CERN and the Allpix Squared authors
// SPDX-License-Identifier: MIT

#include <TCanvas.h>
#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TSystem.h>
#include <TTree.h>

#include <memory>

#include "../../src/objects/MCParticle.hpp"
#include "../../src/objects/PixelCharge.hpp"
#include "../../src/objects/PixelHit.hpp"
#include "../../src/objects/PropagatedCharge.hpp"

/**
 * An example of how to get iterate over data from Allpix Squared,
 * how to obtain information on individual objects and the object history.
 */
void analysisExample(TFile* file, std::string detector) {

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
