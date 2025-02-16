// SPDX-FileCopyrightText: 2023-2024 CERN and the Allpix Squared authors
// SPDX-License-Identifier: MIT

#include <TCanvas.h>
#include <TF1.h>
#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <TSystem.h>
#include <TTree.h>
#include <vector>

#include <memory>

#include "../../src/objects/PropagatedCharge.hpp"

/**
 * An example of how to perform an eta correction of residuals, by fitting of a fifth order polynomial.
 * Takes a TFile (containing MCParticle objects and PixelHit objects) and a detector name as input.
 */
void propagationHistoryPlotting(TFile* file, std::string detector) {

    // Initialise reading of the PropagatedCharge TTrees
    TTree* propagated_charge_tree = static_cast<TTree*>(file->Get("PropagatedCharge"));
    if(!propagated_charge_tree) {
        std::cout << "Could not read tree PropagatedCharge, cannot continue." << std::endl;
        return;
    }
    TBranch* propagated_charge_branch = propagated_charge_tree->FindBranch(detector.c_str());
    if(!propagated_charge_branch) {
        std::cout << "Could not find the branch on tree PixelHit for the corresponding detector, cannot continue."
                  << std::endl;
        return;
    }
    // Bind the information to a predefined vector
    std::vector<allpix::PropagatedCharge*> propagated_charges;
    propagated_charge_branch->SetObject(&propagated_charges);

    // Define histograms

    // In 2D map form
    // TProfile2D* residual_map_full =
    //    new TProfile2D("residual_map_full",
    //                   "Mean 2D residual vs particle hit position; x [#mum]; y [#mum]; Residual [#mum]",
    //                   noOfResBinsXY,
    //                   0,
    //                   pitch_x,
    //                   noOfResBinsXY,
    //                   0,
    //                   pitch_y);

    // Iterate over all events, for eta calculation
    for(int i = 0; i < propagated_charge_tree->GetEntries(); ++i) {
        if(i % 100 == 0) {
            std::cout << "Processing event " << i << " for eta calculation" << std::endl;
        }

        // Access next event. Pushes information into input_*
        propagated_charge_tree->GetEntry(i);

        for(auto& propagated_charge : propagated_charges) {
            std::vector<allpix::PropagatedCharge::PropagationHistory> history = propagated_charge->getPropagationHistory();
            std::cout << "Vector size: " << history.size() << std::endl;
            for(auto& [time, position] : history) {
                std::cout << "Time: " << time << ", position: " << position << std::endl;
            }
            for(size_t i = 0; i < history.size(); i++) {
                std::cout << "Time: " << std::get<0>(history[i]) << ", position: " << std::get<1>(history[i]) << std::endl;
            }
            std::cout << "Position directly: " << propagated_charge->getLocalPosition() << std::endl;
        }
    }

    // Draw histograms
    // TCanvas* etaCorrectionCanvas = new TCanvas("etaCorrectionCanvas", "Eta correction", 1200, 800);
    // etaCorrectionCanvas->Divide(2, 2);
    // etaCorrectionCanvas->cd(1);
    // etaDistributionX->Draw("colz");
    // etaCorrectionCanvas->cd(2);
    // etaDistributionXprofile->Draw();
    // etaCorrectionCanvas->cd(3);
    // etaDistributionY->Draw("colz");
    // etaCorrectionCanvas->cd(4);
    // etaDistributionYprofile->Draw();
}
