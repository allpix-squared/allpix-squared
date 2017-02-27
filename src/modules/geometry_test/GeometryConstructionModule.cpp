#include "GeometryConstructionModule.hpp"

#include "ReadGeoDescription.hpp"

#include "../../core/geometry/GeometryManager.hpp"

#include "../../core/utils/log.h"

using namespace allpix;

// name of the module
const std::string GeometryConstructionModule::name = "geometry_test";

void GeometryConstructionModule::run(){
    LOG(DEBUG) << "START RUN";
    
    std::string file_name = config_.get<std::string>("file");
    std::cout << file_name << std::endl;
    auto geo_descriptions = ReadGeoDescription(file_name);
    
    /*for(auto &geo_desc : ){
        std::cout << geo_desc.first << " " << geo_desc.second->GetHalfChipX() << std::endl;
    }*/
      
    auto detector_model = geo_descriptions.GetDetectorsMap()["mimosa"];  
    Detector det1("name1", detector_model);
    getGeometryManager()->addDetector(det1);
    
    Detector det2("name2", detector_model);
    getGeometryManager()->addDetector(det1);
    
    LOG(DEBUG) << "END RUN";
}
