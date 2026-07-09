/**
 * @file
 *
 * @copyright Copyright (c) 2026 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_MESHPARSER_SILVACOSSF_H
#define ALLPIX_MESHPARSER_SILVACOSSF_H

#include "../MeshParser.hpp"

#include <map>
#include <string>
#include <vector>

namespace mesh_converter {

    /**
     * @brief Parser for raw Silvaco TCAD structure/solution files (.str / .sta)
     *
     * Reads the tagged record format written by ATHENA and ATLAS directly:
     * - "c <id> <x> <y> [<z>]" coordinate records (1-based ids)
     * - "s <n> <codes...>" solution specification listing the quantity code of each value column
     * - "n <id> <region> <values...>" nodal value records (0-based ids, coordinate id = node id + 1)
     *
     * Interface nodes shared between two regions appear as duplicate value records for the same node;
     * the record with the largest absolute electrostatic potential (the semiconductor side) is kept.
     */
    class SilvacoSSFParser : public MeshParser {

    private:
        // Read the grid
        MeshMap read_meshes(const std::string& file_name) override;

        // Read the field for the requested observable
        FieldMap read_fields(const std::string& file_name, const std::string& observable) override;

        // Resolve the .grd/.dat file name handed over by the converter to the raw input file
        static std::string resolve_file(const std::string& file_name);

        // Parse the raw file into coordinates, quantity codes and deduplicated nodal values
        static void parse_file(const std::string& file_name,
                               std::map<int, std::array<double, 3>>& coords,
                               std::vector<int>& codes,
                               std::map<int, std::vector<double>>& node_values,
                               bool& two_dimensional);
    };
} // namespace mesh_converter

#endif // ALLPIX_MESHPARSER_SILVACOSSF_H
