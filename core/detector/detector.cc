/**
 * allpix detector class implementation
 */

#include "exceptions.h"
#include "configuration.h"
#include "log.h"
#include "detector.h"

using namespace allpix;

Detector::Detector(const Configuration config) {
  LOG(logDEBUG) << "Reading detector configuration from: " << config.Name();

  // Ensure a detector configuration section is provided:
  if(!config.SetSection("Detector")) throw allpix::exception("No detector configuration provided");

  LOG(logDEBUG) << "Get(detector_model) = " << config.Get("detector_model","none");
}

Detector::~Detector() {
}

