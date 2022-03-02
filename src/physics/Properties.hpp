/**
 * @file
 * @brief Utility to register unit conversions with the framework's unit system
 *
 * @copyright Copyright (c) 2022 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_PROPERTIES_H
#define ALLPIX_PROPERTIES_H

#include "core/geometry/DetectorModel.hpp"
#include "core/utils/log.h"
#include "core/utils/unit.h"
#include "exceptions.h"

namespace allpix {

    /**
     * @brief Obtain ionization / charge creation energy for different materials
     */
    static double get_charge_creation_energy(SensorMaterial material) {
        if(material == SensorMaterial::SILICON) {
            return Units::get(3.64, "eV");
        } else if(material == SensorMaterial::GALLIUM_ARSENIDE) {
            return Units::get(4.2, "eV");
        }
        throw ModelError();
        // "Key charge_creation_energy not set and no default value found for sensor material " + allpix::to_string(material)
    }

    /**
     * @brief Obtain Fano factor for different materials
     */
    static double get_fano_factor(SensorMaterial material) {
        if(material == SensorMaterial::SILICON) {
            return 0.115;
        } else if(material == SensorMaterial::GALLIUM_ARSENIDE) {
            return 0.14;
        }
        throw ModelError();
        // "Key fano_factor not set and no default value found for sensor material " + allpix::to_string(material)
    }

} // namespace allpix

#endif /* ALLPIX_PROPERTIES_H */
