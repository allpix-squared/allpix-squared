#ifndef ALLPIXOBJECT_H
#define ALLPIXOBJECT_H 1

// Global includes
#include <vector>
#include <string>

// ROOT includes
#include "TObject.h"
#include "TTree.h"

//-------------------------------------------------------------------------------
// Generic base class. Every class which inherits from AllpixObject can be
// placed on the storage element and passed between algorithms
//-------------------------------------------------------------------------------

class AllpixObject : public TObject{
  
public:
  
  // Constructors and destructors
  AllpixObject(){}
  virtual ~AllpixObject(){}
  
  // Methods to get member variables
  std::string getDetectorID(){return m_detectorID;}

  // Methods to set member variables
  void setDetectorID(std::string detectorID){m_detectorID = detectorID;}
  
  // Member variables
  std::string m_detectorID;

  // ROOT I/O class definition - update version number when you change this class!
  //FIXME: this gives a linking error 
  //ClassDef(AllpixObject,1);

};

// Vector type declaration
typedef std::vector<AllpixObject*> AllpixObjects;

#endif // ALLPIXOBJECT_H
