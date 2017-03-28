/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "Message.hpp"

#include <memory>
#include <utility>

using namespace allpix;

// constructor and destructor
Message::Message() : detector_() {}
Message::~Message() = default;

// get and set detector
std::shared_ptr<Detector> Message::getDetector() const {
    return detector_;
}
void Message::setDetector(std::shared_ptr<Detector> detector) {
    detector_ = std::move(detector);
}
