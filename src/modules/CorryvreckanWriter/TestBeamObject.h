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

namespace corryvreckan {

    class TestBeamObject : public TObject {

    public:
        // Constructors and destructors
        TestBeamObject() = default;
        //  virtual ~TestBeamObject(){
        //    m_timestamp = 0;
        //  }

        // Methods to get member variables
        std::string getDetectorID() { return m_detectorID; }
        long long int timestamp() { return m_timestamp; }
        void timestamp(long long int time) { m_timestamp = time; }

        // Methods to set member variables
        void setDetectorID(std::string detectorID) { m_detectorID = std::move(detectorID); }

        // Member variables
        std::string m_detectorID;
        long long int m_timestamp;

        // ROOT I/O class definition - update version number when you change this class!
        ClassDef(TestBeamObject, 1)
    };

    // Vector type declaration
    using TestBeamObjects = std::vector<TestBeamObject*>;

} // namespace corryvreckan
#endif // TESTBEAMOBJECT_H
