/**
 * allpix API class implementation
 */

#include "allpix.h"
#include "exceptions.h"
#include "configuration.h"
#include "log.h"
#include "config.h"
#include "detector.h"

#include <istream>

using namespace allpix;

allpixCore::allpixCore(std::string config) {

  LOG(logQUIET) << "This is " << PACKAGE_STRING;

  std::ifstream cfgfile(config);
  if (cfgfile.is_open()) {
    m_config = Configuration(cfgfile);
    m_config.Set("Name", config);
    LOG(logDEBUG) << "Successfully loaded configuration file " << config;
  } else {
    LOG(logCRITICAL) << "Unable to open file '" << config << "'";
    throw allpix::exception("Unable to open file '" + config + "'");
  }

  // Initialize Geant4 run manager:
  m_runmanager = new G4RunManager();


  // Initialize allpix detector description:
  m_detector = allpix::Detector(m_config);
}

allpixCore::~allpixCore() {
  // Delete the pointer data member
  delete m_runmanager;
}

