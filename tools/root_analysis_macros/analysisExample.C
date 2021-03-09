#include <TCanvas.h>
#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TSystem.h>
#include <TTree.h>

#include <memory>

// FIXME: these includes should be absolute and provided with installation?
#include "../../src/objects/MCParticle.hpp"
#include "../../src/objects/PixelCharge.hpp"
#include "../../src/objects/PixelHit.hpp"
#include "../../src/objects/PropagatedCharge.hpp"

/**
 * Construct a ROOT TTree from the data objects that can be used for comparison
 */
void analysisExample(TFile* file, std::string detector) {
    // Read pixel hit output
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

    std::vector<allpix::PixelHit*> input_hits;
    pixel_hit_branch->SetObject(&input_hits);

    // Read MC truth
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

    std::vector<allpix::MCParticle*> input_particles;
    mc_particle_branch->SetObject(&input_particles);

    TH2D* hitmap = new TH2D("hitmap", "Hitmap; x [mm]; y [mm]; hits", 200, 0, 20, 200, 0, 20);

    TH1D* residual_x = new TH1D("residual_x", "residual x; x_{MC} - x_{hit} [mm]; hits", 200, -5, 5);
    TH1D* residual_x_related =
        new TH1D("residual_x_related", "residual X, related hits; x_{MC} - x_{hit} [mm]; hits", 200, -5, 5);
    TH1D* residual_y = new TH1D("residual_y", "residual y; y_{MC} - y_{hit} [mm]; hits", 200, -5, 5);
    TH1D* residual_y_related =
        new TH1D("residual_y_related", "residual Y, related hits; y_{MC} - y_{hit} [mm]; hits", 200, -5, 5);

    TH1D* spectrum = new TH1D("spectrum", "PixelHit signal spectrum; signal; hits", 200, 0, 100000);

    // Convert tree for every event
    for(int i = 0; i < pixel_hit_tree->GetEntries(); ++i) {
        if(i % 100 == 0) {
            std::cout << "Processing event " << i << std::endl;
        }

        pixel_hit_tree->GetEntry(i);
        mc_particle_tree->GetEntry(i);

        double position_mcparts_x = 0.;
        double position_mcparts_y = 0.;
        for(auto& mc_part : input_particles) {
            position_mcparts_x += mc_part->getLocalEndPoint().x();
            position_mcparts_y += mc_part->getLocalEndPoint().y();
        }
        position_mcparts_x /= input_particles.size();
        position_mcparts_y /= input_particles.size();

        // Set cluster sizes
        for(auto& hit : input_hits) {
            double position_hit_x = hit->getPixel().getLocalCenter().x();
            double position_hit_y = hit->getPixel().getLocalCenter().y();
            double charge = hit->getSignal();

            auto parts = hit->getMCParticles();
            double position_mcparts_related_x = 0.;
            double position_mcparts_related_y = 0.;
            for(auto& part : parts) {
                position_mcparts_related_x += part->getLocalEndPoint().x();
                position_mcparts_related_y += part->getLocalEndPoint().y();
            }
            position_mcparts_related_x /= parts.size();
            position_mcparts_related_y /= parts.size();

            hitmap->Fill(position_hit_x, position_hit_y);

            residual_x->Fill(position_mcparts_x - position_hit_x);
            residual_x_related->Fill(position_mcparts_related_x - position_hit_x);
            residual_y->Fill(position_mcparts_y - position_hit_y);
            residual_y_related->Fill(position_mcparts_related_y - position_hit_y);

            spectrum->Fill(charge);
        }
    }

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
