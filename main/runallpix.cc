#include "allpix.h"
#include "configuration.h"

#include <iostream>
#include <ostream>
#include <vector>

int main(int /*argc*/, const char ** argv) {

  try {

    std::string config = "testconfig.cfg";
    
    auto allpixCore(config);
  } catch(...) {}
  
    return 0;
}
