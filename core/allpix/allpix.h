/**
 * allpix class header
 */

#ifndef ALLPIX_API_H
#define ALLPIX_API_H

#include "G4RunManager.hh"

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
    allpixCore();

    /** Default destructor for liballpix API
     *
     *  Will power down the DTB, disconnect properly from the testboard,
     *  and destroy all member objects.
     */
    ~allpixCore();

  private:
    G4RunManager m_runmanager;

  }; // class allpixCore

} //namespace allpix

#endif /* ALLPIX_API_H */
