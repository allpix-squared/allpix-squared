#include <iostream>
#include <ostream>
#include <vector>

#include "../core/AllPix.hpp"

#include "../core/module/Module.hpp"

#include "../core/utils/exceptions.h"
#include "../core/utils/log.h"

#include "examples.h"

using namespace allpix;

int main(int, const char **) {
    //TEMP TEST
    Messenger *msg = new Messenger();
    TestModuleOne test1;
    TestModuleTwo test2;
    test2.start(msg);
    test1.start(msg);
    delete msg;
    //END TEST
    
    
    
    AllPix *apx = 0;
    try {
        // Set global log level:
        LogLevel log_level = Log::getLevelFromString("DEBUG");
        Log::setReportingLevel(log_level);
        
        LOG(INFO) << "Log level: " << Log::getStringFromLevel(log_level);
        
        apx = new AllPix();
    } catch(allpix::exception &e) { 
        LOG(CRITICAL) << e.what(); 
    }
    
    delete apx;
    
    return 0;
}
