/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "TestVisualizationModule.hpp"

#include "G4RunManager.hh"
#include "G4UIsession.hh"
#include "G4UIterminal.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "G4VisManager.hh"

#include "../../core/AllPix.hpp"
#include "../../core/utils/log.h"

using namespace allpix;

const std::string TestVisualizationModule::name = "visualization_test";

TestVisualizationModule::TestVisualizationModule(AllPix *apx, ModuleIdentifier id, Configuration config): Module(apx, id) {
    config_ = config;
}
TestVisualizationModule::~TestVisualizationModule() {}

void TestVisualizationModule::init() {
    LOG(INFO) << "INITIALIZING VISUALIZATION";
    
    //initialize the session and the visualization manager
    session_g4_ = std::make_shared<G4UIterminal>();
    vis_manager_g4_ = std::make_shared<G4VisExecutive>();
    vis_manager_g4_->Initialize();
    
    // execute initialization macro if provided
    if(config_.has("macro_init")){
        G4UImanager *UI = G4UImanager::GetUIpointer();
        UI->ApplyCommand("/control/execute "+config_.get<std::string>("macro_init"));
    }
}

// run the deposition
void TestVisualizationModule::run() {
    LOG(INFO) << "VISUALIZING RESULT";
    
    // execute run macro if provided
    G4UImanager *UI = G4UImanager::GetUIpointer();
    
    if(config_.has("macro_run")){        
        UI->ApplyCommand("/control/execute "+config_.get<std::string>("macro_run"));
    }
    
    vis_manager_g4_->GetCurrentViewer()->ShowView();
    
    LOG(INFO) << "END VISUALIZATION";
}


