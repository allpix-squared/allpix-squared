/**
 * @author Neal Gauvin <neal.gauvin@cern.ch>
 * @author John Idarraga <idarraga@cern.ch>
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "ReadGeoDescription.hpp"

#include <set>

#include "core/module/ModuleError.hpp"
#include "core/utils/log.h"

#include "core/config/ConfigReader.hpp"
#include "core/geometry/PixelDetectorModel.hpp"

#include "tools/ROOT.h"

// ROOT
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>

using namespace allpix;
using namespace ROOT::Math;

ReadGeoDescription::ReadGeoDescription(std::string file_name) : models_() {
    std::ifstream file(file_name);
    if(!file.good()) {
        throw ModuleError("Geometry description file '" + file_name + "' not found");
    }

    ConfigReader reader(file, file_name);

    for(auto& config : reader.getConfigurations()) {
        models_[config.getName()] = parse_config(config);
    }
}

std::shared_ptr<PixelDetectorModel> ReadGeoDescription::parse_config(const Configuration& config) {
    std::string model_name = config.getName();
    std::shared_ptr<PixelDetectorModel> model = std::make_shared<PixelDetectorModel>(model_name);

    // pixel amount
    if(config.has("pixel_amount")) {
        XYVector vec = config.get<XYVector>("pixel_amount");
        model->SetNPixelsX(static_cast<int>(std::round(vec.x())));
        model->SetNPixelsY(static_cast<int>(std::round(vec.y())));
    }
    // size, positions and offsets
    if(config.has("pixel_size")) {
        // FIXME: multiply by two to simulate old behaviour
        model->setPixelSize(2 * config.get<XYVector>("pixel_size"));
    }
    if(config.has("chip_size")) {
        // FIXME: multiply by two to simulate old behaviour
        model->setChipSize(2 * config.get<XYZVector>("chip_size"));
    }
    if(config.has("chip_offset")) {
        model->setChipOffset(config.get<XYZVector>("chip_offset"));
    }
    if(config.has("sensor_size")) {
        // FIXME: multiply by two to simulate old behaviour
        model->setSensorSize(2 * config.get<XYZVector>("sensor_size"));
    }
    if(config.has("sensor_offset")) {
        model->setSensorOffset(config.get<XYVector>("sensor_offset"));
    }
    if(config.has("pcb_size")) {
        // FIXME: multiply by two to simulate old behaviour
        model->setPCBSize(2 * config.get<XYZVector>("pcb_size"));
    }

    // excess for the guard rings
    if(config.has("sensor_gr_excess_htop")) {
        model->SetSensorExcessHTop(config.get<double>("sensor_gr_excess_htop"));
    }
    if(config.has("sensor_gr_excess_hbottom")) {
        model->SetSensorExcessHBottom(config.get<double>("sensor_gr_excess_hbottom"));
    }
    if(config.has("sensor_gr_excess_hleft")) {
        model->SetSensorExcessHLeft(config.get<double>("sensor_gr_excess_hleft"));
    }
    if(config.has("sensor_gr_excess_hright")) {
        model->SetSensorExcessHRight(config.get<double>("sensor_gr_excess_hright"));
    }

    // bump parameters
    if(config.has("bump_sphere_radius")) {
        model->SetBumpRadius(config.get<double>("bump_sphere_radius"));
    }
    if(config.has("bump_height")) {
        model->SetBumpHeight(config.get<double>("bump_height"));
    }
    if(config.has("bump_cylinder_radius")) {
        model->SetBumpDr(config.get<double>("bump_cylinder_radius"));
    }
    if(config.has("bump_offset")) {
        XYVector vec = config.get<XYVector>("bump_offset");
        model->SetBumpOffsetX(vec.x());
        model->SetBumpOffsetY(vec.y());
    }

    return model;
}

// Return detector model
// FIXME: should we throw an error if it does not exists
std::shared_ptr<PixelDetectorModel> ReadGeoDescription::getDetectorModel(const std::string& name) const {
    if(models_.find(name) == models_.end()) {
        return nullptr;
    }
    return models_.at(name);
}
