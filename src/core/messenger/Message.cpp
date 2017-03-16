/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "Message.hpp"

using namespace allpix;

std::shared_ptr<Detector> Message::getDetector() const { return detector_; }
void Message::setDetector(std::shared_ptr<Detector> detector) { detector_ = std::move(detector); }
