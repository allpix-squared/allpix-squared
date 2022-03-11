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
     * @brief Ionization / charge creation energy for different materials
     */
    static std::map<SensorMaterial, double> ionization_energies = {
        {SensorMaterial::SILICON, Units::get(3.64, "eV")}, {SensorMaterial::GALLIUM_ARSENIDE, Units::get(4.2, "eV")}};

    /**
     * @brief Fano factors for different materials
     */
    static std::map<SensorMaterial, double> fano_factors = {{SensorMaterial::SILICON, 0.115},
                                                            {SensorMaterial::GALLIUM_ARSENIDE, 0.14}};

} // namespace allpix

#endif /* ALLPIX_PROPERTIES_H */
