/**
 * @author John Idarraga <idarraga@cern.ch>
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_READ_GEO_H
#define ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_READ_GEO_H

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "../../core/config/Configuration.hpp"
#include "../../core/geometry/PixelDetectorModel.hpp"

namespace allpix {
    
    class ReadGeoDescription {
    public:
        // Constructor and destructor
        ReadGeoDescription(std::string);
        ~ReadGeoDescription() {};

        // Return the model (or return nullptr if it does not exist) 
        std::shared_ptr<PixelDetectorModel> getDetectorModel(const std::string &name) const;
    private:     
        std::shared_ptr<PixelDetectorModel> parseConfig(Configuration &);
        
        std::map<std::string, std::shared_ptr<PixelDetectorModel>> models_;      
    };
}

#endif /* ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_READ_GEO_H */
