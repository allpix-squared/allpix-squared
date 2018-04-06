#include "UserHookSetUniqueTrackID.hpp"
#include "AllpixG4TrackInfo.hpp"

using namespace allpix;

//Initialisation of static counter for info
int AllpixG4TrackInfo::gCounter = 1;

void UserHookSetUniqueTrackID::PreUserTrackingAction(const G4Track* aTrack) {
  G4Track* theTrack = const_cast<G4Track*>(aTrack);
  theTrack->SetUserInformation(new AllpixG4TrackInfo());
}
