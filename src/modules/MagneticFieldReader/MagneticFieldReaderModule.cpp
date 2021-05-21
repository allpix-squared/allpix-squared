/**
 * @file
 * @brief Implementation of module to define magnetic fields
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
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
    // Enable parallelization of this module if multithreading is enabled
    enable_parallelization();
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
            detector->setMagneticField(detector->getOrientation().Inverse() *
                                       geometryManager_->getMagneticField(detector->getPosition()));
            LOG(DEBUG) << "Magnetic field in detector " << detector->getName() << ": "
                       << Units::display(detector->getMagneticField(), {"T", "mT"});
        }
        LOG(INFO) << "Set constant magnetic field: " << Units::display(b_field, {"T", "mT"});
    }
}
