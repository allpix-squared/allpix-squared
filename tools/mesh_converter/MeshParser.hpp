#ifndef ALLPIX_MESHPARSER_H
#define ALLPIX_MESHPARSER_H

#include "MeshElement.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace mesh_converter {

    using MeshMap = std::map<std::string, std::vector<Point>>;
    using FieldMap = std::map<std::string, std::map<std::string, std::vector<Point>>>;

    /**
     * @brief Parser class to read different data formats
     */
    class MeshParser {
    public:
        static std::shared_ptr<MeshParser> Factory(std::string parser);
        /**
         * @brief Default constructor
         */
        MeshParser() = default;
        /**
         * @brief Default destructor
         */
        virtual ~MeshParser() = default;

        // Read the grid
        virtual MeshMap read_meshes(const std::string& file_name, bool mesh_tree) = 0;

        // Read the electric field
        virtual FieldMap read_fields(const std::string& file_name) = 0;
    };

} // namespace mesh_converter

#include "parsers/DFISEParser.hpp"

#endif // ALLPIX_MESHPARSER_H
