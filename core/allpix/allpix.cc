/**
 * allpix API class implementation
 */

#include "allpix.h"
#include "exceptions.h"
#include "configuration.h"

#include <istream>

using namespace allpix;

allpixCore::allpixCore(std::string config) {

  std::ifstream cfgfile(config);
  if (cfgfile.is_open()) {
    m_config = Configuration(cfgfile);
    m_config.Set("Name", config);
  } else {
    throw allpix::exception("Unable to open file '" + config + "'");
  }

  
  m_runmanager = new G4RunManager();
}

allpixCore::~allpixCore() {
  // Delete the pointer data member
  delete m_runmanager;
}

