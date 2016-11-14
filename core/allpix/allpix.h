/**
 * allpix class header
 */

#ifndef ALLPIX_API_H
#define ALLPIX_API_H

#include "G4RunManager.hh"
#include "configuration.h"
#include "detector.h"

namespace allpix {

  /** allpix API class definition
   *
   */
  class allpixCore {

  public:
    /** Default constructor for the allpix API
     *
     *  Instantiates a Geant4 RunManager instance
     *
     */
    allpixCore(std::string config);

    /** Default destructor for liballpix API
     *
     *  Will destroy all pointer data members from Geant4.
     */
    ~allpixCore();

    /** Copy constructor
	FIXME add copy constructor because we have pointer members!
     */
    //allpixcore(const allpixCore &L);

    /** Assignment
	FIXME add assignment declaration because we have pointer members!
     */
    //allpixCore & operator=(const allpixCore &L);

  private:
    G4RunManager * m_runmanager;

    Configuration m_config;

    Detector m_detector;
    
  }; // class allpixCore

} //namespace allpix

#endif /* ALLPIX_API_H */
