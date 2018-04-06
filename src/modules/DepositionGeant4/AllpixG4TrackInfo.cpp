#include "AllpixG4TrackInfo.hpp"

using namespace allpix;

int AllpixG4TrackInfo::gCounter = 1;
std::map<int,int> AllpixG4TrackInfo::gG4ToCustomID = std::map<int, int>();

AllpixG4TrackInfo::AllpixG4TrackInfo(int G4TrackID) : G4VUserTrackInformation(), _counter(gCounter++) {
    gG4ToCustomID.emplace(G4TrackID, _counter);
}

void AllpixG4TrackInfo::resetCounter() {
    gCounter = 1;
    gG4ToCustomID.clear(); 
}

int AllpixG4TrackInfo::getCustomIDfromG4ID(int G4ID){
    return G4ID == 0 ? G4ID : gG4ToCustomID.at(G4ID);
}
