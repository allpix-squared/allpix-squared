/**
 * @file
 * @brief Definition of Monte-Carlo track object
 *
 * @copyright Copyright (c) 2018-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_MC_TRACK_H
#define ALLPIX_MC_TRACK_H

#include <Math/Point3D.h>
#include <Math/Vector3D.h>
#include <TRef.h>

#include "Object.hpp"

namespace allpix {
    /**
     * @ingroup Objects
     * @brief Monte-Carlo track through the world
     */
    class MCTrack : public Object {
    public:
        /**
         * @brief Construct a Monte-Carlo track
         * @param start_point Global point where track came into existence
         * @param end_point Global point where track went out of existence
         * @param g4_volume_start Geant4 volume where track originated in
         * @param g4_volume_end Geant4 volume where track terminated in
         * @param g4_prod_process_name Geant4 creation process name
         * @param g4_prod_process_type Geant4 creation process id
         * @param particle_id PDG particle id
         * @param start_time Time of first appearance
         * @param end_time Time of last appearance
         * @param initial_kin_E Initial kinetic energy (in MeV)
         * @param final_kin_E Final kinetic energy (in MeV)
         * @param initial_tot_E Initial total energy (in MeV)
         * @param final_tot_E Final total energy (in MeV)
         * @param initial_mom_direction Initial momentum direction
         */
        MCTrack(ROOT::Math::XYZPoint start_point,
                ROOT::Math::XYZPoint end_point,
                std::string g4_volume_start,
                std::string g4_volume_end,
                std::string g4_prod_process_name,
                int g4_prod_process_type,
                int particle_id,
                double start_time,
                double end_time,
                double initial_kin_E,
                double final_kin_E,
                double initial_tot_E,
                double final_tot_E,
                ROOT::Math::XYZVector initial_mom_direction);

        /**
         * @brief Get the point of where the track originated
         * @return Track start point
         */
        ROOT::Math::XYZPoint getStartPoint() const;

        /**
         * @brief Get the point of where the track terminated
         * @return Track end point
         */
        ROOT::Math::XYZPoint getEndPoint() const;

        /**
         * @brief Get the time of first appearance of this track
         * @return Time of appearance of this track in the global reference system
         */
        double getGlobalStartTime() const;

        /**
         * @brief Get the time of last appearance of this track
         * @return Time of disappearance of this track in the global reference system
         */
        double getGlobalEndTime() const;

        /**
         * @brief Get PDG particle id for the particle
         * @return Particle id
         */
        int getParticleID() const;

        /**
         * @brief Get the Geant4 internal ID of the process which created the particle
         * @return The Geant4 process type or "-1" if no such process exists
         */
        int getCreationProcessType() const;

        /**
         * @brief Getter for the kinetic energy the particle had when the track was created
         * @return The kinetic energy in MeV of the particle at the beginning of the track
         */
        double getKineticEnergyInitial() const;
        /**
         * @brief Getter for the total energy (i.e. kinetic energy and dynamic mass) the particle had when the track was
         * created
         * @return The total energy in MeV of the particle at the beginning of the track
         */
        double getTotalEnergyInitial() const;

        /**
         * @brief Getter for the kinetic energy the particle had when the track terminated
         * @return The kinetic energy in MeV of the particle at the end of the track
         */
        double getKineticEnergyFinal() const;

        /**
         * @brief Getter for the total energy (i.e. kinetic energy and dynamic mass) the particle had when the track
         * terminated
         * @return The total energy in MeV of the particle at the end of the track
         */
        double getTotalEnergyFinal() const;

        /**
         * @brief Getter for the momentum direction the particle had when the track was created
         * @return Particle motion direction at the beginning of the track
         */
        ROOT::Math::XYZVector getMomentumDirectionInitial() const;

        /**
         * @brief Getter for the Geant4 name of the physical volume in which the track originated
         * @return The name of the phyical volume
         */
        std::string getOriginatingVolumeName() const;

        /**
         * @brief Getter for the Geant4 name of the physical volume in which the track terminated
         * @return The name of the phyical volume
         */
        std::string getTerminatingVolumeName() const;

        /**
         * @brief Getter for the name of the process which created this particle
         * @return The process name or "none" if no such process exists
         */
        std::string getCreationProcessName() const;

        /**
         * @brief Get the parent MCTrack if it has one
         * @return Parent MCTrack or null pointer if it has no parent
         * @warning No \ref MissingReferenceException is thrown, because a track without parent should always be handled.
         */
        const MCTrack* getParent() const;

        /**
         * @brief Set the Monte-Carlo parent track
         * @param mc_track The Monte-Carlo track
         * @warning Special method because parent can only be set after creation, should not be replaced later.
         */
        void setParent(const MCTrack* mc_track);

        /**
         * @brief Print an ASCII representation of MCTrack to the given stream
         * @param out Stream to print to
         */
        void print(std::ostream& out) const override;

        /**
         * @brief ROOT class definition
         */
        ClassDefOverride(MCTrack, 7); // NOLINT
        /**
         * @brief Default constructor for ROOT I/O
         */
        MCTrack() = default;

        void loadHistory() override;
        void petrifyHistory() override;

    private:
        ROOT::Math::XYZPoint start_point_{};
        ROOT::Math::XYZPoint end_point_{};

        std::string start_g4_vol_name_{};
        std::string end_g4_vol_name_{};
        std::string origin_g4_process_name_{};

        int origin_g4_process_type_{};
        int particle_id_{};

        double global_start_time_{};
        double global_end_time_{};

        double initial_kin_E_{};
        double final_kin_E_{};
        double initial_tot_E_{};
        double final_tot_E_{};
        ROOT::Math::XYZVector initial_mom_direction_{};

        PointerWrapper<MCTrack> parent_;
    };

    /**
     * @brief Typedef for message carrying MC tracks
     */
    using MCTrackMessage = Message<MCTrack>;
} // namespace allpix

#endif /* ALLPIX_MC_TRACK_H */
