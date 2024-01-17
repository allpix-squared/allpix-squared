/**
 * @file
 * @brief Utility to register unit conversions with the framework's unit system
 *
 * @copyright Copyright (c) 2019-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_ADD_UNITS_H
#define ALLPIX_ADD_UNITS_H

#include "core/utils/log.h"
#include "core/utils/unit.h"

namespace allpix {

    /**
     * @brief Sets the default unit conventions
     */
    static void register_units() {
        LOG(TRACE) << "Adding physical units";

        // LENGTH
        Units::add("nm", 1e-6);
        Units::add("um", 1e-3);
        Units::add("mm", 1);
        Units::add("cm", 1e1);
        Units::add("dm", 1e2);
        Units::add("m", 1e3);
        Units::add("km", 1e6);

        // TIME
        Units::add("ps", 1e-3);
        Units::add("ns", 1);
        Units::add("us", 1e3);
        Units::add("ms", 1e6);
        Units::add("s", 1e9);

        // TEMPERATURE
        Units::add("K", 1);

        // ENERGY
        Units::add("eV", 1e-6);
        Units::add("keV", 1e-3);
        Units::add("MeV", 1);
        Units::add("GeV", 1e3);

        // CHARGE
        Units::add("e", 1);
        Units::add("ke", 1e3);
        Units::add("fC", 1 / 1.602176634e-4);
        Units::add("C", 1 / 1.602176634e-19);

        // VOLTAGE
        // NOTE: fixed by above
        Units::add("mV", 1e-9);
        Units::add("V", 1e-6);
        Units::add("kV", 1e-3);

        // MAGNETIC FIELD
        Units::add("kT", 1);
        Units::add("T", 1e-3);
        Units::add("mT", 1e-6);

        // ANGLES
        // NOTE: these are fake units
        Units::add("deg", 3.14159265358979323846 / 180.0);
        Units::add("rad", 1);
        Units::add("mrad", 1e-3);

        // FLUENCE
        // NOTE: pseudo unit "1-MeV neutron equivalent"
        Units::add("neq", 1);
    }
} // namespace allpix

#endif /* ALLPIX_ADD_UNITS_H */
