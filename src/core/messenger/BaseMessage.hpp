/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_BASE_MESSAGE_H
#define ALLPIX_BASE_MESSAGE_H

#include <memory>

#include "core/geometry/Detector.hpp"

namespace allpix {
    class BaseMessage {
    public:
        // Constructor and destructors
        BaseMessage();
        BaseMessage(std::shared_ptr<Detector> detector);
        virtual ~BaseMessage();

        // Get linked detector
        std::shared_ptr<Detector> getDetector() const;

        // Set default copy behaviour
        BaseMessage(const BaseMessage&) = default;
        BaseMessage& operator=(const BaseMessage&) = default;

    private:
        std::shared_ptr<Detector> detector_;
    };
}

#endif /* ALLPIX_BASE_MESSAGE_H */
