/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "VisualizationGeant4Module.hpp"

#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <utility>

#include <G4RunManager.hh>
#ifdef G4UI_USE_QT
#include <G4UIQt.hh>
#endif
#include <G4UImanager.hh>
#include <G4UIsession.hh>
#include <G4UIterminal.hh>
#include <G4VisExecutive.hh>

#include "core/utils/log.h"

using namespace allpix;

VisualizationGeant4Module::VisualizationGeant4Module(Configuration config, Messenger*, GeometryManager*)
    : config_(std::move(config)), has_run_(false), vis_manager_g4_(nullptr), gui_session_(nullptr) {}
VisualizationGeant4Module::~VisualizationGeant4Module() {
    if(!has_run_ && vis_manager_g4_ != nullptr && vis_manager_g4_->GetCurrentViewer() != nullptr) {
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
    // SUPPRESS_STREAM(G4cout);

    // check if we have a running G4 manager
    G4RunManager* run_manager_g4 = G4RunManager::GetRunManager();
    if(run_manager_g4 == nullptr) {
        RELEASE_STREAM(G4cout);
        throw ModuleError("Cannot visualize using Geant4 without a Geant4 geometry builder");
    }

    if(config_.has("use_gui")) {
        // Need to provide parameters, simulate this behaviour
        session_param_ = ALLPIX_PROJECT_NAME;
        session_param_ptr_ = const_cast<char*>(session_param_.data()); // NOLINT
#ifdef G4UI_USE_QT
        gui_session_ = std::make_unique<G4UIQt>(1, &session_param_ptr_);
#else
        throw InvalidValueError(
            config_, "use_gui", "GUI session cannot be started because Qt is not available in this Geant4");
#endif
    }

    // initialize the session and the visualization manager
    LOG(INFO) << "Initializing visualization";
    vis_manager_g4_ = std::make_unique<G4VisExecutive>("quiet");
    vis_manager_g4_->Initialize();

    // execute standard commands
    // FIXME: should execute this directly and not through the UI
    G4UImanager* UI = G4UImanager::GetUIpointer();
    UI->ApplyCommand("/vis/scene/create");
    // FIXME: no way to check if this driver actually exists...
    int check_driver = UI->ApplyCommand("/vis/sceneHandler/create " + config_.get<std::string>("driver"));
    if(check_driver != 0) {
        RELEASE_STREAM(G4cout);
        std::set<G4String> candidates;
        for(auto system : vis_manager_g4_->GetAvailableGraphicsSystems()) {
            for(auto& nickname : system->GetNicknames()) {
                if(!nickname.contains("FALLBACK")) {
                    candidates.insert(nickname);
                }
            }
        }

        std::string candidate_str;
        for(auto& candidate : candidates) {
            candidate_str += candidate;
            candidate_str += ", ";
        }
        if(!candidate_str.empty()) {
            candidate_str = candidate_str.substr(0, candidate_str.size() - 2);
        }

        vis_manager_g4_.reset();
        throw InvalidValueError(
            config_, "driver", "visualization driver does not exists (options are " + candidate_str + ")");
    }

    UI->ApplyCommand("/vis/sceneHandler/attach");
    UI->ApplyCommand("/vis/viewer/create");

    // release the g4 output
    RELEASE_STREAM(G4cout);

    // execute initialization macro if provided
    if(config_.has("macro_init")) {
        UI->ApplyCommand("/control/execute " + config_.getPath("macro_init"));
    }
}

void VisualizationGeant4Module::run(unsigned int) {
    // execute the run macro
    if(config_.has("macro_run")) {
        G4UImanager* UI = G4UImanager::GetUIpointer();
        UI->ApplyCommand("/control/execute " + config_.getPath("macro_run"));
    }
}

// display the visualization after all events have passed
void VisualizationGeant4Module::finalize() {
    // flush the view or open an interactive session depending on settings
    if(config_.has("use_gui")) {
        LOG(INFO) << "Starting visualization session";
        gui_session_->SessionStart();
    } else {
        if(config_.get("interactive", false)) {
            LOG(INFO) << "Starting terminal session";
            std::unique_ptr<G4UIsession> session = std::make_unique<G4UIterminal>();
            session->SessionStart();
        } else {
            LOG(INFO) << "Starting viewer";
            vis_manager_g4_->GetCurrentViewer()->ShowView();
        }
    }

    // set that we did succesfully visualize
    has_run_ = true;
}
