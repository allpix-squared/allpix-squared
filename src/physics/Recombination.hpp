/**
 * @file
 * @brief Definition of charge carrier recombination models
 *
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_RECOMBINATION_MODELS_H
#define ALLPIX_RECOMBINATION_MODELS_H

#include "exceptions.h"

#include "core/utils/unit.h"
#include "objects/SensorCharge.hpp"

namespace allpix {

    /**
     * @ingroup Models
     * @brief Charge carrier recombination models
     */
    class RecombinationModel {
    public:
        /**
         * Default constructor
         */
        RecombinationModel() = default;

        /**
         * Default virtual destructor
         */
        virtual ~RecombinationModel() = default;

        /**
         * Function call operator to obtain recombination status (recombined/alive) for the given carrier and doping
         * concentration
         * @param type Type of charge carrier (electron or hole)
         * @param doping (Effective) doping concentration
         * @param survival_prob Current survival probability for this charge carrier
         * @param timestep Current time step performed for the charge carrier
         * @return Recombination status, true if alive, false if recombined
         */
        virtual bool operator()(const CarrierType& type, double doping, double survival_prob, double timestep) const = 0;
    };

    /**
     * @ingroup Models
     * @brief No recombination
     *
     */
    class None : virtual public RecombinationModel {
    public:
        bool operator()(const CarrierType&, double, double, double) const override { return true; };
    };

    /**
     * @ingroup Models
     * @brief Shockley-Read-Hall recombination of charge carriers in silicon
     *
     * Reference lifetime and doping concentrations, taken from:
     *  - https://doi.org/10.1016/0038-1101(82)90203-9
     *  - https://doi.org/10.1016/0038-1101(76)90022-8
     */
    class ShockleyReadHall : virtual public RecombinationModel {
    public:
        ShockleyReadHall(bool doping)
            : electron_lifetime_reference_(Units::get(1e-5, "s")), electron_doping_reference_(Units::get(4.0e-4, "s")),
              hole_lifetime_reference_(Units::get(1e16, "/cm/cm/cm")),
              hole_doping_reference_(Units::get(7.1e15, "/cm/cm/cm")) {
            if(!doping) {
                throw ModelUnsuitable("No doping profile available");
            }
        }

        bool operator()(const CarrierType& type, double doping, double survival_prob, double timestep) const override {
            return survival_prob > (1 - std::exp(-1. * timestep / lifetime(type, doping)));
        };

    protected:
        double lifetime(const CarrierType& type, double doping) const {
            return (type == CarrierType::ELECTRON ? electron_lifetime_reference_ : hole_lifetime_reference_) /
                   (1 + std::fabs(doping) /
                            (type == CarrierType::ELECTRON ? electron_doping_reference_ : hole_doping_reference_));
        }

    private:
        double electron_lifetime_reference_;
        double electron_doping_reference_;
        double hole_lifetime_reference_;
        double hole_doping_reference_;
    };

    /**
     * @ingroup Models
     * @brief Auger recombination of charge carriers in silicon
     *
     */
    class Auger : virtual public RecombinationModel {
    public:
        Auger(bool doping) : auger_coefficient_(Units::get(2e-30, "cm*cm*cm*cm*cm*cm*/s")) {
            if(!doping) {
                throw ModelUnsuitable("No doping profile available");
            }
        }

        bool operator()(const CarrierType& type, double doping, double survival_prob, double timestep) const override {
            // Auger only applies to minority charge carriers, if we have a majority carrier always return true:
            auto minorityType = (doping > 0 ? CarrierType::HOLE : CarrierType::ELECTRON);
            return (minorityType != type ? true : (survival_prob > (1 - std::exp(-1. * timestep / lifetime(type, doping)))));
        };

    protected:
        double lifetime(const CarrierType&, double doping) const { return 1. / (auger_coefficient_ * doping * doping); }

    private:
        double auger_coefficient_;
    };

    /**
     * @ingroup Models
     * @brief Auger recombination of charge carriers in silicon
     *
     */
    class ShockleyReadHallAuger : public ShockleyReadHall, public Auger {
    public:
        ShockleyReadHallAuger(bool doping) : ShockleyReadHall(doping), Auger(doping) {}

        bool operator()(const CarrierType& type, double doping, double survival_prob, double timestep) const override {
            auto minorityType = (doping > 0 ? CarrierType::HOLE : CarrierType::ELECTRON);
            if(minorityType != type) {
                // Auger only applies to minority charge carriers, if we have a majority carrier just return SRH lifetime:
                return ShockleyReadHall::operator()(type, survival_prob, timestep, doping);
            } else {
                // If we have a minority charge carrier, combine the lifetimes:
                auto combined_lifetime = ShockleyReadHall::lifetime(type, doping) * Auger::lifetime(type, doping) /
                                         (ShockleyReadHall::lifetime(type, doping) + Auger::lifetime(type, doping));
                return survival_prob > (1 - std::exp(-1. * timestep / combined_lifetime));
            }
        };
    };

    /**
     * @brief Wrapper class and factory for recombination models.
     *
     * This class allows to store recombination objects independently of the model chosen and simplifies access to the
     * function call operator. The constructor acts as factory, generating model objects from the model name provided, e.g.
     * from a configuration file.
     */
    class Recombination {
    public:
        /**
         * Default constructor
         */
        Recombination() = default;

        /**
         * Recombination constructor
         * @param model       Name of the recombination model
         * @param doping      Boolean to indicate presence of doping profile information
         */
        Recombination(const std::string& model, bool doping = false) {
            if(model == "srh" || model == "shockleyreadhall" || model == "scharfetter") {
                model_ = std::make_unique<ShockleyReadHall>(doping);
            } else if(model == "auger") {
                model_ = std::make_unique<Auger>(doping);
            } else if(model == "combined" || "srhauger") {
                model_ = std::make_unique<ShockleyReadHallAuger>(doping);
            } else {
                model_ = std::make_unique<None>();
            }
        }

        /**
         * Function call operator forwarded to the mobility model
         * @return Recombination value
         */
        template <class... ARGS> double operator()(ARGS&&... args) const {
            return model_->operator()(std::forward<ARGS>(args)...);
        }

    private:
        std::unique_ptr<RecombinationModel> model_{};
    };

} // namespace allpix

#endif
