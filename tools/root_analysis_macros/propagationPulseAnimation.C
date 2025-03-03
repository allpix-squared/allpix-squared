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

#include "../../src/objects/PixelCharge.hpp"
#include "../../src/objects/PropagatedCharge.hpp"

/**
 * An example of how to plot the full propagation history of charge carriers, and make an animation for a single event.
 * Takes a TFile (containing PropagatedCharge objects and PixelCharge objects), a detector name, the event number to
 * simulate, and whether or not to save the animation as a result as input. Note: saving the animations as .gif files takes a
 * long time.
 */
void propagationPulseAnimation(TFile* dataFile, std::string detector, int animEventNo = 0, bool save_gif_output = false) {

    // Initialise reading of the PropagatedCharge and PixelCharge TTrees
    TTree* propagated_charge_tree = static_cast<TTree*>(dataFile->Get("PropagatedCharge"));
    if(!propagated_charge_tree) {
        std::cout << "Could not read tree PropagatedCharge, cannot continue." << std::endl;
        return;
    }
    TTree* pixel_charge_tree = static_cast<TTree*>(dataFile->Get("PixelCharge"));
    if(!pixel_charge_tree) {
        std::cout << "Could not read tree PixelCharge, cannot continue." << std::endl;
        return;
    }
    TBranch* propagated_charge_branch = propagated_charge_tree->FindBranch(detector.c_str());
    if(!propagated_charge_branch) {
        std::cout << "Could not find the branch on tree PropagatedCharge for the corresponding detector, cannot continue."
                  << std::endl;
        return;
    }
    TBranch* pixel_charge_branch = pixel_charge_tree->FindBranch(detector.c_str());
    if(!pixel_charge_branch) {
        std::cout << "Could not find the branch on tree PixelCharge for the corresponding detector, cannot continue."
                  << std::endl;
        return;
    }
    // Bind the information to a predefined vector
    std::vector<allpix::PropagatedCharge*> propagated_charges;
    propagated_charge_branch->SetObject(&propagated_charges);

    std::vector<allpix::PixelCharge*> pixel_charges;
    pixel_charge_branch->SetObject(&pixel_charges);

    // Each pixelcharge can have a pulse. So we can slice the pulse on time as well. but how best to display? An event can
    // have many pixelcharge objects, and thus many pulses Do I want to focus on just one, or display them all? Or sum them
    // all up, as a "total charge from sensor" thing? Or just do the biggest one, highest collected charge? Maybe that's the
    // way to go. Find the highest, center the whole thing on that and display it. I think that makes sense.

    // Vector to store plot markers. Needed so that they aren't deleted automatically.
    std::vector<TMarker*> markers = {};

    double thickness = 400;

    // Setup for animation; define the timestep size, and the number of steps
    std::vector<double> animTimes = {0.0};
    double timeStepSize = 100; // ps
    int numberOfSteps = 100;
    double timeCounter = 0.0;
    std::generate_n(std::back_inserter(animTimes), numberOfSteps, [&timeCounter, &timeStepSize]() {
        timeCounter += timeStepSize;
        return timeCounter;
    });
    // For each of these timestamps, we want to plot *each* propagatedcharge object's position.

    // Access selected event
    propagated_charge_tree->GetEntry(animEventNo);
    pixel_charge_tree->GetEntry(animEventNo);

    // Find the pixel that gets the maximum charge. That's the one we want to center our attentions on
    double maxCharge = 0.0;
    ROOT::Math::XYZPoint maxChargePixelPos;
    allpix::Pulse maxPulse;
    for(auto& pixel_charge : pixel_charges) {
        auto pulse = pixel_charge->getPulse();
        double totalTime = pulse.getBinning() * (pulse.size() - 1);
        std::cout << "Pixel pulse binning: " << pulse.getBinning() << ", charge: " << pixel_charge->getCharge()
                  << ", pos. : " << pixel_charge->getPixel().getLocalCenter() << ", total time: " << totalTime << std::endl;
        if(std::abs(pixel_charge->getCharge()) > maxCharge) {
            maxCharge = std::abs(pixel_charge->getCharge());
            maxChargePixelPos = pixel_charge->getPixel().getLocalCenter();
            maxPulse = pulse;
        }
    }
    std::cout << "The maximum: " << maxChargePixelPos << ", charge: " << maxCharge << std::endl;

    ROOT::Math::XYVector pixelSize = pixel_charges[0]->getPixel().getSize();
    std::cout << "Pixel size: " << pixelSize << std::endl;

    // Set up the drawing, to have axes. 3x the pixel pitch in this case
    TH2D* propagation_map_xz_animation = new TH2D("propagation_map_xz_animation",
                                                  "Propagation positions for all times; x [#mum]; z [#mum];",
                                                  1000,
                                                  (maxChargePixelPos.x() - pixelSize.x() / 2) * 1000,
                                                  (maxChargePixelPos.x() + pixelSize.x() / 2) * 1000,
                                                  1000,
                                                  -thickness / 2.0,
                                                  thickness / 2.0);
    // Create canvas for GIF animition of process
    TCanvas* animCanvas_xz = new TCanvas(("animation_" + std::to_string(animEventNo) + "_xz").c_str(),
                                         ("Propagation of charge for event " + std::to_string(animEventNo)).c_str(),
                                         2 * 1280,
                                         1024);
    animCanvas_xz->Divide(2, 1);
    animCanvas_xz->cd(1);

    propagation_map_xz_animation->Draw();
    gStyle->SetOptStat(false);

    animCanvas_xz->cd(2);
    // Pulse is a vector of doubles, with a given bin width. So in this way, element 0 is between 0 and 0.01, bin 1 is
    // between 0.01 and 0.02 etc. How to slice it? Find the element that times binning gets us to the point before the
    // plotting time point.

    TH1D* pulseHisto = new TH1D("Pulse", "Pulse; t [ns]; I [unit]", 2500, 0, 2500);
    for(int i = 0; i < maxPulse.size(); i++) {
        std::cout << " Time: " << i * 0.01 << ", " << maxPulse.at(i);
    }
    std::cout << std::endl;
    // pulseHisto->Draw();
    // How to do it? Fill the whole thing, then "uncover" as time progresses? Or setbincontent for all points below current
    // time...
    //  I should use SetContent: https://root.cern.ch/doc/v608/classTH1.html#a828aa3b97264611a20c3849e1820b28b
    //  Slice the vector at the given point, and use SetContent. Should only set the nonzero ones, basically.
    //  Just cruical that the histogram matches the pulse exactly, but it should if defined correctly.
    //  v2 = std::vector<int>(v1.begin() + 1, v1.end());
    //  Grabbing the vector as a pointer: double * a = &v[0];
    pulseHisto->SetContent(&maxPulse[0]);
    pulseHisto->Draw();

    // Animate the requested event
    for(auto& ts : animTimes) {

        animCanvas_xz->Clear();
        animCanvas_xz->Divide(2, 1);
        animCanvas_xz->Draw();
        propagation_map_xz_animation->SetTitle(
            ("Charge carrier positions in xz at time " + std::to_string(static_cast<int>(round(ts))) + " ps").c_str());

        animCanvas_xz->cd(1);
        propagation_map_xz_animation->Draw();

        int current_color = 1;
        for(auto& propagated_charge : propagated_charges) {

            // Set the colour for this charge carrier
            EColor plot_color =
                (propagated_charge->getType() == allpix::CarrierType::ELECTRON ? EColor::kAzure : EColor::kOrange);
            current_color = static_cast<short int>(plot_color - 9 + (static_cast<int>(current_color) + 1) % 19);
            auto marker = new TMarker();
            marker->SetMarkerStyle(kFullCircle);
            // Marker size dependnet on charge carrier group size
            marker->SetMarkerSize(static_cast<float>(propagated_charge->getCharge() * 0.1));
            marker->SetMarkerColor(current_color);

            // Retrieve the history for this PropagatedCharge object
            std::vector<allpix::PropagatedCharge::PropagationHistory> history = propagated_charge->getPropagationHistory();
            // Get the first item that has a higher time than the timestamp
            auto it = find_if(history.begin(), history.end(), [=](auto&& item) { return std::get<0>(item) * 1000.0 > ts; });
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
                // Skip this, the begin can be silly, and end means that we don't find a point corresponding to the time
                continue;
            }
            marker->SetX(markerPosition.x() * 1000.0);
            marker->SetY(markerPosition.z() * 1000.0);
            gPad->Range(0, -140, 450, 140);
            marker->Draw();
            markers.push_back(std::move(marker)); // Push it to the vector to save it from deletion
        }

        animCanvas_xz->cd(2);
        // pulseHisto->SetBinContent();
        pulseHisto->Draw();

        animCanvas_xz->Modified();
        animCanvas_xz->Update();
        // Note: takes quite a long time!
        if(save_gif_output) {
            animCanvas_xz->Print("anim_xz.gif+10");
        }

        markers.clear();
    }
}
