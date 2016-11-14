/**
 * allpix detector class header
 */

#ifndef ALLPIX_DETECTOR_H
#define ALLPIX_DETECTOR_H

#include "configuration.h"

namespace allpix {

  /** allpix detector class definition
   *
   */
  class Detector {

  public:
    /** Default constructor for the allpix detector class
     */
    Detector(const Configuration config);

    Detector() {};

    /** Default destructor for the allpix detector class
     */
    ~Detector();

  }; // class detector

} //namespace allpix

#endif /* ALLPIX_DETECTOR_H */
