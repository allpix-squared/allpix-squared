/**
 * allpix API class implementation
 */

#include "allpix.h"

using namespace allpix;

allpixCore::allpixCore() {
  m_runmanager = new G4RunManager();
}

allpixCore::~allpixCore() {
  // Delete the pointer data member
  delete m_runmanager;
}

