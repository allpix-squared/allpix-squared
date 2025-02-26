/**
 * @file
 * @brief Implementation of Geant4 geometry visualization module
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "VisualizationGeant4Module.hpp"

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#ifdef G4UI_USE_QT
#include <QCoreApplication>
#endif

#include <G4LogicalVolume.hh>
#ifdef G4UI_USE_QT
#include <G4UIQt.hh>
#endif
#include <G4PVParameterised.hh>
#include <G4RunManager.hh>
#include <G4UImanager.hh>
#include <G4UIsession.hh>
#include <G4UItcsh.hh>
#include <G4UIterminal.hh>
#include <G4VPVParameterisation.hh>
#include <G4VisAttributes.hh>
#include <G4VisExecutive.hh>

#include "core/config/exceptions.h"
#include "core/utils/log.h"
#include "tools/geant4/G4LoggingDestination.hpp"

using namespace allpix;

VisualizationGeant4Module::VisualizationGeant4Module(Configuration& config, Messenger*, GeometryManager* geo_manager)
    : Module(config), geo_manager_(geo_manager) {
    // Interpret transparency parameter as opacity for backwards-compatibility
    config_.setAlias("opacity", "transparency", true);

    // Set default mode and driver for display
    config_.setDefault("mode", "gui");
    config_.setDefault("driver", "OGL");

    // Set to accumulate all hits and display at the end by default
    config_.setDefault("accumulate", true);
    config_.setDefault("simple_view", true);

    mode_ = config_.get<ViewingMode>("mode");
}
/**
 * Without applying this workaround the visualization (sometimes without content) is also shown when an exception occurred in
 * any other module.
 */
VisualizationGeant4Module::~VisualizationGeant4Module() {
    // Fetch the driver (ignoring errors)
    std::string driver;
    try {
        driver = config_.get<std::string>("driver", "");
    } catch(ConfigurationError& e) {
        driver = "";
    }

    // Invoke VRML2FILE workaround if necessary to prevent visualisation in case of exceptions
    if(!has_run_ && vis_manager_g4_ != nullptr && vis_manager_g4_->GetCurrentViewer() != nullptr && driver == "VRML2FILE") {
        LOG(TRACE) << "Invoking VRML workaround to prevent visualization under error conditions";

        auto* str = getenv("G4VRMLFILE_VIEWER");
        if(str != nullptr) {
            setenv("G4VRMLFILE_VIEWER", "NONE", 1);
        }
        vis_manager_g4_->GetCurrentViewer()->ShowView();
        if(str != nullptr) {
            setenv("G4VRMLFILE_VIEWER", str, 1);
        }
    }
}

void VisualizationGeant4Module::initialize() {

    // Check if we have a running G4 manager
    G4RunManager* run_manager_g4 = G4RunManager::GetRunManager();
    if(run_manager_g4 == nullptr) {
        throw ModuleError("Cannot visualize using Geant4 without a Geant4 geometry builder");
    }

    // Create the gui if required
    if(mode_ == ViewingMode::GUI) {
        // Need to provide parameters, simulate this behaviour
        session_param_ = ALLPIX_PROJECT_NAME;
        session_param_ptr_ = session_param_.data();

#ifdef G4UI_USE_QT
        gui_session_ = std::make_unique<G4UIQt>(1, &session_param_ptr_);
#else
        throw InvalidValueError(config_, "mode", "GUI session cannot be started because Qt is not available in this Geant4");
#endif
    }

    // Get the UI commander
    G4UImanager* UI = G4UImanager::GetUIpointer();

    // Disable auto refresh while we are simulating and building
    UI->ApplyCommand("/vis/viewer/set/autoRefresh false");

    // Set the visibility attributes for visualization
    set_visualization_attributes();

    // Initialize the session and the visualization manager
    LOG(TRACE) << "Initializing visualization";
    vis_manager_g4_ = std::make_unique<G4VisExecutive>("quiet");
    vis_manager_g4_->Initialize();

    // Create the viewer
    UI->ApplyCommand("/vis/scene/create");

    // Initialize the driver and checking it actually exists
    int check_driver = UI->ApplyCommand("/vis/sceneHandler/create " + config_.get<std::string>("driver"));
    if(check_driver != 0) {
        std::set<G4String> candidates;
        for(auto* system : vis_manager_g4_->GetAvailableGraphicsSystems()) {
            for(const auto& nickname : system->GetNicknames()) {
                if(nickname.find("FALLBACK") == std::string::npos) {
                    candidates.insert(nickname);
                }
            }
        }

        std::string candidate_str;
        for(const auto& candidate : candidates) {
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

    // Set default visualization settings
    set_visualization_settings();

    // Reset the default displayListLimit
    auto display_limit = config_.get<std::string>("display_limit", "1000000");
    UI->ApplyCommand("/vis/ogl/set/displayListLimit " + display_limit);

    // Execute initialization macro if provided
    if(config_.has("macro_init")) {
        UI->ApplyCommand("/control/execute " + config_.getPath("macro_init", true).string());
    }

    // Force logging through our framework again since it seems to be reset during initialization:
    UI->SetCoutDestination(G4LoggingDestination::getInstance());
}

/**
 * Visualization settings are converted from internal configuration to Geant4 macro syntax
 */
void VisualizationGeant4Module::set_visualization_settings() {
    // Get the UI commander
    G4UImanager* UI = G4UImanager::GetUIpointer();

    // Set the background to white
    auto bkg_color = config_.get<std::string>("background_color", "white");
    auto ret_code = UI->ApplyCommand("/vis/viewer/set/background " + bkg_color);
    if(ret_code != 0) {
        throw InvalidValueError(config_, "background_color", "background color not defined");
    }

    // Accumulate all events if requested
    auto accumulate = config_.get<bool>("accumulate");
    if(accumulate) {
        UI->ApplyCommand("/vis/scene/endOfEventAction accumulate");
        UI->ApplyCommand("/vis/scene/endOfRunAction accumulate");
    } else {
        UI->ApplyCommand("/vis/scene/endOfEventAction refresh");
        UI->ApplyCommand("/vis/scene/endOfRunAction refresh");
    }

    // Display trajectories if specified
    auto display_trajectories = config_.get<bool>("display_trajectories", true);
    if(display_trajectories) {
        // Add smooth trajectories
        UI->ApplyCommand("/vis/scene/add/trajectories smooth rich");

        // Store trajectories if accumulating
        if(accumulate) {
            UI->ApplyCommand("/tracking/storeTrajectory 2");
        }

        // Hide trajectories inside the detectors
        auto hide_trajectories = config_.get<bool>("hidden_trajectories", true);
        if(hide_trajectories) {
            UI->ApplyCommand("/vis/viewer/set/hiddenEdge 1");
            UI->ApplyCommand("/vis/viewer/set/hiddenMarker 1");
        }

        // color trajectories by charge or particle id
        auto traj_color = config_.get<ColorMode>("trajectories_color_mode", ColorMode::CHARGE);
        if(traj_color == ColorMode::GENERIC) {
            UI->ApplyCommand("/vis/modeling/trajectories/create/generic allpixModule");

            UI->ApplyCommand("/vis/modeling/trajectories/allpixModule/default/setLineColor " +
                             config_.get<std::string>("trajectories_color", "blue"));
        } else if(traj_color == ColorMode::CHARGE) {
            // Create draw by charge
            UI->ApplyCommand("/vis/modeling/trajectories/create/drawByCharge allpixModule");

            // Set colors for charges
            ret_code = UI->ApplyCommand("/vis/modeling/trajectories/allpixModule/set 1 " +
                                        config_.get<std::string>("trajectories_color_positive", "blue"));
            if(ret_code != 0) {
                throw InvalidValueError(config_, "trajectories_color_positive", "charge color not defined");
            }
            UI->ApplyCommand("/vis/modeling/trajectories/allpixModule/set 0 " +
                             config_.get<std::string>("trajectories_color_neutral", "green"));
            if(ret_code != 0) {
                throw InvalidValueError(config_, "trajectories_color_neutral", "charge color not defined");
            }
            UI->ApplyCommand("/vis/modeling/trajectories/allpixModule/set -1 " +
                             config_.get<std::string>("trajectories_color_negative", "red"));
            if(ret_code != 0) {
                throw InvalidValueError(config_, "trajectories_color_negative", "charge color not defined");
            }
        } else if(traj_color == ColorMode::PARTICLE) {
            UI->ApplyCommand("/vis/modeling/trajectories/create/drawByParticleID allpixModule");

            auto particle_colors = config_.getArray<std::string>("trajectories_particle_colors");
            for(auto& particle_color : particle_colors) {
                ret_code = UI->ApplyCommand("/vis/modeling/trajectories/allpixModule/set " + particle_color);
                if(ret_code != 0) {
                    throw InvalidValueError(
                        config_, "trajectories_particle_colors", "combination particle type and color not valid");
                }
            }
        }

        // Set default settings for steps
        auto draw_steps = config_.get<bool>("trajectories_draw_step", true);
        if(draw_steps) {
            UI->ApplyCommand("/vis/modeling/trajectories/allpixModule/default/setDrawStepPts true");
            ret_code = UI->ApplyCommand("/vis/modeling/trajectories/allpixModule/default/setStepPtsSize " +
                                        config_.get<std::string>("trajectories_draw_step_size", "2"));
            if(ret_code != 0) {
                throw InvalidValueError(config_, "trajectories_draw_step_size", "step size not valid");
            }
            ret_code = UI->ApplyCommand("/vis/modeling/trajectories/allpixModule/default/setStepPtsColour " +
                                        config_.get<std::string>("trajectories_draw_step_color", "red"));
            if(ret_code != 0) {
                throw InvalidValueError(config_, "trajectories_draw_step_color", "step color not defined");
            }
        }
    }

    // Display hits if specified
    auto display_hits = config_.get<bool>("display_hits", false);
    if(display_hits) {
        UI->ApplyCommand("/vis/scene/add/hits");
    }

    // Set viewer style
    auto view_style = config_.get<std::string>("view_style", "surface");
    ret_code = UI->ApplyCommand("/vis/viewer/set/style " + view_style);
    if(ret_code != 0) {
        throw InvalidValueError(config_, "view_style", "view style is not defined");
    }

    // Set default viewer orientation
    auto viewpoint_angles =
        config_.getArray<double>("viewpoint_thetaphi", {Units::get<double>(-70, "deg"), Units::get<double>(20, "deg")});
    if(viewpoint_angles.size() != 2) {
        LOG(FATAL)
            << "Parameter viewpoint_thetaphi VisualizationGeant4Module is not valid. Must be two angles (theta, phi).";
        throw InvalidValueError(config_, "viewpoint_thetaphi", "invalid number of parameters given, must be two");
    }
    auto viewpoint_cmd = "/vis/viewer/set/viewpointThetaPhi " + std::to_string(Units::convert(viewpoint_angles[0], "deg")) +
                         " " + std::to_string(Units::convert(viewpoint_angles[1], "deg"));
    UI->ApplyCommand(viewpoint_cmd);

    // Do auto refresh if not accumulating and start viewer already
    if(!accumulate) {
        UI->ApplyCommand("/vis/viewer/set/autoRefresh true");
    }

    // Number of line segments to approximate a circle with; used to visualize radial detectors with more precision
    auto line_segments = config_.get<std::string>("line_segments", "250");
    UI->ApplyCommand("/vis/viewer/set/lineSegmentsPerCircle " + line_segments);
}

/**
 * The default colors and visibility are as follows
 * - Wrapper: Red (Invisible)
 * - Support: Greenish
 * - Chip: Blackish
 * - Bumps: Grey
 * - Sensor: Blackish
 */
void VisualizationGeant4Module::set_visualization_attributes() {
    // To add some opacity in the solids, set to 0.4. 1 means fully opaque.
    // Opacity can be switched off in the visualisation.
    auto alpha = config_.get<double>("opacity", 0.4);
    if(alpha <= 0 || alpha > 1) {
        throw InvalidValueError(config_, "opacity", "opacity level should be between 0 and 1");
    }

    // Wrapper
    auto wrapperVisAtt = G4VisAttributes(G4Color(1, 0, 0, 0.1)); // Red
    wrapperVisAtt.SetVisibility(false);

    // Support
    auto supportColor = G4Color(0.36, 0.66, 0.055, alpha); // Greenish
    auto supportVisAtt = G4VisAttributes(supportColor);
    supportVisAtt.SetLineWidth(1);
    supportVisAtt.SetForceSolid(false);

    // Chip
    auto chipColor = G4Color(0.18, 0.2, 0.21, alpha); // Blackish
    auto ChipVisAtt = G4VisAttributes(chipColor);
    ChipVisAtt.SetForceSolid(false);

    // Bumps
    auto bumpColor = G4Color(0.5, 0.5, 0.5, alpha); // Grey
    auto BumpVisAtt = G4VisAttributes(bumpColor);
    BumpVisAtt.SetForceSolid(false);

    // The logical volume holding all the bumps
    auto BumpBoxVisAtt = G4VisAttributes(bumpColor);
    BumpBoxVisAtt.SetForceSolid(false);

    // Sensors, ie pixels
    auto sensorColor = G4Color(0.18, 0.2, 0.21, alpha); // Blackish
    auto SensorVisAtt = G4VisAttributes(sensorColor);
    SensorVisAtt.SetForceSolid(false);

    // Passive Materials
    auto passivematerialColor = G4Color(0., 0., 1, alpha); // Blue
    auto PassiveMaterialVisAtt = G4VisAttributes(passivematerialColor);
    PassiveMaterialVisAtt.SetLineWidth(1);
    PassiveMaterialVisAtt.SetForceSolid(false);

    // The box holding all the pixels
    auto BoxVisAtt = G4VisAttributes(sensorColor);
    BoxVisAtt.SetForceSolid(false);

    // In default simple view mode, pixels and bumps are set to invisible, not to be displayed.
    // The logical volumes holding them are instead displayed.
    auto simple_view = config_.get<bool>("simple_view");
    if(simple_view) {
        SensorVisAtt.SetVisibility(false);
        BoxVisAtt.SetVisibility(true);
        BumpVisAtt.SetVisibility(false);
        BumpBoxVisAtt.SetVisibility(true);
    } else {
        SensorVisAtt.SetVisibility(true);
        BoxVisAtt.SetVisibility(true);
        BumpVisAtt.SetVisibility(true);
        BumpBoxVisAtt.SetVisibility(false);
    }

    // Apply the visualization attributes to all elements that exist
    for(const auto& name : geo_manager_->getExternalObjectNames()) {

        auto set_vis_attribute = [this, name](const std::string& volume, const G4VisAttributes& attr) {
            auto log = geo_manager_->getExternalObject<G4LogicalVolume>(name, volume);
            // Only set attributes if object exists and it does not yet have attributes:
            if(log != nullptr && log->GetVisAttributes() == nullptr) {
                log->SetVisAttributes(attr);
            }
        };

        set_vis_attribute("wrapper_log", wrapperVisAtt);
        set_vis_attribute("sensor_log", BoxVisAtt);
        set_vis_attribute("pixel_log", SensorVisAtt);
        set_vis_attribute("bumps_wrapper_log", BumpBoxVisAtt);
        set_vis_attribute("bumps_cell_log", BumpVisAtt);
        set_vis_attribute("chip_log", ChipVisAtt);
        set_vis_attribute("passive_material_log", PassiveMaterialVisAtt);

        auto supports_log =
            geo_manager_->getExternalObject<std::vector<std::shared_ptr<G4LogicalVolume>>>(name, "supports_log");
        if(supports_log != nullptr) {
            for(auto& support_log : *supports_log) {
                support_log->SetVisAttributes(supportVisAtt);
            }
        }
    }
}

void VisualizationGeant4Module::run(Event*) {
    if(!config_.get<bool>("accumulate")) {
        vis_manager_g4_->GetCurrentViewer()->ShowView();
        std::this_thread::sleep_for(
            std::chrono::nanoseconds(config_.get<unsigned long>("accumulate_time_step", Units::get(100ul, "ms"))));
    }
}

static bool has_gui = false;
static void (*prev_handler)(int) = nullptr;
/**
 * @brief Override interrupt handling to close the Qt application in GUI mode
 */
static void interrupt_handler(int signal) {
// Exit the Qt application if it is used
// FIXME: Is there a better way to trigger this?
#ifdef G4UI_USE_QT
    if(has_gui) {
        QCoreApplication::exit();
    }
#endif

    std::signal(SIGINT, prev_handler);
    std::raise(signal);
}

void VisualizationGeant4Module::add_visualization_volumes() {
    // Only place the pixel matrix for the visualization if we have no simple view
    if(!config_.get<bool>("simple_view")) {
        // Loop through detectors
        for(auto& detector : geo_manager_->getDetectors()) {
            auto sensor_log = geo_manager_->getExternalObject<G4LogicalVolume>(detector->getName(), "sensor_log");
            auto pixel_log = geo_manager_->getExternalObject<G4LogicalVolume>(detector->getName(), "pixel_log");
            auto pixel_param = geo_manager_->getExternalObject<G4VPVParameterisation>(detector->getName(), "pixel_param");

            // Continue if a required external object is missing
            if(sensor_log == nullptr || pixel_log == nullptr || pixel_param == nullptr) {
                continue;
            }

            // Place the pixels if all objects are available
            std::shared_ptr<G4PVParameterised> pixel_param_phys = std::make_shared<G4PVParameterised>(
                "pixel_" + detector->getName() + "_param",
                pixel_log.get(),
                sensor_log.get(),
                kUndefined,
                detector->getModel()->getNPixels().x() * detector->getModel()->getNPixels().y(),
                pixel_param.get(),
                false);
            geo_manager_->setExternalObject(detector->getName(), "pixel_param_phys", pixel_param_phys);
        }
    }
}

void VisualizationGeant4Module::finalize() {
    // Add volumes that are only used in the visualization
    add_visualization_volumes();

    // Enable automatic refresh before showing view
    G4UImanager* UI = G4UImanager::GetUIpointer();
    UI->ApplyCommand("/vis/viewer/set/autoRefresh true");

    // Set new signal handler to fetch CTRL+C and close the Qt application
    prev_handler = std::signal(SIGINT, interrupt_handler);

    // Open GUI / terminal or start viewer depending on mode
    if(mode_ == ViewingMode::GUI && gui_session_ != nullptr) {
        LOG(INFO) << "Starting visualization session";
        has_gui = true;
        gui_session_->SessionStart();
    } else if(mode_ == ViewingMode::TERMINAL) {
        LOG(INFO) << "Starting terminal session";
        Log::finish();
        std::unique_ptr<G4UIsession> session = std::make_unique<G4UIterminal>(new G4UItcsh);
        session->SessionStart();
    } else {
        LOG(INFO) << "Starting viewer";
        vis_manager_g4_->GetCurrentViewer()->ShowView();
    }

    // Set that we did successfully visualize
    has_run_ = true;
}
