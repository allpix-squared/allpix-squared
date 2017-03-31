/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */
#ifndef ALLPIX_GEOMETRY_DETECTOR_MODEL_H
#define ALLPIX_GEOMETRY_DETECTOR_MODEL_H

#include <string>
#include <utility>

namespace allpix {
    class DetectorModel {
    public:
        explicit DetectorModel(std::string type) : type_(std::move(type)) {}
        virtual ~DetectorModel() = default;

        std::string getType() const;
        void setType(std::string type);

        // FIXME: what would be valid overlapping parameters here
    private:
        std::string type_;
    };
} // namespace allpix

#endif // ALLPIX_GEOMETRY_MANAGER_H
