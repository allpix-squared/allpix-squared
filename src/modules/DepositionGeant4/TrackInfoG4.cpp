#include "TrackInfoG4.hpp"
#include "G4VProcess.hh"

using namespace allpix;

TrackInfoG4::TrackInfoG4(int custom_track_id, int parent_track_id, const G4Track* const aTrack)
    : custom_track_id_(custom_track_id), parent_track_id_(parent_track_id) {
    auto G4OriginatingVolumeName = aTrack->GetVolume()->GetName();
    auto G4Process = aTrack->GetCreatorProcess();
    auto processType = (G4Process != nullptr) ? G4Process->GetProcessType() : -1;
    auto processName = (G4Process != nullptr) ? static_cast<std::string>(G4Process->GetProcessName()) : "none";
    _mcTrack = std::make_unique<MCTrack>(static_cast<ROOT::Math::XYZPoint>(aTrack->GetPosition()),
                                         static_cast<std::string>(G4OriginatingVolumeName),
                                         processName,
                                         processType,
                                         aTrack->GetDynamicParticle()->GetPDGcode(),
                                         custom_track_id_,
                                         parent_track_id_,
                                         aTrack->GetKineticEnergy(),
                                         aTrack->GetTotalEnergy());
}

void TrackInfoG4::finaliseInfo(const G4Track* const aTrack) {
    _mcTrack->setNumberOfSteps(aTrack->GetCurrentStepNumber());
    _mcTrack->setEndPoint(static_cast<ROOT::Math::XYZPoint>(aTrack->GetPosition()));
    _mcTrack->setFinalKineticEnergy(aTrack->GetKineticEnergy());
    _mcTrack->setFinalTotalEnergy(aTrack->GetTotalEnergy());
}

std::unique_ptr<MCTrack> TrackInfoG4::releaseMCTrack() {
    return std::move(_mcTrack);
}
