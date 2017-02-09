#ifndef ANALYSIS_H
#define ANALYSIS_H 1

// Global includes
#include <vector>
#include <map>

// ROOT includes
#include "TFile.h"
#include "TDirectory.h"
#include "TBrowser.h"

// Local includes
#include "Algorithm.hpp"
#include "Clipboard.hpp"
#include "Parameters.hpp"

//-------------------------------------------------------------------------------
// The analysis class is the core class which allows the event processing to
// run. It basically contains a vector of algorithms, each of which is initialised,
// run on each event and finalised. It does not define what an event is, merely
// runs each algorithm sequentially and passes the clipboard between them (erasing
// it at the end of each run sequence). When an algorithm returns a 0, the event
// processing will stop.
//-------------------------------------------------------------------------------

class Allpix2 {

public:
  
  // Constructors and destructors
  Allpix2(Parameters*);
  virtual ~Allpix2(){};

  // Member functions
  void add(Algorithm*);
  void run();
  void timing();
  void initialiseAll();
  void finaliseAll();

protected:
  
  // Member variables
  Parameters* m_parameters;
  Clipboard* m_clipboard;
  vector<Algorithm*> m_algorithms;
  TFile* m_histogramFile;
  TDirectory* m_directory;
  int m_events;
};
#endif // ANALYSIS_H
