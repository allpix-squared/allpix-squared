#include <TTree.h>
#include <TFile.h>
#include <Math/Vector2D.h>
#include <Math/DisplacementVector2D.h>

#include <memory>

// FIXME: these includes should be absolute and provided with installation?
#include "../../src/objects/DepositedCharge.hpp"
#include "../..//src/objects/PixelCharge.hpp"
#include "../../src/objects/PixelHit.hpp"

// FIXME: pixel size should be available in the data
std::shared_ptr<TTree> constructComparisonTree(TFile *file, std::string dut, ROOT::Math::XYVector pixel_size) {
    // Read pixel hit output
    TTree *pixel_hit_tree = static_cast<TTree*>(file->Get("PixelHit"));
    TBranch *pixel_hit_branch = pixel_hit_tree->FindBranch(dut.c_str());
    std::vector<allpix::PixelHit*> input_hits;
    pixel_hit_branch->SetObject(&input_hits);
    
    // Read pixel charge output
    TTree *pixel_charge_tree = static_cast<TTree*>(file->Get("PixelCharge"));
    TBranch *pixel_charge_branch = pixel_charge_tree->FindBranch(dut.c_str());
    std::vector<allpix::PixelCharge*> input_charges;
    pixel_charge_branch->SetObject(&input_charges);
    
    // Read deposited charge output
    TTree *deposited_charge_tree = static_cast<TTree*>(file->Get("DepositedCharge"));
    TBranch *deposited_charge_branch = deposited_charge_tree->FindBranch(dut.c_str());
    std::vector<allpix::DepositedCharge*> input_deposits;
    deposited_charge_branch->SetObject(&input_deposits);
        
    // Initialize output tree and branches
    auto output_tree = std::make_shared<TTree>("clusters", ("Cluster information for " + dut).c_str());
    int event_num;
    // Event number
    output_tree->Branch("eventNr", &event_num);
    // Cluster size
    int output_cluster, output_cluster_x, output_cluster_y; double aspect_ratio;
    output_tree->Branch("size", &output_cluster);
    output_tree->Branch("sizeX", &output_cluster_x);
    output_tree->Branch("sizeY", &output_cluster_y);
    output_tree->Branch("aspectRatio", &aspect_ratio);
    // Charge info
    int output_total_charge; std::vector<int> output_charge;
    output_tree->Branch("totalCharge", &output_total_charge);
    output_tree->Branch("charge", &output_charge);
    // Signal info
    int output_total_signal; std::vector<int> output_signal;
    output_tree->Branch("totalSignal", &output_total_signal);
    output_tree->Branch("signal", &output_signal);
    // Single pixel row / col
    std::vector<int> output_rows; std::vector<int> output_cols;
    output_tree->Branch("row", &output_rows);
    output_tree->Branch("col", &output_cols);
    // Real track information
    int output_track_count; double output_track_x, output_track_y;
    output_tree->Branch("trackCount", &output_track_count); // FIXME: problems arise if not one
    output_tree->Branch("trackLocalX", &output_track_x);
    output_tree->Branch("trackLocalY", &output_track_y);
    // Calculated track information and residuals
    double output_x, output_y, output_res_x, output_res_y;
    output_tree->Branch("localX", &output_track_x);
    output_tree->Branch("localY", &output_track_y);
    output_tree->Branch("resX", &output_res_x);
    output_tree->Branch("resY", &output_res_y);
    
    // Convert tree for every event
    for(int i = 0; i<pixel_hit_tree->GetEntries(); ++i) {
        pixel_hit_tree->GetEntry(i);
        pixel_charge_tree->GetEntry(i);
        deposited_charge_tree->GetEntry(i);
        
        // Set event number
        event_num = i+1;
        
        // Set cluster sizes
        output_cluster = input_hits.size();
        std::set<int> unique_x;
        std::set<int> unique_y;
        for(auto& hit : input_hits) {
            unique_x.insert(hit->getPixel().x());
            unique_y.insert(hit->getPixel().y());
        }
        output_cluster_x = unique_x.size();
        output_cluster_y = unique_y.size();
        aspect_ratio = static_cast<double>(output_cluster_y)/output_cluster_x;
        
        // Set charge information
        output_charge.clear();
        output_total_charge = 0;
        for(auto& pixel_charge : input_charges) {
            output_charge.push_back(pixel_charge->getCharge());
            output_total_charge += pixel_charge->getCharge();
        }
        
        // Set signal information
        output_signal.clear();
        output_total_signal = 0;
        for(auto& hit : input_hits) {
            output_signal.push_back(hit->getSignal());
            output_total_signal += hit->getSignal();
        }
        
        // Set pixel position information
        output_rows.clear(); 
        output_cols.clear();
        for(auto& hit : input_hits) {
            // FIXME defined order
            output_rows.push_back(hit->getPixel().y());
            output_cols.push_back(hit->getPixel().x());
        }
        
        // Guess information about the track (should be an extra object)
        double min_position_z = 0;
        for(auto& deposit : input_deposits) {
            min_position_z = std::min(min_position_z, deposit->getPosition().z()); 
        }
        output_track_count = 0;
        for(auto& deposit : input_deposits) {
            if(std::fabs(min_position_z - deposit->getPosition().z()) < 1e-9) {
                ++output_track_count;
                // FIXME just picks the last 'track' now
                output_track_x = deposit->getPosition().x();
                output_track_y = deposit->getPosition().y();
            }
        }
        
        // Calculate local x using a simple center of gravity fit
        // FIXME no corrections are applied
        ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<double>> totalPixel;
        double totalSignal = 0;
        for(auto& hit : input_hits) {
            // FIXME: should not use the row / column but their position
            totalPixel += hit->getPixel() * hit->getSignal();
            totalSignal += hit->getSignal();
        }
        totalPixel /= totalSignal;
        output_x = totalPixel.x() * pixel_size.x();
        output_y = totalPixel.y() * pixel_size.y();
        output_res_x = output_track_x - output_x;
        output_res_y = output_track_y - output_y;
        
        output_tree->Fill();
    }
    
    return output_tree;
}
