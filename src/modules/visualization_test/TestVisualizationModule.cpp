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

// run the deposition
void TestVisualizationModule::run() {
    LOG(INFO) << "VISUALIZE THE RESULT";
    
    // load the G4 run manager from allpix
    std::shared_ptr<G4RunManager> run_manager_g4 = getAllPix()->getExternalManager<G4RunManager>();
    assert(run_manager_g4); // FIXME: temporary assert (throw a proper exception later if the manager is not defined)
    
    // ALERT: TEMPORARY EXECUTE MACRO (AND HANG EVERYTHING)
    if(config_.has("macro")){
        G4VisManager* visManager = new G4VisExecutive;
        visManager->Initialize();
        
        G4UIsession *session = new G4UIterminal();
        G4UImanager *UI = G4UImanager::GetUIpointer();
        UI->ApplyCommand("/control/execute "+config_.get<std::string>("macro"));
        session->SessionStart();
        delete session;
    }
    
    LOG(INFO) << "END VISUALIZATION";
}


