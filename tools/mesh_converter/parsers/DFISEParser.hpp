#ifndef ALLPIX_MESHPARSER_DFISE_H
#define ALLPIX_MESHPARSER_DFISE_H

#include "../MeshParser.hpp"

#include <iostream>
#include <map>
#include <vector>

namespace mesh_converter {

    class DFISEParser : public MeshParser {
        // Sections to read in DF-ISE file
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

    public:
        // Read the grid
        MeshMap read_meshes(const std::string& file_name, bool mesh_tree) override;

        // Read the electric field
        FieldMap read_fields(const std::string& file_name) override;
    };
} // namespace mesh_converter

#endif // ALLPIX_MESHPARSER_DFISE_H
