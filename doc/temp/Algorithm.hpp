#ifndef ALGORITHM_H 
#define ALGORITHM_H 1

// Global includes
#include <string>

// ROOT includes
#include "TStopwatch.h"

// Local includes
#include "Clipboard.hpp"
#include "Parameters.hpp"
#include "logger.hpp"

//-------------------------------------------------------------------------------
// The algorithm class is the base class that all user algorithms are built on. It
// allows the analysis class to hold algorithms of different types, without knowing
// what they are, and provides the functions initialise, run and finalise. It also
// gives some basic tools like the "tcout" replacement for "cout" (appending the
// algorithm name) and the stopwatch for timing measurements.
//-------------------------------------------------------------------------------

enum StatusCode {
  Success,
  SkipEvent,
  Failure,
} ;

class Algorithm{

public:

  // Constructors and destructors
  Algorithm(){}
  Algorithm(string name){
    m_name = name;
    m_stopwatch = new TStopwatch();
    info = logger(m_name+"::info", Info);
    debug = logger(m_name+"::debug", Debug);
    warning = logger(m_name+"::warning", Warning);
    error = logger(m_name+"::error", Error);
  }
  virtual ~Algorithm(){}
  
  // Three main functions - initialise, run and finalise. Called for every algorithm
  virtual void initialise(Parameters*){}
  virtual StatusCode run(Clipboard*){ return Success;}
  virtual void finalise(){}
  
  // Methods to get member variables
  string getName(){return m_name;}
  TStopwatch* getStopwatch(){return m_stopwatch;}
  
  bool m_debug;

protected:

  // Member variables
  Parameters* parameters;
  TStopwatch* m_stopwatch;
  string m_name;
  logger info;
  logger debug;
  logger warning;
  logger error;
  
};


#endif
