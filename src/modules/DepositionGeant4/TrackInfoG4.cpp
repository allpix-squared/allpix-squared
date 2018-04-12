#include "TrackInfoG4.hpp"
#include "G4VProcess.hh"

using namespace allpix;

// Initialisation of static members for TrackInfoG4 which are needed to store state
int TrackInfoG4::gCounter = 1;
std::map<int, int> TrackInfoG4::gG4ToCustomID = std::map<int, int>();
std::map<int, std::unique_ptr<MCTrack>> TrackInfoG4::gStoredTracks = std::map<int, std::unique_ptr<MCTrack>>();

TrackInfoG4::TrackInfoG4(const G4Track* const aTrack) : custom_track_id_(gCounter++) {
    gG4ToCustomID[aTrack->GetTrackID()] = custom_track_id_;

    // If the Geant4 ID is 0 we mustn't convert
    auto G4ParentID = aTrack->GetParentID();
    parent_track_id_ = G4ParentID == 0 ? G4ParentID : gG4ToCustomID.at(G4ParentID);

    auto G4OriginatingVolumeName = aTrack->GetVolume()->GetName();
    auto G4Process = aTrack->GetCreatorProcess();
    auto processType = G4Process ? G4Process->GetProcessType() : -1;
    auto processName = G4Process ? static_cast<std::string>(G4Process->GetProcessName()) : "none";
    _mcTrack = std::make_unique<MCTrack>(static_cast<ROOT::Math::XYZPoint>(aTrack->GetPosition()),
                                         static_cast<std::string>(G4OriginatingVolumeName),
                                         processName,
                                         processType,
                                         aTrack->GetDynamicParticle()->GetPDGcode(),
                                         custom_track_id_,
                                         parent_track_id_,
                                         aTrack->GetTotalEnergy());
}

void TrackInfoG4::finaliseInfo(const G4Track* const aTrack) {
    _mcTrack->setNumberSteps(aTrack->GetCurrentStepNumber());
    _mcTrack->setEndPoint(static_cast<ROOT::Math::XYZPoint>(aTrack->GetPosition()));
    _mcTrack->setFinalEnergy(aTrack->GetTotalEnergy());
    //_mcTrack->dumpInfo();
    TrackInfoG4::storeTrack(std::move(_mcTrack));
}

void TrackInfoG4::registerTrack(int trackID) {
    std::cout << "Registeres track: " << trackID << '\n';
    gStoredTracks[trackID] = nullptr;
}

void TrackInfoG4::storeTrack(std::unique_ptr<MCTrack> theTrack) {
    auto ID = theTrack->getTrackID();
    auto element = gStoredTracks.find(ID);
    if(element != gStoredTracks.end()) {
        std::cout << "Stored: \n" << *(theTrack.get()) << '\n';
        (*element).second = std::move(theTrack);
    }
}

void TrackInfoG4::reset(Module* module, Messenger* messenger) {
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
