#include "GeometryConstructionModule.hpp"

#include "ReadGeoDescription.hpp"

#include "../../core/utils/log.h"

using namespace allpix;

// name of the module
const std::string GeometryConstructionModule::name = "geometry_test";

void GeometryConstructionModule::run(){
    LOG(DEBUG) << "START RUN";
    
    std::string file_name = "models/mimosa26.xml";

    auto geo_descriptions = ReadGeoDescription(file_name);
    
    for(auto &geo_desc : geo_descriptions.GetDetectorsMap()){
        std::cout << geo_desc.first << " " << geo_desc.second->GetHalfChipX() << std::endl;
    }
    
    LOG(DEBUG) << "END RUN";
}
