/**
 * @file
 * @brief Implementation of module to read doping concentration maps
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "DopingProfileReaderModule.hpp"

#include <string>
#include <utility>

#include "core/utils/log.h"

using namespace allpix;

DopingProfileReaderModule::DopingProfileReaderModule(Configuration& config, Messenger*, std::shared_ptr<Detector> detector)
    : Module(config, detector), detector_(std::move(detector)) {
    // Enable parallelization of this module if multithreading is enabled
    enable_parallelization();
}

void DopingProfileReaderModule::initialize() {
    FieldType type = FieldType::GRID;

    // Check field strength
    auto field_model = config_.get<std::string>("model");

    // set depth of doping profile
    auto model = detector_->getModel();
    auto doping_depth = config_.get<double>("doping_depth", model->getSensorSize().z());
    if(doping_depth - model->getSensorSize().z() > std::numeric_limits<double>::epsilon()) {
        throw InvalidValueError(config_, "doping_depth", "doping depth can not be larger than the sensor thickness");
    }
    auto sensor_max_z = model->getSensorCenter().z() + model->getSensorSize().z() / 2.0;
    auto thickness_domain = std::make_pair(sensor_max_z - doping_depth, sensor_max_z);

    // Calculate the field depending on the configuration
    if(field_model == "mesh") {
        // Read the field scales from the configuration, defaulting to 1.0x1.0 pixel cell:
        auto scales = config_.get<ROOT::Math::XYVector>("field_scale", {1.0, 1.0});
        // FIXME Add sanity checks for scales here
        LOG(DEBUG) << "Doping concentration map will be scaled with factors " << scales;
        std::array<double, 2> field_scale{{scales.x(), scales.y()}};

        // Get the field offset in fractions of the pixel pitch, default is 0.0x0.0, i.e. starting at pixel boundary:
        auto offset = config_.get<ROOT::Math::XYVector>("field_offset", {0.0, 0.0});
        if(offset.x() > 1.0 || offset.y() > 1.0) {
            throw InvalidValueError(
                config_,
                "field_offset",
                "shifting doping concentration map by more than one pixel (offset > 1.0) is not allowed");
        }
        LOG(DEBUG) << "Doping concentration map starts with offset " << offset << " to pixel boundary";
        std::array<double, 2> field_offset{{offset.x(), offset.y()}};

        auto field_data = read_field(field_scale);
        detector_->setDopingProfileGrid(
            field_data.getData(), field_data.getDimensions(), field_scale, field_offset, thickness_domain);

    } else if(field_model == "constant") {
        LOG(TRACE) << "Adding constant doping concentration";
        type = FieldType::CONSTANT;

        auto concentration = config_.get<double>("doping_concentration");
        LOG(INFO) << "Set constant doping concentration of " << Units::display(concentration, {"/cm/cm/cm"});
        FieldFunction<double> function = [concentration](const ROOT::Math::XYZPoint&) noexcept { return concentration; };

        detector_->setDopingProfileFunction(function, type);
    } else if(field_model == "regions") {
        LOG(TRACE) << "Adding doping concentration depending on sensor region";
        type = FieldType::CUSTOM;

        auto concentration = config_.getMatrix<double>("doping_concentration");
        std::map<double, double> concentration_map;
        for(const auto& region : concentration) {
            if(region.size() != 2) {
                throw InvalidValueError(
                    config_, "doping_concentration", "expecting two values per row, depth and concentration");
            }

            concentration_map[region.front()] = region.back();
            LOG(INFO) << "Set constant doping concentration of " << Units::display(region.back(), {"/cm/cm/cm"})
                      << " at sensor depth " << Units::display(region.front(), {"um", "mm"});
        }
        FieldFunction<double> function = [concentration_map, thickness = detector_->getModel()->getSensorSize().z()](
                                             const ROOT::Math::XYZPoint& position) {
            // Lower bound returns the first element that is *not less* than the given key - in this case, the z position
            // should always be *before* the region boundary set in the vector
            auto item = concentration_map.lower_bound(thickness / 2 - position.z());
            if(item != concentration_map.end()) {
                return item->second;
            } else {
                return concentration_map.rbegin()->second;
            }
        };

        detector_->setDopingProfileFunction(function, type);
    } else {
        throw InvalidValueError(config_, "model", "model should be 'constant', 'regions' or 'mesh'");
    }
}

/**
 * The field read from the INIT format are shared between module instantiations using the static FieldParser.
 */
FieldParser<double> DopingProfileReaderModule::field_parser_(FieldQuantity::SCALAR);
FieldData<double> DopingProfileReaderModule::read_field(std::array<double, 2> field_scale) {

    try {
        LOG(TRACE) << "Fetching doping concentration map from mesh file";

        // Get field from file
        auto field_data = field_parser_.getByFileName(config_.getPath("file_name", true), "/cm/cm/cm");

        // Check if electric field matches chip
        check_detector_match(field_data.getSize(), field_scale);

        LOG(INFO) << "Set doping concentration map with " << field_data.getDimensions().at(0) << "x"
                  << field_data.getDimensions().at(1) << "x" << field_data.getDimensions().at(2) << " cells";

        // Return the field data
        return field_data;
    } catch(std::invalid_argument& e) {
        throw InvalidValueError(config_, "file_name", e.what());
    } catch(std::runtime_error& e) {
        throw InvalidValueError(config_, "file_name", e.what());
    } catch(std::bad_alloc& e) {
        throw InvalidValueError(config_, "file_name", "file too large");
    }
}

/**
 * @brief Check if the detector matches the file header
 */
void DopingProfileReaderModule::check_detector_match(std::array<double, 3> dimensions, std::array<double, 2> field_scale) {
    auto xpixsz = dimensions[0];
    auto ypixsz = dimensions[1];
    auto thickness = dimensions[2];

    auto model = detector_->getModel();
    // Do a several checks with the detector model
    if(model != nullptr) {
        // Check field dimension in z versus the sensor thickness:
        if(std::fabs(thickness - model->getSensorSize().z()) > std::numeric_limits<double>::epsilon()) {
            LOG(WARNING) << "Thickness of doping concentration map is " << Units::display(thickness, "um")
                         << " but sensor thickness is " << Units::display(model->getSensorSize().z(), "um");
        }

        // Check the field extent along the pixel pitch in x and y:
        if(std::fabs(xpixsz - model->getPixelSize().x() * field_scale[0]) > std::numeric_limits<double>::epsilon() ||
           std::fabs(ypixsz - model->getPixelSize().y() * field_scale[1]) > std::numeric_limits<double>::epsilon()) {
            LOG(WARNING) << "Doping concentration map size is (" << Units::display(xpixsz, {"um", "mm"}) << ","
                         << Units::display(ypixsz, {"um", "mm"}) << ") but current configuration results in an map area of ("
                         << Units::display(model->getPixelSize().x() * field_scale[0], {"um", "mm"}) << ","
                         << Units::display(model->getPixelSize().y() * field_scale[1], {"um", "mm"}) << ")" << std::endl
                         << "The size of the area to which the doping concentration is applied can be changes using the "
                            "field_scale parameter.";
        }
    }
}
