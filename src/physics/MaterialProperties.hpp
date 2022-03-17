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

namespace allpix {

    /**
     * @brief Ionization / charge creation energy for different materials
     *
     * @warning All values in framework-internal units, here: MeV
     */
    static std::map<SensorMaterial, double> ionization_energies = {
        {SensorMaterial::SILICON, 3.64e-6},
        {SensorMaterial::GALLIUM_ARSENIDE, 4.2e-6},
        {SensorMaterial::CADMIUM_TELLURIDE, 4.43e-6}, // https://doi.org/10.1016/0029-554X(74)90662-4
        {SensorMaterial::CADMIUM_ZINC_TELLURIDE, 4.6e-6},
        {SensorMaterial::DIAMOND, 13.1e-6},         // https://doi.org/10.1002/pssa.201600195
        {SensorMaterial::SILICON_CARBIDE, 7.6e-6}}; // https://doi.org/10.1109/NSSMIC.2005.1596542

    /**
     * @brief Fano factors for different materials
     */
    static std::map<SensorMaterial, double> fano_factors = {
        {SensorMaterial::SILICON, 0.115},
        {SensorMaterial::GALLIUM_ARSENIDE, 0.14},
        {SensorMaterial::CADMIUM_TELLURIDE, 0.24},      // https://doi.org/10.1016/j.nima.2018.09.02
        {SensorMaterial::CADMIUM_ZINC_TELLURIDE, 0.14}, // https://doi.org/10.1109/23.322857
        {SensorMaterial::DIAMOND, 0.382},               // https://doi.org/10.1002/pssa.201600195
        +{SensorMaterial::SILICON_CARBIDE, 0.1}};       // https://doi.org/10.1016/j.nima.2010.08.046

} // namespace allpix

#endif /* ALLPIX_PROPERTIES_H */
