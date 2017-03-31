/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "VisualizationGeant4Module.hpp"

#include <cstdlib>
#include <memory>
#include <string>
#include <utility>

#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "G4UIsession.hh"
#include "G4UIterminal.hh"
#include "G4VisExecutive.hh"
#include "G4VisManager.hh"

#include "core/utils/log.h"

using namespace allpix;

const std::string VisualizationGeant4Module::name = "VisualizationGeant4";

VisualizationGeant4Module::VisualizationGeant4Module(Configuration config, Messenger*, GeometryManager*)
    : config_(std::move(config)), has_run_(false), vis_manager_g4_(nullptr) {}
VisualizationGeant4Module::~VisualizationGeant4Module() {
    if(!has_run_ && vis_manager_g4_ != nullptr) {
        LOG(DEBUG) << "Invoking VRML workaround to prevent visualization under error conditions";

        // FIXME: workaround to skip VRML visualization in case we stopped before reaching the run method
        auto str = getenv("G4VRMLFILE_VIEWER");
        if(str != nullptr) {
            setenv("G4VRMLFILE_VIEWER", "NONE", 1);
        }
        vis_manager_g4_->GetCurrentViewer()->ShowView();
        if(str != nullptr) {
            setenv("G4VRMLFILE_VIEWER", str, 1);
        }
    }
}

void VisualizationGeant4Module::init() {
    // suppress all geant4 output
    SUPPRESS_STREAM(G4cout);

    // check if we have a running G4 manager
    G4RunManager* run_manager_g4 = G4RunManager::GetRunManager();
    if(run_manager_g4 == nullptr) {
        throw ModuleError("Cannot visualize using Geant4 without a Geant4 geometry builder");
    }

    // initialize the session and the visualization manager
    LOG(INFO) << "Initializing visualization";
    vis_manager_g4_ = std::make_shared<G4VisExecutive>("quiet");
    vis_manager_g4_->Initialize();

    // execute standard commands
    // FIXME: should execute this directly and not through the UI
    G4UImanager* UI = G4UImanager::GetUIpointer();
    UI->ApplyCommand("/vis/scene/create");
    // FIXME: no way to check if this driver actually exists...
    UI->ApplyCommand("/vis/sceneHandler/create " + config_.get<std::string>("driver"));
    UI->ApplyCommand("/vis/sceneHandler/attach");

    UI->ApplyCommand("/vis/viewer/create");

    // release the g4 output
    RELEASE_STREAM(G4cout);

    // execute initialization macro if provided
    if(config_.has("macro_init")) {
        UI->ApplyCommand("/control/execute " + config_.get<std::string>("macro_init"));
    }
}

// display the visualization after all events have passed
void VisualizationGeant4Module::finalize() {
    // execute the main macro
    if(config_.has("macro_run")) {
        G4UImanager* UI = G4UImanager::GetUIpointer();
        UI->ApplyCommand("/control/execute " + config_.get<std::string>("macro_run"));
    }

    // flush the view or open an interactive session depending on settings
    if(config_.get("interactive", false)) {
        std::unique_ptr<G4UIsession> session = std::make_unique<G4UIterminal>();
        session->SessionStart();
    } else {
        LOG(INFO) << "Starting visualization viewer";
        vis_manager_g4_->GetCurrentViewer()->ShowView();
    }

    // set that we did succesfully visualize
    has_run_ = true;
}

// External function, to allow loading from dynamic library without knowing module type.
// Should be overloaded in all module implementations, added here to prevent crashes
Module* allpix::generator(Configuration config, Messenger* messenger, GeometryManager* geometry) {
    VisualizationGeant4Module* module = new VisualizationGeant4Module(config, messenger, geometry);
    return dynamic_cast<Module*>(module);
}
