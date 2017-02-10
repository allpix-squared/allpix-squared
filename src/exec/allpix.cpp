#include <iostream>
#include <ostream>
#include <vector>

#include "../core/AllPix.hpp"

#include "../core/utils/exceptions.h"
#include "../core/utils/log.h"

using namespace allpix;

int main(int argc, const char ** argv) {
    
    std::string logLevel = "DEBUG";
    
    AllPix *apx = 0;
    try {
        
        // Set global log level:
        Log::ReportingLevel() = Log::FromString(logLevel);
        LOG(logINFO) << "Log level: " << logLevel;
        
        apx = new AllPix();
    } catch(allpix::exception &e) { 
        LOG(logCRITICAL) << e.what(); 
    }
    
    delete apx;
    
    return 0;
}
