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
 * simulate, whether or not to save the animation as a result, and a file containing the modules output as input. Note:
 * saving the animations as .gif files takes a long time.
 */
void propagationPulseAnimation(
    TFile* dataFile, std::string detector, int animEventNo = 0, bool save_gif_output = false, TFile* modulesFile = nullptr) {

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

    // Each pixelcharge can have a pulse. So we can slice the pulse on time as well. Find the highest pulse, center the whole
    // thing on that and display it. I think that makes sense.

    // Vector to store plot markers. Needed so that they aren't deleted automatically.
    std::vector<TMarker*> markers = {};

    double thickness = 400;

    // Setup for animation; define the timestep size, and the number of steps
    std::vector<double> animTimes = {0.0};
    double timeStepSize = 100; // ps
    int numberOfSteps = 250;
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
    // If we don't have  a pulse: give a message and abort
    if(maxPulse.getBinning() == 0) {
        std::cerr
            << "ERROR: This analysis script needs to be run on results of simulations using the [PulseTransfer] module."
            << std::endl;
        return;
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
    animCanvas_xz->Divide(2, 2);
    animCanvas_xz->cd(1);

    propagation_map_xz_animation->Draw();
    gStyle->SetOptStat(false);

    // Draw field, from modules file
    TH2F* field = nullptr;
    if(modulesFile != nullptr) {
        // Select weighting potential or E-field
        std::string fieldName = "WeightingPotential";
        std::string fieldPath = "WeightingPotentialReader/" + detector + "/potential_x";
        // std::string fieldName = "ElectricField";
        // std::string fieldPath = "ElectricFieldReader/" + detector + "/field_magnitude";
        auto fieldFromFile = static_cast<TH2F*>(modulesFile->Get(fieldPath.c_str()));
        // Need to "shift" it along x; make a new histo in the right position
        field = new TH2F(fieldName.c_str(),
                         "#phi_{w}/V_{w}",
                         fieldFromFile->GetNbinsX(),
                         (maxChargePixelPos.x() - pixelSize.x() / 2) * 1000,
                         (maxChargePixelPos.x() + pixelSize.x() / 2) * 1000,
                         fieldFromFile->GetNbinsY(),
                         -thickness / 2.0,
                         thickness / 2.0);
        // Loop over old histo, fill new histo
        for(int i = 0; i < fieldFromFile->GetNbinsX(); i++) {
            for(int j = 0; j < fieldFromFile->GetNbinsY(); j++) {
                field->SetBinContent(i, j, fieldFromFile->GetBinContent(i, j));
            }
        }
    }
    // Plot the field as contours
    field->Draw("CONT1 same");

    animCanvas_xz->cd(2);
    // Pulse is a vector of doubles, with a given bin width. So in this way, element 0 is between 0 and 0.01, bin 1 is
    // between 0.01 and 0.02 etc. How to slice it? Find the element that times binning gets us to the point before the
    // plotting time point.

    TH1D* pulseHisto = new TH1D("Induced charge per unit step time",
                                "Induced charge per unit step time; t [ns]; Q_{ind} [e]",
                                maxPulse.size(),
                                0,
                                maxPulse.getBinning() * (maxPulse.size() - 1));
    //  Use SetContent: https://root.cern.ch/doc/v608/classTH1.html#a828aa3b97264611a20c3849e1820b28b
    //  Slice the vector at the given point, and use SetContent. Should only set the nonzero ones, basically.
    //  Just cruical that the histogram matches the pulse exactly, but it should if defined correctly.
    //  v2 = std::vector<int>(v1.begin() + 1, v1.end());
    //  Grabbing the vector as a pointer: double * a = &v[0];
    pulseHisto->SetContent(&maxPulse[0]);
    pulseHisto->SetMaximum(*std::max_element(maxPulse.begin(), maxPulse.end()));
    pulseHisto->SetMinimum(*std::min_element(maxPulse.begin(), maxPulse.end()));

    // Generate other interesting pulse vector variants
    // Absolute value of pulse
    auto absoluteInducedPulse = maxPulse;
    std::for_each(absoluteInducedPulse.begin(), absoluteInducedPulse.end(), [](auto& bin) { bin = std::fabs(bin); });

    // Induced current
    auto inducedCurrent = maxPulse;
    // Convert charge bins to current in uA
    std::for_each(inducedCurrent.begin(), inducedCurrent.end(), [maxPulse](auto& bin) {
        bin = static_cast<double>(bin * 1.602176634e-19 / maxPulse.getBinning() * 1e9) * 1e6;
    });

    TH1D* inducedCurrentHisto = new TH1D("Induced current",
                                         "Induced current; t [ns]; I_{ind} [#muA]",
                                         maxPulse.size(),
                                         0,
                                         maxPulse.getBinning() * (maxPulse.size() - 1));
    inducedCurrentHisto->SetContent(&inducedCurrent[0]);
    inducedCurrentHisto->SetMaximum(*std::max_element(inducedCurrent.begin(), inducedCurrent.end()));
    inducedCurrentHisto->SetMinimum(*std::min_element(inducedCurrent.begin(), inducedCurrent.end()));

    // Cumulative charge
    std::vector<double> cumulativeCharge;
    double charge = 0;
    for(const auto& bin : maxPulse) {
        charge += bin;
        cumulativeCharge.push_back(charge);
    }

    TH1D* cumulativeChargeHisto = new TH1D("Cumulative charge",
                                           "Cumulative charge; t [ns]; Q_{tot} [e]",
                                           maxPulse.size(),
                                           0,
                                           maxPulse.getBinning() * (maxPulse.size() - 1));
    cumulativeChargeHisto->SetContent(&cumulativeCharge[0]);
    cumulativeChargeHisto->SetMaximum(*std::max_element(cumulativeCharge.begin(), cumulativeCharge.end()) + 10);
    cumulativeChargeHisto->SetMinimum(*std::min_element(cumulativeCharge.begin(), cumulativeCharge.end()) - 10);

    // Animate the requested event
    for(auto& ts : animTimes) {

        animCanvas_xz->Clear();
        animCanvas_xz->Divide(2, 2);
        animCanvas_xz->Draw();
        propagation_map_xz_animation->SetTitle(
            ("Charge carrier positions in xz at time " + std::to_string(static_cast<int>(round(ts))) + " ps").c_str());

        animCanvas_xz->cd(1);
        propagation_map_xz_animation->Draw();
        field->Draw("CONT1 same");

        int current_color = 1;
        for(auto& propagated_charge : propagated_charges) {

            // Set the colour for this charge carrier
            EColor plot_color =
                (propagated_charge->getType() == allpix::CarrierType::ELECTRON ? EColor::kAzure : EColor::kOrange);
            current_color = static_cast<short int>(plot_color - 9 + (static_cast<int>(current_color) + 1) % 19);
            auto marker = new TMarker();
            marker->SetMarkerStyle(kFullCircle);
            // Marker size dependent on charge carrier group size
            marker->SetMarkerSize(static_cast<float>(propagated_charge->getCharge() * 0.5));
            marker->SetMarkerColor(current_color);

            // Retrieve the history for this PropagatedCharge object
            std::vector<allpix::PropagatedCharge::PropagationHistory> history = propagated_charge->getPropagationHistory();
            // Get the first item that has a higher time than the timestamp
            auto it = find_if(history.begin(), history.end(), [=](auto&& item) { return std::get<0>(item) * 1000.0 > ts; });
            ROOT::Math::XYZPoint markerPosition;
            double markerTime = 0.0;
            if(it != history.begin() && it != history.end() && it != (history.begin() + 1)) {
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
            marker->Draw();
            markers.push_back(std::move(marker)); // Push it to the vector to save it from deletion
        }

        animCanvas_xz->cd(2);
        // Find the bin nearest to the time
        auto nearestBin = static_cast<int>(ts / (maxPulse.getBinning() * 1000.0));
        // Create copies of the interesting vectors, with content up until the nearest bin and the rest filled with zeroes

        auto cumulativeChargePulse = std::vector<double>(cumulativeCharge.begin(), cumulativeCharge.begin() + nearestBin);
        cumulativeChargePulse.resize(maxPulse.size(), 0);
        cumulativeChargeHisto->SetContent(&cumulativeChargePulse[0]);
        cumulativeChargeHisto->Draw();

        animCanvas_xz->cd(3);
        auto inducedCurrentPulse = std::vector<double>(inducedCurrent.begin(), inducedCurrent.begin() + nearestBin);
        inducedCurrentPulse.resize(maxPulse.size(), 0);
        inducedCurrentHisto->SetContent(&inducedCurrentPulse[0]);
        inducedCurrentHisto->Draw();

        animCanvas_xz->cd(4);
        auto regularPulse = std::vector<double>(maxPulse.begin(), maxPulse.begin() + nearestBin);
        regularPulse.resize(maxPulse.size(), 0);
        pulseHisto->SetContent(&regularPulse[0]);

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
