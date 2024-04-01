/**
 * @file
 * @brief Implementation of module to define magnetic fields
 *
 * @copyright Copyright (c) 2018-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "MagneticFieldReaderModule.hpp"

#include <fstream>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>

#include <Math/Point3D.h>
#include <Math/Vector3D.h>
#include <TH2F.h>

#include "core/config/exceptions.h"
#include "core/geometry/DetectorModel.hpp"
#include "core/utils/log.h"
#include "core/utils/unit.h"

using namespace allpix;

MagneticFieldReaderModule::MagneticFieldReaderModule(Configuration& config, Messenger*, GeometryManager* geoManager)
    : Module(config), geometryManager_(geoManager) {
    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();
}

void MagneticFieldReaderModule::initialize() {
    MagneticFieldType type = MagneticFieldType::NONE;

    // Check field strength
    auto field_model = config_.get<MagneticField>("model");

    // Calculate the field depending on the configuration
    if(field_model == MagneticField::CONSTANT) {
        LOG(TRACE) << "Adding constant magnetic field";
        type = MagneticFieldType::CONSTANT;

        auto b_field = config_.get<ROOT::Math::XYZVector>("magnetic_field", ROOT::Math::XYZVector());

        MagneticFieldFunction function = [b_field](const ROOT::Math::XYZPoint&) { return b_field; };

        geometryManager_->setMagneticFieldFunction(function, type);
        auto detectors = geometryManager_->getDetectors();
        for(auto& detector : detectors) {
            // TODO the magnetic field is calculated once for the center position of the detector. This could be extended to
            // a function enabling a gradient in the magnetic field inside the sensor
            auto position = detector->getPosition();
            detector->setMagneticField(detector->getOrientation().Inverse() * geometryManager_->getMagneticField(position));
            LOG(DEBUG) << "Magnetic field in detector " << detector->getName() << ": "
                       << Units::display(detector->getMagneticField(detector->getLocalPosition(position)), {"T", "mT"});
        }
        LOG(INFO) << "Set constant magnetic field: " << Units::display(b_field, {"T", "mT"});
    } else if(field_model == MagneticField::MESH) {
        type = MagneticFieldType::CUSTOM;

        auto fallback_field = config_.get<ROOT::Math::XYZVector>("magnetic_field_fallback", ROOT::Math::XYZVector());
        auto field_data = read_field();

        auto dims = field_data.getDimensionality(); // Vector dimensionality of the field (will be 3 for B-field)
        auto cellsize = field_data.getSize();       // Physical dimensions for each cell in the field mesh
        auto ncells = field_data.getDimensions();   // Number of cells in each direction of the field mesh
        auto field_mesh = field_data.getData();     // The field mesh B-field data as a flattened array

        MagneticFieldFunction function =
            [fallback_field, field_mesh, cellsize, ncells, dims](const ROOT::Math::XYZPoint& coord) {
                // Find the nearest field mesh cell to the given input coordinate (assuming the mesh is centred about the
                // origin)
                auto xind = std::round((coord.X() / cellsize[0]) + (static_cast<double>(ncells[0]) / 2.));
                auto yind = std::round((coord.Y() / cellsize[1]) + (static_cast<double>(ncells[1]) / 2.));
                auto zind = std::round((coord.Z() / cellsize[2]) + (static_cast<double>(ncells[2]) / 2.));

                if(xind < 0 || xind >= static_cast<double>(ncells[0]) || yind < 0 ||
                   yind >= static_cast<double>(ncells[1]) || zind < 0 || zind >= static_cast<double>(ncells[2])) {
                    return fallback_field;
                } else {
                    auto field = field_mesh.get();

                    auto ix = static_cast<uint64_t>(xind);
                    auto iy = static_cast<uint64_t>(yind);
                    auto iz = static_cast<uint64_t>(zind);

                    auto bfieldx = field->at(ix * ncells[1] * ncells[2] * dims + iy * ncells[2] * dims + iz * dims);
                    auto bfieldy = field->at(ix * ncells[1] * ncells[2] * dims + iy * ncells[2] * dims + iz * dims + 1);
                    auto bfieldz = field->at(ix * ncells[1] * ncells[2] * dims + iy * ncells[2] * dims + iz * dims + 2);
                    return ROOT::Math::XYZVector(bfieldx, bfieldy, bfieldz);
                }
            };

        geometryManager_->setMagneticFieldFunction(function, type);
        auto detectors = geometryManager_->getDetectors();
        for(auto& detector : detectors) {
            // TODO the magnetic field is calculated once for the center position of the detector. This could be extended to
            // a function enabling a gradient in the magnetic field inside the sensor
            auto position = detector->getPosition();
            detector->setMagneticField(detector->getOrientation().Inverse() * geometryManager_->getMagneticField(position));
            LOG(DEBUG) << "Magnetic field in detector " << detector->getName() << ": "
                       << Units::display(detector->getMagneticField(detector->getLocalPosition(position)), {"T", "mT"});
        }
        LOG(INFO) << "Set meshed magnetic field from file";
    }
}

/**
 * The field data read from files are shared between module instantiations using the static
 * FieldParser's getByFileName method.
 */
FieldParser<double> MagneticFieldReaderModule::field_parser_(FieldQuantity::VECTOR);
FieldData<double> MagneticFieldReaderModule::read_field() {

    try {
        LOG(TRACE) << "Fetching magnetic field from mesh file";

        // Get field from file
        auto field_data = field_parser_.getByFileName(config_.getPath("file_name", true), "T");

        LOG(INFO) << "Set magnetic field with " << field_data.getDimensions().at(0) << "x"
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
