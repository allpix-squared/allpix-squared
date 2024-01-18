/**
 * @file
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
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

    private:
        // Read the grid
        MeshMap read_meshes(const std::string& file_name) override;

        // Read the electric field
        FieldMap read_fields(const std::string& file_name, const std::string& observable) override;
    };
} // namespace mesh_converter

#endif // ALLPIX_MESHPARSER_SILVACO_H
