#include "MeshParser.hpp"

using namespace mesh_converter;

std::shared_ptr<MeshParser> MeshParser::factory(std::string parser) {
    std::transform(parser.begin(), parser.end(), parser.begin(), ::tolower);

    if(parser == "df-ise" || parser == "dfise") {
        return std::make_shared<DFISEParser>();
    } else {
        throw std::runtime_error("Unknown parser type \"" + parser + "\"");
    }
}
