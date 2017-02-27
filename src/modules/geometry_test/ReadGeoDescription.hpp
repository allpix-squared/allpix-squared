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

//ALERT: this reader is very buggy... so do not throw in wrong input
//FIXME: we want to get rid of this later of course

#include "../../core/geometry/PixelDetector.hpp"

class TXMLNode;

namespace allpix{
    
    class ReadGeoDescription {
    public:
        ReadGeoDescription(std::string);
        ~ReadGeoDescription(){};

        std::map<std::string, std::shared_ptr<PixelDetector > > &GetDetectorsMap();
        bool StringIsRelevant(std::string);
    private:     
        void ParseContext(TXMLNode *);
        
        std::string m_currentNodeName;
        std::string m_currentAtt;

        std::map<std::string, std::shared_ptr<PixelDetector>> m_detsGeo;        
        std::string m_currentType;

        std::map<std::string, double> m_unitsMap;

    };
}

#endif /* ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_READ_GEO_H */
