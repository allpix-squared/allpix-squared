#include "AllpixG4TrackInfo.hpp"
#include "G4VProcess.hh"

using namespace allpix;

int AllpixG4TrackInfo::gCounter = 1;
std::map<int, int> AllpixG4TrackInfo::gG4ToCustomID = std::map<int, int>();
std::map<int, std::unique_ptr<MCTrack>> AllpixG4TrackInfo::gStoredTracks = std::map<int, std::unique_ptr<MCTrack>>();

AllpixG4TrackInfo::AllpixG4TrackInfo(const G4Track* const aTrack) : _customTrackID(gCounter++) {
    gG4ToCustomID[aTrack->GetTrackID()] = _customTrackID;

    // If the Geant4 ID is 0 we mustn't convert
    auto G4ParentID = aTrack->GetParentID();
    _parentTrackID = G4ParentID == 0 ? G4ParentID : gG4ToCustomID.at(G4ParentID);

    auto G4OriginatingVolumeName = aTrack->GetVolume()->GetName();
    auto G4Process = aTrack->GetCreatorProcess();
    auto processType = G4Process ? G4Process->GetProcessType() : -1;
    auto processName = G4Process ? static_cast<std::string>(G4Process->GetProcessName()) : "none";
    _mcTrack = std::make_unique<MCTrack>(static_cast<ROOT::Math::XYZPoint>(aTrack->GetPosition()),
                                         static_cast<std::string>(G4OriginatingVolumeName),
                                         processName,
                                         processType,
                                         aTrack->GetDynamicParticle()->GetPDGcode(),
                                         _customTrackID,
                                         _parentTrackID,
                                         aTrack->GetTotalEnergy());
}

void AllpixG4TrackInfo::finaliseInfo(const G4Track* const aTrack) {
    _mcTrack->setNumberSteps(aTrack->GetCurrentStepNumber());
    _mcTrack->setEndPoint(static_cast<ROOT::Math::XYZPoint>(aTrack->GetPosition()));
    _mcTrack->setFinalEnergy(aTrack->GetTotalEnergy());
    //_mcTrack->dumpInfo();
    AllpixG4TrackInfo::storeTrack(std::move(_mcTrack));
}

void AllpixG4TrackInfo::registerTrack(int trackID) {
    std::cout << "Registeres track: " << trackID << '\n';
    gStoredTracks[trackID] = nullptr;
}

void AllpixG4TrackInfo::storeTrack(std::unique_ptr<MCTrack> theTrack) {
    auto ID = theTrack->getTrackID();
    auto element = gStoredTracks.find(ID);
    if(element != gStoredTracks.end()) {
        std::cout << "Stored: \n" << *(theTrack.get()) << '\n';
        (*element).second = std::move(theTrack);
    }
}

void AllpixG4TrackInfo::reset(Module* module, Messenger* messenger) {
    auto test = std::vector<MCTrack>();
    for(auto it = gStoredTracks.begin(); it != gStoredTracks.end(); ++it) {
        if((*it).second)
            test.push_back(*std::move((*it).second));
    }
    auto mc_track_message = std::make_shared<MCTrackMessage>(std::move(test));
    messenger->dispatchMessage(module, mc_track_message);
    gCounter = 1;
    gG4ToCustomID.clear();
}
