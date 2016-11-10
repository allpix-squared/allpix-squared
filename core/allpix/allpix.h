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

  }; // class allpixCore

} //namespace allpix

#endif /* ALLPIX_API_H */
