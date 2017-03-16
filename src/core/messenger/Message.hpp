/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MESSAGE_H
#define ALLPIX_MESSAGE_H

#include "core/geometry/Detector.hpp"

namespace allpix {
    class Message {
    public:
        // Constructor and destructors
        Message();
        virtual ~Message();

        // Link message to a detector
        std::shared_ptr<Detector> getDetector() const;
        void                      setDetector(std::shared_ptr<Detector>);

        // Set default copy behaviour
        Message(const Message&) = default;
        Message& operator=(const Message&) = default;

        // TODO: we might want to have a resolving function to get the content of a message by name for histogramming etc.

    private:
        std::shared_ptr<Detector> detector_;
    };
}

#endif /* ALLPIX_MESSAGE_H */
