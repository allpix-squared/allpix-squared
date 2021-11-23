#include <TCanvas.h>
#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TSystem.h>
#include <TTree.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdlib.h>
using namespace std;
/**
 * An example of how to get iterate over data from Allpix Squared,
 * how to obtain information on individual objects and the object history.
 */
void rootMacro() {
    std::string detector = "telescope0_0";
    TFile* file = new TFile("output/cosmicsMC.root");
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
    ofstream myfile;
    myfile.open("MCTracks.txt");

    // Iterate over all events
    for(int i = 0; i < mc_particle_tree->GetEntries(); ++i) {
        if(i % 100 == 0) {
            std::cout << "Processing event " << i << std::endl;
        }
        // Access next event. Pushes information into input_*
        mc_particle_tree->GetEntry(i);
        // Loop over all particles in the event
        for(auto& mc_part : input_particles) {
            // Only look at (anti-)muons (PDG-ID: (-)13)
            if(abs(mc_part->getParticleID()) == 13) {
                // Get track info
                auto startPoint = mc_part->getLocalStartPoint();
                auto endPoint = mc_part->getLocalEndPoint();
                auto direction = startPoint - endPoint;
                std::cout << startPoint << " " << direction / direction.z() << std::endl;
                // Write to file
                myfile << startPoint << " " << direction / direction.z() << std::endl;
            }
        }
    }
    myfile.close();
}
