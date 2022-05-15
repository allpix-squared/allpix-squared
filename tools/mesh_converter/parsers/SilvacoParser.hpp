/**
 * @file
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_MESHPARSER_SILVACO_H
#define ALLPIX_MESHPARSER_SILVACO_H

#include "../MeshParser.hpp"

#include <iostream>
#include <map>
#include <vector>

namespace mesh_converter {

    class SilvacoParser : public MeshParser {
        // Sections to read in extracted files
        enum class DFSection {
            NONE = 0,
            IGNORED,
            HEADER,
            INFO,
            REGION,
            COORDINATES,
            VERTICES,
            EDGES,
            FACES,
            ELEMENTS,

            DONOR_CONCENTRATION,
            DOPING_CONCENTRATION,
            ACCEPTOR_CONCENTRATION,
            ELECTRIC_FIELD,
            ELECTROSTATIC_POTENTIAL,
            VALUES
        };

    private:
        // Read the grid
        MeshMap read_meshes(const std::string& file_name) override;

        // Read the electric field
        FieldMap read_fields(const std::string& file_name) override;
    };
} // namespace mesh_converter

#endif // ALLPIX_MESHPARSER_SILVACO_H
