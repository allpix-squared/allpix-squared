#include "Message.hpp"

#include <memory>
#include <utility>

using namespace allpix;

// constructor and destructor
BaseMessage::BaseMessage() : detector_(nullptr) {}
BaseMessage::BaseMessage(std::shared_ptr<Detector> detector) : detector_(std::move(detector)) {}
BaseMessage::~BaseMessage() = default;

// get and set detector
std::shared_ptr<Detector> BaseMessage::getDetector() const {
    return detector_;
}
