/**
 * @file
 * @brief Utility to register unit conversions with the framework's unit system
 *
 * @copyright Copyright (c) 2022-2025 CERN and the Allpix Squared authors.
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
        {SensorMaterial::GALLIUM_NITRIDE,
         8.33e-6}, // https://etd.ohiolink.edu/apexprod/rws_etd/send_file/send?accession=osu1448405475
        {SensorMaterial::GERMANIUM, 2.97e-6},         // https://doi.org/10.1016/0883-2889(91)90002-I
        {SensorMaterial::CADMIUM_TELLURIDE, 4.43e-6}, // https://doi.org/10.1016/0029-554X(74)90662-4
        {SensorMaterial::CADMIUM_ZINC_TELLURIDE, 4.6e-6},
        {SensorMaterial::DIAMOND, 13.1e-6},         // https://doi.org/10.1002/pssa.201600195
        {SensorMaterial::SILICON_CARBIDE, 7.6e-6}, // https://doi.org/10.1109/NSSMIC.2005.1596542
        {SensorMaterial::CESIUM_LEAD_BROMIDE,5.3e-6}}; // https://doi.org/10.1038/s41467-018-04073-3

    /**
     * @brief Fano factors for different materials
     */
    static std::map<SensorMaterial, double> fano_factors = {
        {SensorMaterial::SILICON, 0.115},
        {SensorMaterial::GALLIUM_ARSENIDE, 0.14},
        {SensorMaterial::GALLIUM_NITRIDE,
         0.07}, // https://etd.ohiolink.edu/apexprod/rws_etd/send_file/send?accession=osu1448405475
        {SensorMaterial::GERMANIUM, 0.112},             // https://doi.org/10.1016/0883-2889(91)90002-I
        {SensorMaterial::CADMIUM_TELLURIDE, 0.24},      // https://doi.org/10.1016/j.nima.2018.09.025
        {SensorMaterial::CADMIUM_ZINC_TELLURIDE, 0.14}, // https://doi.org/10.1109/23.322857
        {SensorMaterial::DIAMOND, 0.382},               // https://doi.org/10.1002/pssa.201600195
        {SensorMaterial::SILICON_CARBIDE, 0.1},        // https://doi.org/10.1016/j.nima.2010.08.046
        {SensorMaterial::CESIUM_LEAD_BROMIDE,0.1}};                // Not yet measured, but assumed by authors https://doi.org/10.1038/s41566-020-00727-1
} // namespace allpix

#endif /* ALLPIX_PROPERTIES_H */
