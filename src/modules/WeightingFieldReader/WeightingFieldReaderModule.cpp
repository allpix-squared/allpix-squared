/**
 * @file
 * @brief Implementation of module to read weighting fields
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied
 * verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities
 * granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "WeightingFieldReaderModule.hpp"

#include <fstream>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>

#include <Math/Vector3D.h>
#include <TH2F.h>

#include "core/config/exceptions.h"
#include "core/geometry/DetectorModel.hpp"
#include "core/utils/log.h"
#include "core/utils/unit.h"

using namespace allpix;

WeightingFieldReaderModule::WeightingFieldReaderModule(Configuration& config, Messenger*, std::shared_ptr<Detector> detector)
    : Module(config, detector), detector_(std::move(detector)) {
    using namespace ROOT::Math;
    config_.setDefault("field_size", DisplacementVector2D<Cartesian2D<int>>(3, 3));
}

void WeightingFieldReaderModule::init() {

    auto field_model = config_.get<std::string>("model");

    // Calculate thickness domain
    auto model = detector_->getModel();
    auto sensor_max_z = model->getSensorCenter().z() + model->getSensorSize().z() / 2.0;
    auto thickness_domain = std::make_pair(sensor_max_z - model->getSensorSize().z(), sensor_max_z);

    // Calculate the field depending on the configuration
    if(field_model == "init") {
        WeightingFieldReaderModule::FieldData field_data;
        field_data = read_init_field();
        detector_->setWeightingFieldGrid(field_data.first, field_data.second, thickness_domain);
    } else {
        throw InvalidValueError(config_, "model", "model should be 'init'");
    }
}

/**
 * The field read from the INIT format are shared between module instantiations
 * using the static WeightingFieldReaderModule::get_by_file_name method.
 */
WeightingFieldReaderModule::FieldData WeightingFieldReaderModule::read_init_field() {
    using namespace ROOT::Math;

    try {
        LOG(TRACE) << "Fetching weighting field from init file";

        auto field_size = config_.get<DisplacementVector2D<Cartesian2D<int>>>("field_size");

        // Get field from file
        auto field_data = get_by_file_name(config_.getPath("file_name", true), *detector_.get(), field_size);
        LOG(INFO) << "Set weighting field with " << field_data.second.at(0) << "x" << field_data.second.at(1) << "x"
                  << field_data.second.at(2) << " cells";

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
inline static void check_detector_match(Detector& detector,
                                        double thickness,
                                        double xpixsz,
                                        double ypixsz,
                                        ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>> field_size) {
    auto model = detector.getModel();
    // Do a several checks with the detector model
    if(model != nullptr) {
        if(std::fabs(thickness - model->getSensorSize().z()) > std::numeric_limits<double>::epsilon()) {
            LOG(WARNING) << "Thickness of sensor in file is " << Units::display(thickness, "um")
                         << " but in the model it is " << Units::display(model->getSensorSize().z(), "um");
        }

        // Check that the total field size is n*pitch:
        if(std::fabs(xpixsz - model->getPixelSize().x() * field_size.x()) > std::numeric_limits<double>::epsilon() ||
           std::fabs(ypixsz - model->getPixelSize().y() * field_size.y()) > std::numeric_limits<double>::epsilon()) {
            LOG(WARNING) << "Field size is (" << Units::display(xpixsz, {"um", "mm"}) << ","
                         << Units::display(ypixsz, {"um", "mm"}) << ") but expecting (" << field_size.x() << " * "
                         << Units::display(model->getPixelSize().x(), {"um", "mm"}) << ", " << field_size.y() << " * "
                         << Units::display(model->getPixelSize().y(), {"um", "mm"}) << ")";
        }
    }
}

std::map<std::string, WeightingFieldReaderModule::FieldData> WeightingFieldReaderModule::field_map_;
WeightingFieldReaderModule::FieldData
WeightingFieldReaderModule::get_by_file_name(const std::string& file_name,
                                             Detector& detector,
                                             ROOT::Math::DisplacementVector2D<ROOT::Math::Cartesian2D<int>> field_size) {

    // Search in cache (NOTE: the path reached here is always a canonical name)
    auto iter = field_map_.find(file_name);
    if(iter != field_map_.end()) {
        // FIXME Check detector match here as well
        return iter->second;
    }

    // Load file
    std::ifstream file(file_name);

    std::string header;
    std::getline(file, header);

    LOG(TRACE) << "Header of file " << file_name << " is " << header;

    // Read the header
    std::string tmp;
    file >> tmp >> tmp;        // ignore the init seed and cluster length
    file >> tmp >> tmp >> tmp; // ignore the incident pion direction
    file >> tmp >> tmp >> tmp; // ignore the magnetic field (specify separately)
    double thickness, xpixsz, ypixsz;
    file >> thickness >> xpixsz >> ypixsz;
    thickness = Units::get(thickness, "um");
    xpixsz = Units::get(xpixsz, "um");
    ypixsz = Units::get(ypixsz, "um");
    file >> tmp >> tmp >> tmp >> tmp; // ignore temperature, flux, rhe (?) and new_drde (?)
    size_t xsize, ysize, zsize;
    file >> xsize >> ysize >> zsize;
    file >> tmp;

    // Check if weighting field matches chip
    check_detector_match(detector, thickness, xpixsz, ypixsz, field_size);

    if(file.fail()) {
        throw std::runtime_error("invalid data or unexpected end of file");
    }
    auto field = std::make_shared<std::vector<double>>();
    field->resize(xsize * ysize * zsize * 3);

    // Loop through all the field data
    for(size_t i = 0; i < xsize * ysize * zsize; ++i) {
        if(file.eof()) {
            throw std::runtime_error("unexpected end of file");
        }

        // Get index of weighting field
        size_t xind, yind, zind;
        file >> xind >> yind >> zind;

        if(file.fail() || xind > xsize || yind > ysize || zind > zsize) {
            throw std::runtime_error("invalid data");
        }
        xind--;
        yind--;
        zind--;

        // Loop through components of weighting field
        for(size_t j = 0; j < 3; ++j) {
            double input;
            file >> input;

            // Set the weighting field at a position
            (*field)[xind * ysize * zsize * 3 + yind * zsize * 3 + zind * 3 + j] = Units::get(input, "V/cm");
        }
    }

    return std::make_pair(field, std::array<size_t, 3>{{xsize, ysize, zsize}});
}
