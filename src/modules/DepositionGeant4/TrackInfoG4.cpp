#include "TrackInfoG4.hpp"
#include "G4VProcess.hh"

using namespace allpix;

TrackInfoG4::TrackInfoG4(int custom_track_id, int parent_track_id, const G4Track* const aTrack)
    : custom_track_id_(custom_track_id), parent_track_id_(parent_track_id) {
    auto G4Process = aTrack->GetCreatorProcess();
    origin_g4_process_type_ = (G4Process != nullptr) ? G4Process->GetProcessType() : -1;
    particle_id_ = aTrack->GetDynamicParticle()->GetPDGcode();
    start_point_ = static_cast<ROOT::Math::XYZPoint>(aTrack->GetPosition());
    origin_g4_vol_name_ = aTrack->GetVolume()->GetName();
    origin_g4_process_name_ = (G4Process != nullptr) ? static_cast<std::string>(G4Process->GetProcessName()) : "none";
    initial_kin_E_ = aTrack->GetKineticEnergy();
    initial_tot_E_ = aTrack->GetTotalEnergy();
}

void TrackInfoG4::finalizeInfo(const G4Track* const aTrack) {
    final_kin_E_ = aTrack->GetKineticEnergy();
    final_tot_E_ = aTrack->GetTotalEnergy();
    n_steps_ = aTrack->GetCurrentStepNumber();
    end_point_ = static_cast<ROOT::Math::XYZPoint>(aTrack->GetPosition());
}

int TrackInfoG4::getID() const {
    return custom_track_id_;
}

int TrackInfoG4::getParentID() const {
    return parent_track_id_;
}

ROOT::Math::XYZPoint TrackInfoG4::getStartPoint() const {
    return start_point_;
}

ROOT::Math::XYZPoint TrackInfoG4::getEndPoint() const {
    return end_point_;
}

int TrackInfoG4::getParticleID() const {
    return particle_id_;
}

int TrackInfoG4::getCreationProcessType() const {
    return origin_g4_process_type_;
}

int TrackInfoG4::getNumberOfSteps() const {
    return n_steps_;
}

double TrackInfoG4::getKineticEnergyInitial() const {

    return initial_kin_E_;
}

double TrackInfoG4::getTotalEnergyInitial() const {
    return initial_tot_E_;
}

double TrackInfoG4::getKineticEnergyFinal() const {
    return final_kin_E_;
}

double TrackInfoG4::getTotalEnergyFinal() const {
    return final_tot_E_;
}

std::string TrackInfoG4::getOriginatingVolumeName() const {
    return origin_g4_vol_name_;
}

std::string TrackInfoG4::getCreationProcessName() const {
    return origin_g4_process_name_;
}
