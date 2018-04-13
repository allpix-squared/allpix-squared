/**
 * @file
 * @brief Definition of Monte-Carlo track object
 * @copyright Copyright (c) 2018 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_MC_TRACK_H
#define ALLPIX_MC_TRACK_H

#include <Math/Point3D.h>
#include <TRef.h>
#include <TString.h>

#include <iostream>
#include "Object.hpp"

namespace allpix {
    /**
     * @brief Monte-Carlo track through the world
     */
    class MCTrack : public Object {
    public:
        /**
         * @brief Construct a Monte-Carlo track
         * @param start_point Global point where track came into existance
         * @param TODO
         * @param TODO
         */
        MCTrack(ROOT::Math::XYZPoint start_point,
                std::string g4_volume,
                std::string g4_prod_process_name,
                int g4_prod_process_type,
                int particle_id,
                int track_id,
                int parent_id,
                double initial_kin_E,
                double initial_tot_E);

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
         * @brief Get particle identifier
         * @return Particle identifier
         */
        int getParticleID() const;

        int getParentTrackID() const;

        void setEndPoint(ROOT::Math::XYZPoint end_point) { end_point_ = std::move(end_point); };
        void setFinalTotalEnergy(double final_tot_E) { final_tot_E_ = final_tot_E; };
        void setFinalKineticEnergy(double final_kin_E) { final_kin_E_ = final_kin_E; };
        void setNumberSteps(int n_steps) { n_steps_ = n_steps; };

        double getFinalKineticEnergy() const { return final_kin_E_; };
        /**
         * @brief Set the Monte-Carlo parent track
         * @param mc_track The Monte-Carlo track
         * @warning Special method because parent can only be set after creation, should not be replaced later.
         */
        void setParent(const MCTrack* mc_track);

        /**
         * @brief Get the parent MCTrack if it has one
         * @return Parent MCTrack or null pointer if it has no parent
         * @warning No \ref MissingReferenceException is thrown, because a track without parent should always be handled.
         */
        const MCTrack* getParent() const;

        /**
         * @brief Get unique track ID of this track
         */
        int getTrackID() const;

        friend std::ostream& operator<<(std::ostream& stream, const MCTrack& track);

        /**
         * @brief ROOT class definition
         */
        ClassDef(MCTrack, 1);
        /**
         * @brief Default constructor for ROOT I/O
         */
        MCTrack() = default;

    private:
        ROOT::Math::XYZPoint start_point_{};
        ROOT::Math::XYZPoint end_point_{};

        int particle_id_{};
        int track_id_{};
        int parent_id_{};
        int origin_g4_process_type_{};
        int n_steps_{};

        double initial_kin_E_{};
        double initial_tot_E_{};
        double final_kin_E_{};
        double final_tot_E_{};

        TString origin_g4_vol_name_{};
        TString origin_g4_process_name_{};

        TRef parent_;
    };

    /**
     * @brief Typedef for message carrying MC tracks
     */
    using MCTrackMessage = Message<MCTrack>;
} // namespace allpix

#endif
