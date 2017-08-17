#ifndef TESTBEAMOBJECT_H
#define TESTBEAMOBJECT_H 1

// Include files
#include <string>
#include <vector>
#include "TObject.h"
#include "TTree.h"

//-------------------------------------------------------------------------------
// Generic base class. Every class which inherits from TestBeamObject can be
// placed on the clipboard and written out to file.
//-------------------------------------------------------------------------------

namespace Corryvreckan {

    class TestBeamObject : public TObject {

    public:
        // Constructors and destructors
        TestBeamObject() {}
        //  virtual ~TestBeamObject(){
        //    m_timestamp = 0;
        //  }

        // Methods to get member variables
        std::string getDetectorID() { return m_detectorID; }
        long long int timestamp() { return m_timestamp; }
        void timestamp(long long int time) { m_timestamp = time; }

        // Methods to set member variables
        void setDetectorID(std::string detectorID) { m_detectorID = detectorID; }

        // Function to get instantiation of inherited class (given a string, give back an object of type 'daughter')
        //  static TestBeamObject* Factory(std::string, TestBeamObject* object = NULL);

        // Member variables
        std::string m_detectorID;
        long long int m_timestamp;

        // ROOT I/O class definition - update version number when you change this class!
        ClassDef(TestBeamObject, 1)
    };

    // Vector type declaration
    typedef std::vector<TestBeamObject*> TestBeamObjects;

} // namespace Corryvreckan
#endif // TESTBEAMOBJECT_H
