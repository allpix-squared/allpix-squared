// SPDX-FileCopyrightText: 2023-2024 CERN and the Allpix Squared authors
// SPDX-License-Identifier: MIT

#include <TCanvas.h>
#include <TF1.h>
#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TMarker.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <TROOT.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TTree.h>
#include <vector>

#include <memory>

#include "../../src/objects/PropagatedCharge.hpp"

/**
 * An example of how to plot the full propagation history of charge carriers, and make an animation for a single event.
 * Takes a TFile (containing MCParticle objects and PixelHit objects), a detector name, the event number to simulate, and
 * whether or not to save the animation as a result as input. Note: saving the animations as .gif files takes a long time.
 */
void propagationHistoryPlotting(TFile* file, std::string detector, int animEventNo, bool save_gif_output) {

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

    // Vector to store markers. Needed so that they aren't deleted automatically.
    std::vector<TMarker*> markers = {};

    // Define histograms

    // In 2D map form
    double pitch_x = 150;
    double pitch_y = 100;
    double pitch_x_mm = pitch_x / 1000.0;
    double pitch_y_mm = pitch_y / 1000.0;
    double thickness = 285;

    double max_time = 10000; // ps
    TH2D* propagation_map_xz_all = new TH2D("propagation_map_xz_all",
                                            "Propagation positions for all times; x [#mum]; z [#mum];",
                                            1000,
                                            0,
                                            pitch_x,
                                            1000,
                                            -thickness / 2.0,
                                            thickness / 2.0);
    TH2D* propagation_map_yz_all = new TH2D("propagation_map_yz_all",
                                            "Propagation positions for all times; y [#mum]; z [#mum];",
                                            1000,
                                            0,
                                            pitch_y,
                                            1000,
                                            -thickness / 2.0,
                                            thickness / 2.0);
    TH2D* propagation_map_xy_all = new TH2D("propagation_map_xy_all",
                                            "Propagation positions for all times; x [#mum]; y [#mum];",
                                            1000,
                                            0,
                                            pitch_x,
                                            1000,
                                            0,
                                            pitch_y);
    TH2D* propagation_map_tz_all = new TH2D("propagation_map_tz_all",
                                            "Propagation positions in z vs time; time [ps]; z [#mum];;",
                                            1000,
                                            0,
                                            max_time,
                                            1000,
                                            -thickness / 2.0,
                                            thickness / 2.0);
    TH2D* propagation_map_tx_all = new TH2D("propagation_map_tx_all",
                                            "Propagation positions in x vs time; time [ps]; x [#mum];;",
                                            1000,
                                            0,
                                            max_time,
                                            1000,
                                            0,
                                            pitch_x);
    TH2D* propagation_map_ty_all = new TH2D("propagation_map_ty_all",
                                            "Propagation positions in y vs time; time [ps]; y [#mum];;",
                                            1000,
                                            0,
                                            max_time,
                                            1000,
                                            0,
                                            pitch_y);

    std::vector<double> animTimes = {0.0};
    double timeStepSize = 100; // ps
    int numberOfSteps = 100;
    double timeCounter = 0.0;
    std::generate_n(std::back_inserter(animTimes), numberOfSteps, [&timeCounter, &timeStepSize]() {
        timeCounter += timeStepSize;
        return timeCounter;
    });
    // For each of these timestamps, we want to plot *each* propagatedcharge object's position.

    // Set up the drawing, to have axes
    TH2D* propagation_map_xz_animation = new TH2D("propagation_map_xz_animation",
                                                  "Propagation positions for all times; x [#mum]; z [#mum];",
                                                  1000,
                                                  0,
                                                  3 * pitch_x,
                                                  1000,
                                                  -thickness / 2.0,
                                                  thickness / 2.0);
    // Create canvas for GIF animition of process
    TCanvas* animCanvas_xz = new TCanvas(("animation_" + std::to_string(animEventNo) + "_xz").c_str(),
                                         ("Propagation of charge for event " + std::to_string(animEventNo)).c_str(),
                                         1280,
                                         1024);
    animCanvas_xz->cd();

    propagation_map_xz_animation->Draw();
    gStyle->SetOptStat(false);

    // Iterate over all events
    for(int i = 0; i < propagated_charge_tree->GetEntries(); ++i) {
        if(i % 100 == 0) {
            std::cout << "Processing event " << i << std::endl;
        }

        // Access next event. Pushes information into input_*
        propagated_charge_tree->GetEntry(i);

        for(auto& propagated_charge : propagated_charges) {
            std::vector<allpix::PropagatedCharge::PropagationHistory> history = propagated_charge->getPropagationHistory();
            for(auto& [time, position] : history) {
                auto inPixelPos = ROOT::Math::XYVector(std::fmod(position.x() + pitch_x_mm / 2, pitch_x_mm) * 1000,
                                                       std::fmod(position.y() + pitch_y_mm / 2, pitch_y_mm) * 1000);
                // std::cout << "In-pixel position: " << inPixelPos << std::endl;
                propagation_map_xz_all->Fill(inPixelPos.x(), position.z() * 1000.0);
                propagation_map_yz_all->Fill(inPixelPos.y(), position.z() * 1000.0);
                propagation_map_xy_all->Fill(inPixelPos.x(), inPixelPos.y());
                propagation_map_tz_all->Fill(time * 1000.0, position.z() * 1000.0);
                propagation_map_tx_all->Fill(time * 1000.0, inPixelPos.x());
                propagation_map_ty_all->Fill(time * 1000.0, inPixelPos.y());
            }
        }

        // Animate the favourite event
        if(i == animEventNo) {
            for(auto& ts : animTimes) {

                animCanvas_xz->Clear();
                animCanvas_xz->Draw();
                propagation_map_xz_animation->SetTitle(
                    ("Charge carrier positions in xz at time " + std::to_string(round(ts)) + " ps").c_str());
                propagation_map_xz_animation->Draw();

                int current_color = 1;
                for(auto& propagated_charge : propagated_charges) {

                    // Set the colour for this charge carrier
                    EColor plot_color =
                        (propagated_charge->getType() == allpix::CarrierType::ELECTRON ? EColor::kAzure : EColor::kOrange);
                    current_color = static_cast<short int>(plot_color - 9 + (static_cast<int>(current_color) + 1) % 19);
                    auto marker = new TMarker();
                    marker->SetMarkerStyle(kFullCircle);
                    marker->SetMarkerSize(static_cast<float>(propagated_charge->getCharge() * 0.1));
                    marker->SetMarkerColor(current_color);

                    std::vector<allpix::PropagatedCharge::PropagationHistory> history =
                        propagated_charge->getPropagationHistory();
                    // Get the first item that has a higher time than the timestamp
                    auto it = find_if(
                        history.begin(), history.end(), [=](auto&& item) { return std::get<0>(item) * 1000.0 > ts; });
                    ROOT::Math::XYZPoint markerPosition;
                    double markerTime = 0.0;
                    if(it != history.begin() && it != history.end()) {
                        markerPosition = std::get<1>(*(std::prev(it)));
                        markerTime = std::get<0>(*(std::prev(it))) * 1000.0;
                        // In the event that the times are exactly equal:
                        if((std::abs(ts - std::get<0>(*(it)) * 1000.0)) <
                           std::numeric_limits<double>::epsilon()) { // Floaty magic... We're not so bothered for this
                                                                     // particular thing, as it's just visualisation
                            markerPosition = std::get<1>(*it);
                            markerTime = std::get<0>(*it) * 1000.0;
                        }
                    } else {
                        // SKip this, the begin can be silly
                        continue;
                    }

                    // In um, in pixel coordinate system (3 pixels)
                    auto markerInPixelPos =
                        ROOT::Math::XYZPoint(std::fmod(markerPosition.x() + pitch_x_mm / 2, 3 * pitch_x_mm) * 1000,
                                             std::fmod(markerPosition.y() + pitch_y_mm / 2, 3 * pitch_y_mm) * 1000,
                                             markerPosition.z() * 1000.0);
                    marker->SetX(markerInPixelPos.x());
                    marker->SetY(markerInPixelPos.z());
                    gPad->Range(0, -140, 450, 140);
                    marker->Draw();
                    markers.push_back(std::move(marker)); // Push it to the vector to save it from deletion
                }
                animCanvas_xz->Modified();
                animCanvas_xz->Update();
                // Note: takes quite a long time!
                if(save_gif_output) {
                    animCanvas_xz->Print("anim_xz.gif+10");
                }

                markers.clear();
            }
        }
    }

    // Draw histograms
    TCanvas* propagation_map_all_canvas = new TCanvas("propagation_map_all_canvas", "Propagation map", 1200, 800);
    propagation_map_all_canvas->Divide(2, 2);
    propagation_map_all_canvas->cd(1);
    propagation_map_xz_all->Draw("colz");
    propagation_map_all_canvas->cd(2);
    propagation_map_yz_all->Draw("colz");
    propagation_map_all_canvas->cd(3);
    propagation_map_xy_all->Draw("colz");
    propagation_map_all_canvas->cd(4);
    propagation_map_tz_all->Draw("colz");

    TCanvas* propagation_map_times_canvas =
        new TCanvas("propagation_map_times_canvas", "Propagation maps vs time", 1200, 800);
    propagation_map_times_canvas->Divide(2, 2);
    propagation_map_times_canvas->cd(1);
    propagation_map_tx_all->Draw("colz");
    propagation_map_times_canvas->cd(2);
    propagation_map_ty_all->Draw("colz");
    propagation_map_times_canvas->cd(3);
    propagation_map_tz_all->Draw("colz");
}
