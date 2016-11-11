#include "allpix.h"
#include "configuration.h"
#include "log.h"

#include <iostream>
#include <ostream>
#include <vector>

using namespace allpix;

int main(int /*argc*/, const char ** argv) {

  std::string logLevel = "DEBUG";

  try {

    // Set global log level:
    Log::ReportingLevel() = Log::FromString(logLevel);
    LOG(logINFO) << "Log level: " << logLevel;

    std::string config = "testconfig.cfg";
    
    allpixCore * apx = new allpixCore(config);
  } catch(...) {}
  
    return 0;
}
