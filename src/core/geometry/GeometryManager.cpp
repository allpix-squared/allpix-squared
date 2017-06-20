/**
 * @file
 * @brief Implementation of geometry manager
 *
 * @copyright MIT License
 */

#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <Math/EulerAngles.h>
#include <Math/Vector3D.h>

#include "GeometryManager.hpp"
#include "core/module/exceptions.h"
#include "core/utils/file.h"
#include "core/utils/log.h"
#include "exceptions.h"

// FIXME: Do we allow including tools in the core
#include "tools/ROOT.h"

using namespace allpix;

GeometryManager::GeometryManager() : closed_{false} {}

/**
 * Loads the geometry by parsing the configuration file
 */
void GeometryManager::load(const Configuration& global_config) {
    LOG(TRACE) << "Reading geometry";

    std::string detector_file_name = global_config.getPath("detectors_file", true);
    std::fstream file(detector_file_name);
    ConfigReader reader(file, detector_file_name);

    // Loop over all defined detectors
    for(auto& detector_section : reader.getConfigurations()) {
        // Get the position and orientation
        auto position = detector_section.get<ROOT::Math::XYZPoint>("position", ROOT::Math::XYZPoint());
        auto orientation = detector_section.get<ROOT::Math::EulerAngles>("orientation", ROOT::Math::EulerAngles());

        // Create the detector and add it without model
        // NOTE: cannot use make_shared here due to the private constructor
        auto detector = std::shared_ptr<Detector>(new Detector(detector_section.getName(), position, orientation));
        addDetector(detector);

        // Add a link to the detector to add the model later
        nonresolved_models_[detector_section.get<std::string>("type")].push_back(detector.get());
    }

    // Load the list of standard model paths
    if(global_config.has("model_paths")) {
        auto extra_paths = global_config.getPathArray("model_paths", true);
        model_paths_.insert(model_paths_.end(), extra_paths.begin(), extra_paths.end());
    }
    if(path_is_directory(ALLPIX_MODEL_DIRECTORY)) {
        model_paths_.emplace_back(ALLPIX_MODEL_DIRECTORY);
    }
    const char* data_dirs_env = std::getenv("XDG_DATA_DIRS");
    if(data_dirs_env == nullptr || strlen(data_dirs_env) == 0) {
        data_dirs_env = "/usr/local/share/:/usr/share/:";
    }
    std::vector<std::string> data_dirs = split<std::string>(data_dirs_env, ":");
    for(auto data_dir : data_dirs) {
        if(data_dir.back() != '/') {
            data_dir += "/";
        }

        data_dir += ALLPIX_PROJECT_NAME;
        if(path_is_directory(data_dir)) {
            model_paths_.emplace_back(data_dir);
        }
    }
}

/**
 * The default list of models to search for are in the following order
 * - The list of paths provided in the main configuration as models_path
 * - The build variable ALLPIX_MODEL_DIR pointing to the installation directory of the framework models
 * - The directories in XDG_DATA_DIRS with ALLPIX_PROJECT_NAME attached or /usr/share/:/usr/local/share if not defined
 */
std::vector<std::string> GeometryManager::getModelsPath() {
    return model_paths_;
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
            auto point = model->getCenter();
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
            auto point = model->getCenter();
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

std::vector<std::shared_ptr<DetectorModel>> GeometryManager::getModels() const {
    return models_;
}
/**
 * @throws InvalidDetectorError If a model with this name does not exist
 */
std::shared_ptr<DetectorModel> GeometryManager::getModel(const std::string& name) const {
    // FIXME: this is not a very nice way to implement this
    for(auto& model : models_) {
        if(model->getType() == name) {
            return model;
        }
    }
    throw allpix::InvalidModelError(name);
}

/**
 * @throws InvalidModuleActionException If the passed detector is a null pointer
 * @throws ModuleError If the geometry is already closed before calling this function
 * @throws DetectorNameExistsError If the detector name is already registered before
 */
void GeometryManager::addDetector(std::shared_ptr<Detector> detector) {
    if(closed_) {
        throw ModuleError("Geometry is already closed before adding detector");
    }
    if(detector == nullptr) {
        throw InvalidModuleActionException("Added detector cannot be a null pointer");
    }

    LOG(TRACE) << "Registering new detector " << detector->getName();
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
        throw allpix::InvalidModelError(type);
    }

    return result;
}

/*
 * After closing the geometry new parts of the geometry cannot be added anymore. All the models for the detectors in the
 * configuration are resolved to requested type (and an error is thrown if this is not possible)
 */
void GeometryManager::close_geometry() {
    LOG(TRACE) << "Geometry is closed";

    closed_ = true;

    // Try to resolve the missing models
    for(auto& detectors_types : nonresolved_models_) {
        for(auto& detector : detectors_types.second) {
            detector->set_model(getModel(detectors_types.first));
        }
    }
}
