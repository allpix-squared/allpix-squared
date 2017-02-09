// Global includes
#include <iostream>
#include <string>
#include <signal.h>

// Local includes
#include "Parameters.hpp"
#include "Allpix2.hpp"
#include "DummyModule.hpp"
#include "logger.hpp"

//-------------------------------------------------------------------------------
// This executable reads command line parameters, then runs each algorithm
// added. Algorithms have 3 steps: initialise, run and finalise.
//-------------------------------------------------------------------------------

// The analysis object to be used
Allpix2* allpix2;

// Handle user interruption
// This allows you to ^C at any point in a controlled way
void userException(int sig){
  error<<endl<<"User interrupted, exiting Allpix2"<<endl;
//  allpix2->clearClipboard(); // To be implemented, cleanup of currently stored object on the storage element
  allpix2->finaliseAll();
  exit(sig);
}

int main(int argc, char *argv[]) {
 
  info<<endl;
  info<<"========================================================================"<<endl;
  info<<"========================== WELCOME TO ALLPIX2 =========================="<<endl;
  info<<"========================================================================"<<endl;
  info<<endl;
  
  // Register escape behaviour
  signal(SIGINT, userException);

  // New parameters object
  Parameters* parameters = new Parameters();
  
  // Global debug flag
  bool globalDebug = false;

  // Algorithm list - this should be replaced by dynamic library loading
  TestAlgorithm* testAlgorithm = new TestAlgorithm(globalDebug);
  
  // Overwrite steering file values from command line
  parameters->readCommandLineOptions(argc,argv);

  // Load alignment parameters - this should be replaced by either a build geometry or just removed completely
  if(!parameters->readConditions()) return 0;
  
  // Initialise the analysis object and add algorithms to run (again, replaced by dynamic library loading)
  allpix2 = new Allpix2(parameters);
  allpix2->add(testAlgorithm);
  
  // Run the algorithm chain
  allpix2->run();

  return 0;

}
