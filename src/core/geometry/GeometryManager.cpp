/**
 * @file
 * @brief Implementation of geometry manager
 *
 * @copyright Copyright (c) 2017-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <Math/RotationX.h>
#include <Math/RotationY.h>
#include <Math/RotationZ.h>
#include <Math/RotationZYX.h>
#include <Math/Vector3D.h>

#include "GeometryManager.hpp"
#include "core/config/ConfigReader.hpp"
#include "core/geometry/exceptions.h"
#include "core/module/exceptions.h"
#include "core/utils/distributions.h"
#include "core/utils/log.h"
#include "core/utils/unit.h"
#include "tools/ROOT.h"

using namespace allpix;
using namespace ROOT::Math;

GeometryManager::GeometryManager() : closed_{false} {}

/**
 * Loads the geometry by looping over all defined detectors
 */
void GeometryManager::load(ConfigManager* conf_manager, RandomNumberGenerator& seeder) {
    // Set up a random number generator and seed it with the global seed:
    random_generator_.seed(seeder());

    // Loop over all defined detectors
    LOG(DEBUG) << "Loading detectors";
    for(auto& geometry_section : conf_manager->getDetectorConfigurations()) {

        // Read role of this section and default to "active" (i.e. detector)
        auto role = geometry_section.get<std::string>("role", "active");
        std::transform(role.begin(), role.end(), role.begin(), ::tolower);
        if(role == "passive") {
            // Check for duplicate names:
            auto it = std::find_if(
                passive_elements_.begin(),
                passive_elements_.end(),
                [name = geometry_section.getName()](const Configuration& cfg) { return name == cfg.getName(); });
            if(it != passive_elements_.end()) {
                throw PassiveElementExistsError(geometry_section.getName());
            }

            passive_elements_.push_back(geometry_section);
            LOG(DEBUG) << "Passive element " << geometry_section.getName() << ", putting aside";
            continue;
        } else if(role != "active") {
            throw InvalidValueError(geometry_section, "role", "unknown role");
        }

        LOG(DEBUG) << "Detector " << geometry_section.getName() << ":";
        // Get the position and orientation of the detector
        auto [position, orientation] = calculate_orientation(geometry_section);

        // Create the detector and add it without model
        // NOTE: cannot use make_shared here due to the private constructor
        auto detector = std::shared_ptr<Detector>(new Detector(geometry_section.getName(), position, orientation));
        addDetector(detector);

        // Add a link to the detector to add the model later
        nonresolved_models_[geometry_section.get<std::string>("type")].emplace_back(geometry_section, detector.get());
    }

    // Calculate the orientations of passive elements
    for(auto& passive_element : passive_elements_) {
        passive_orientations_[passive_element.getName()] = calculate_orientation(passive_element);

        // Check for mandatory but hitherto unused keys:
        auto check_key = [&](const Configuration& cfg, const std::string& key) {
            if(!cfg.has(key)) {
                throw MissingKeyError(key, cfg.getName());
            }
        };

        // Check type keyword
        check_key(passive_element, "type");

        // Check material unless it's a GDML file placement
        auto type = passive_element.get<std::string>("type");
        std::transform(type.begin(), type.end(), type.begin(), ::tolower);
        if(type != "gdml") {
            check_key(passive_element, "material");
        }
    }

    // Load the list of standard model paths
    Configuration& global_config = conf_manager->getGlobalConfiguration();
    if(global_config.has("model_paths")) {
        auto extra_paths = global_config.getPathArray("model_paths", true);
        model_paths_.insert(model_paths_.end(), extra_paths.begin(), extra_paths.end());
        LOG(TRACE) << "Registered model paths from configuration.";
    }
    if(std::filesystem::is_directory(ALLPIX_MODEL_DIRECTORY)) {
        model_paths_.emplace_back(ALLPIX_MODEL_DIRECTORY);
        LOG(TRACE) << "Registered model path: " << ALLPIX_MODEL_DIRECTORY;
    }
    const char* data_dirs_env = std::getenv("XDG_DATA_DIRS");
    if(data_dirs_env == nullptr || strlen(data_dirs_env) == 0) {
        data_dirs_env = "/usr/local/share/:/usr/share/:";
    }
    auto data_dirs = split<std::filesystem::path>(data_dirs_env, ":");
    for(auto data_dir : data_dirs) {
        data_dir /= std::filesystem::path(ALLPIX_PROJECT_NAME) / std::string("models");
        if(std::filesystem::is_directory(data_dir)) {
            model_paths_.emplace_back(data_dir);
            LOG(TRACE) << "Registered global model path: " << data_dir;
        }
    }
    auto config_file_path = global_config.getFilePath();
    if(!config_file_path.empty() && std::filesystem::is_directory(config_file_path.parent_path())) {
        model_paths_.emplace_back(config_file_path.parent_path());
        LOG(TRACE) << "Registered path of configuration file as model location.";
    }
}

/**
 * Returns the pre-calculated position and orientation of a passive element in global coordinates
 */
std::pair<XYZPoint, Rotation3D> GeometryManager::getPassiveElementOrientation(const std::string& passive_element) const {
    if(passive_orientations_.count(passive_element) == 0) {
        throw ModuleError("Passive Material '" + passive_element + "' is not defined.");
    }
    return passive_orientations_.at(passive_element);
}

/**
 * The minimum coordinate is the location of the point where no part of any detector exist with a lower x, y or z-coordinate
 * in the geometry. The minimum point is never above the origin (the origin is always included in the geometry).
 */
ROOT::Math::XYZPoint GeometryManager::getMinimumCoordinate() {
    if(!closed_) {
        close_geometry();
    }

    ROOT::Math::XYZPoint min_point(0, 0, 0);
    // Loop through all detector
    for(auto& detector : detectors_) {
        // Get the model of the detector
        auto model = detector->getModel();

        std::array<int, 8> offset_x = {{1, 1, 1, 1, -1, -1, -1, -1}};
        std::array<int, 8> offset_y = {{1, 1, -1, -1, 1, 1, -1, -1}};
        std::array<int, 8> offset_z = {{1, -1, 1, -1, 1, -1, 1, -1}};

        for(size_t i = 0; i < 8; ++i) {
            auto point = model->getModelCenter();
            point.SetX(point.x() + offset_x.at(i) * model->getSize().x() / 2.0);
            point.SetY(point.y() + offset_y.at(i) * model->getSize().y() / 2.0);
            point.SetZ(point.z() + offset_z.at(i) * model->getSize().z() / 2.0);
            point = detector->getGlobalPosition(point);

            min_point.SetX(std::min(min_point.x(), point.x()));
            min_point.SetY(std::min(min_point.y(), point.y()));
            min_point.SetZ(std::min(min_point.z(), point.z()));
        }
    }

    // Loop through all separate points
    for(auto& point : points_) {
        min_point.SetX(std::min(min_point.x(), point.x()));
        min_point.SetY(std::min(min_point.y(), point.y()));
        min_point.SetZ(std::min(min_point.z(), point.z()));
    }

    return min_point;
}

/**
 * The maximum coordinate is the location of the point where no part of any detector exist with a higher x, y or z-coordinate
 * in the geometry. The maximum point is never below the origin (the origin is always included in the geometry).
 */
ROOT::Math::XYZPoint GeometryManager::getMaximumCoordinate() {
    if(!closed_) {
        close_geometry();
    }

    ROOT::Math::XYZPoint max_point(0, 0, 0);
    // Loop through all detector
    for(auto& detector : detectors_) {
        // Get the model of the detector
        auto model = detector->getModel();

        std::array<int, 8> offset_x = {{1, 1, 1, 1, -1, -1, -1, -1}};
        std::array<int, 8> offset_y = {{1, 1, -1, -1, 1, 1, -1, -1}};
        std::array<int, 8> offset_z = {{1, -1, 1, -1, 1, -1, 1, -1}};

        for(size_t i = 0; i < 8; ++i) {
            auto point = model->getModelCenter();
            point.SetX(point.x() + offset_x.at(i) * model->getSize().x() / 2.0);
            point.SetY(point.y() + offset_y.at(i) * model->getSize().y() / 2.0);
            point.SetZ(point.z() + offset_z.at(i) * model->getSize().z() / 2.0);
            point = detector->getGlobalPosition(point);
            max_point.SetX(std::max(max_point.x(), point.x()));
            max_point.SetY(std::max(max_point.y(), point.y()));
            max_point.SetZ(std::max(max_point.z(), point.z()));
        }
    }

    // Loop through all separate points
    for(auto& point : points_) {
        max_point.SetX(std::max(max_point.x(), point.x()));
        max_point.SetY(std::max(max_point.y(), point.y()));
        max_point.SetZ(std::max(max_point.z(), point.z()));
    }

    return max_point;
}

/**
 * @throws ModuleError If the geometry is already closed before calling this function
 *
 * Can be used to add an arbitrary and unspecified point which is part of the geometry
 */
void GeometryManager::addPoint(ROOT::Math::XYZPoint point) {
    if(closed_) {
        throw ModuleError("Geometry is already closed before adding detector");
    }
    points_.push_back(std::move(point));
}

/**
 * @throws InvalidModuleActionException If the passed detector is a null pointer
 * @throws ModuleError If the geometry is already closed before calling this function
 * @throws DetectorModelExistsError If the detector name is already registered before
 */
void GeometryManager::addModel(std::shared_ptr<DetectorModel> model) {
    if(closed_) {
        throw ModuleError("Geometry is already closed before adding detector");
    }
    if(model == nullptr) {
        throw InvalidModuleActionException("Added model cannot be a null pointer");
    }

    LOG(TRACE) << "Registering new model " << model->getType();
    if(model_names_.find(model->getType()) != model_names_.end()) {
        throw DetectorModelExistsError(model->getType());
    }

    model_names_.insert(model->getType());
    models_.push_back(std::move(model));
}

bool GeometryManager::needsModel(const std::string& name) const {
    return nonresolved_models_.find(name) != nonresolved_models_.end();
}
bool GeometryManager::hasModel(const std::string& name) const {
    return model_names_.find(name) != model_names_.end();
}

/**
 * @throws InvalidDetectorError If a model with this name does not exist
 */
std::shared_ptr<DetectorModel> GeometryManager::getModel(const std::string& name) const {
    // FIXME: this is not a very nice way to implement this
    for(const auto& model : models_) {
        if(model->getType() == name) {
            return model;
        }
    }
    throw allpix::InvalidDetectorModelError(name);
}

/**
 * @throws InvalidModuleActionException If the passed detector is a null pointer
 * @throws ModuleError If the geometry is already closed before calling this function
 * @throws DetectorInvalidNameError If the detector name is invalid
 * @throws DetectorExistsError If the detector name is already registered before
 */
void GeometryManager::addDetector(std::shared_ptr<Detector> detector) {
    if(closed_) {
        throw ModuleError("Geometry is already closed before adding detector");
    }
    if(detector == nullptr) {
        throw InvalidModuleActionException("Added detector cannot be a null pointer");
    }

    LOG(TRACE) << "Registering new detector " << detector->getName();

    // The name global is used for objects not assigned to any detector, hence it shouldn't be used as a detector name
    if(detector->getName() == "global") {
        throw DetectorInvalidNameError(detector->getName());
    }

    if(detector_names_.find(detector->getName()) != detector_names_.end()) {
        throw DetectorExistsError(detector->getName());
    }

    detector_names_.insert(detector->getName());
    detectors_.push_back(std::move(detector));
}

bool GeometryManager::hasDetector(const std::string& name) const {
    return detector_names_.find(name) != detector_names_.end();
}

std::vector<std::shared_ptr<Detector>> GeometryManager::getDetectors() {
    if(!closed_) {
        close_geometry();
    }

    return detectors_;
}
/**
 * @throws InvalidDetectorError If a detector with this name does not exist
 */
std::shared_ptr<Detector> GeometryManager::getDetector(const std::string& name) {
    if(!closed_) {
        close_geometry();
    }

    // FIXME: this is not a very nice way to implement this
    for(auto& detector : detectors_) {
        if(detector->getName() == name) {
            return detector;
        }
    }
    throw allpix::InvalidDetectorError(name);
}

/**
 * @throws InvalidDetectorError If not a single detector with this type exists
 */
std::vector<std::shared_ptr<Detector>> GeometryManager::getDetectorsByType(const std::string& type) {
    if(!closed_) {
        close_geometry();
    }

    std::vector<std::shared_ptr<Detector>> result;
    for(auto& detector : detectors_) {
        if(detector->getType() == type) {
            result.push_back(detector);
        }
    }
    if(result.empty()) {
        throw allpix::InvalidDetectorModelError(type);
    }

    return result;
}

void GeometryManager::load_models() {
    LOG(TRACE) << "Loading remaining default models";

    // Get paths to read models from
    std::vector<std::string> paths = getModelsPath();

    std::vector<std::pair<std::string, ConfigReader>> readers;
    LOG(TRACE) << "Reading model files";
    // Add all the paths to the reader
    for(auto& path : paths) {
        // Check if file or directory
        if(std::filesystem::is_directory(path)) {
            for(const auto& entry : std::filesystem::directory_iterator(path)) {
                if(!entry.is_regular_file()) {
                    continue;
                }

                // Accept only with correct model suffix
                auto sub_path = std::filesystem::canonical(entry);
                std::string suffix(ALLPIX_MODEL_SUFFIX);
                if(sub_path.extension() != suffix) {
                    continue;
                }

                // Read model file and add model to list
                read_model_file(sub_path);
            }
        } else {
            // Always a file because paths are already checked
            read_model_file(path);
        }
    }
}

void GeometryManager::read_model_file(const std::filesystem::path& path) {
    auto model_name = path.stem();
    LOG(TRACE) << "Reading model " << model_name << " in path " << path;

    // Check if we need to look at file at all
    if(hasModel(model_name)) {
        LOG(DEBUG) << "Skipping overwritten model " << model_name << " in path " << path;
        return;
    }
    if(!needsModel(model_name)) {
        LOG(TRACE) << "Skipping not required model " << model_name << " in path " << path;
        return;
    }

    try {
        // Try to parse as config file
        std::ifstream file(path);
        ConfigReader reader(file, path);

        // Parse configuration and add model to the config
        addModel(DetectorModel::factory(model_name, reader));

    } catch(const ConfigParseError& e) {
        // Not a valid config file, see https://gitlab.cern.ch/allpix-squared/allpix-squared/-/issues/277
        LOG(ERROR) << "Skipping invalid model file " << path << ":" << std::endl << e.what();
    }
}

/**
 * After closing the geometry new parts of the geometry cannot be added anymore. All the models for the detectors in the
 * configuration are resolved to requested type (and an error is thrown if this is not possible). Also if a parameter is
 * specialized in the detector config a copy of the model is created with those specialized settings.
 */
void GeometryManager::close_geometry() {
    LOG(TRACE) << "Starting geometry closing procedure";

    // Load all standard models
    load_models();

    // Try to resolve the missing models
    for(auto& [name, config_detectors] : nonresolved_models_) {
        for(auto& [config, detector] : config_detectors) {
            // Create a new model if one of the core model parameters is changed in the detector configuration
            auto model = getModel(name);

            // Get the configuration of the model
            Configuration new_config("");
            auto model_configs = model->getConfigurations();

            // Add all non internal parameters to the config for a specialized model
            for(auto& [key, value] : config.getAll()) {
                // Skip all internal parameters
                if(key == "type" || key == "position" || key == "orientation_mode" || key == "orientation") {
                    continue;
                }
                // Add the extra parameter to the new overwritten config
                new_config.setText(key, value);
            }

            // Create new model if needed
            if(new_config.countSettings() != 0) {
                ConfigReader reader;
                // Add the new configuration first to overwrite
                reader.addConfiguration(std::move(new_config));
                // Then add the original configuration
                for(auto& model_config : model_configs) {
                    reader.addConfiguration(std::move(model_config));
                }

                model = DetectorModel::factory(name, reader);
            }

            detector->set_model(model);
        }
    }

    closed_ = true;
    LOG(TRACE) << "Closed geometry";
}
/**
 * Calculates the position and orientation of the object from the provided configuration file
 */
std::pair<XYZPoint, Rotation3D> GeometryManager::calculate_orientation(const Configuration& config) {

    // Calculate possible detector misalignment to be added
    auto misalignment = [&](auto residuals) {
        double dx = allpix::normal_distribution<double>(0, residuals.x())(random_generator_);
        double dy = allpix::normal_distribution<double>(0, residuals.y())(random_generator_);
        double dz = allpix::normal_distribution<double>(0, residuals.z())(random_generator_);
        return DisplacementVector3D<Cartesian3D<double>>(dx, dy, dz);
    };

    // Get the position and apply potential misalignment
    auto position = config.get<XYZPoint>("position");
    LOG(DEBUG) << "Position:    " << Units::display(position, {"mm", "um"});
    position += misalignment(config.get<XYZPoint>("alignment_precision_position", XYZPoint()));
    LOG(DEBUG) << " misaligned: " << Units::display(position, {"mm", "um"});

    // Get the orientation and apply misalignment to the individual angles before combining them
    auto orient_vec = config.get<XYZVector>("orientation");
    LOG(DEBUG) << "Orientation: " << Units::display(orient_vec, {"deg"});
    orient_vec += misalignment(config.get<XYZVector>("alignment_precision_orientation", XYZVector()));
    LOG(DEBUG) << " misaligned: " << Units::display(orient_vec, {"deg"});

    auto orientation_mode = config.get<std::string>("orientation_mode", "xyz");
    Rotation3D orientation;
    if(orientation_mode == "zyx") {
        // First angle given in the configuration file is around z, second around y, last around x:
        LOG(DEBUG) << "Interpreting Euler angles as ZYX rotation";
        orientation = RotationZYX(orient_vec.x(), orient_vec.y(), orient_vec.z());
    } else if(orientation_mode == "xyz") {
        LOG(DEBUG) << "Interpreting Euler angles as XYZ rotation";
        // First angle given in the configuration file is around x, second around y, last around z:
        orientation = RotationZ(orient_vec.z()) * RotationY(orient_vec.y()) * RotationX(orient_vec.x());
    } else if(orientation_mode == "zxz") {
        LOG(DEBUG) << "Interpreting Euler angles as ZXZ rotation";
        // First angle given in the configuration file is around z, second around x, last around z:
        orientation = EulerAngles(orient_vec.x(), orient_vec.y(), orient_vec.z());
    } else {
        throw InvalidValueError(config, "orientation_mode", "orientation_mode should be either 'zyx', xyz' or 'zxz'");
    }
    return std::pair<XYZPoint, Rotation3D>(position, orientation);
}

bool GeometryManager::hasMagneticField() const {
    return (magnetic_field_type_ != MagneticFieldType::NONE);
}

void GeometryManager::setMagneticFieldFunction(MagneticFieldFunction function, MagneticFieldType type) {
    magnetic_field_function_ = std::move(function);
    magnetic_field_type_ = type;
}

MagneticFieldType GeometryManager::getMagneticFieldType() const {
    return magnetic_field_type_;
}

ROOT::Math::XYZVector GeometryManager::getMagneticField(const ROOT::Math::XYZPoint& position) const {
    return magnetic_field_function_(position);
}
