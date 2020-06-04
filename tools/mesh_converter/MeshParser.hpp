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

        /**
         * @brief Method to read grids of mesh points from the given file
         * @param  file_name Canonical path of the input file
         * @return           Map with mesh points for all regions found in the file
         */
        virtual MeshMap read_meshes(const std::string& file_name) = 0;

        /**
         * @brief Method to read fields from the given file
         * @param  file_name Canonical path pof the input file
         * @return           Map with all fields for the different regions
         */
        virtual FieldMap read_fields(const std::string& file_name) = 0;
    };

} // namespace mesh_converter

#include "parsers/DFISEParser.hpp"

#endif // ALLPIX_MESHPARSER_H
