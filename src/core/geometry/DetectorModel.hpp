/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */
#ifndef ALLPIX_GEOMETRY_DETECTOR_MODEL_H
#define ALLPIX_GEOMETRY_DETECTOR_MODEL_H

#include <string>

namespace allpix{
    class DetectorModel {
    public:
        DetectorModel() {}
        virtual ~DetectorModel() {}
        
        std::string getType() const;
        void setType(std::string type);
        
        //FIXME: what would be valid overlapping parameters here
    private:
        std::string type_;
    };
}

#endif // ALLPIX_GEOMETRY_MANAGER_H
