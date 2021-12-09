/**
 * @file
 * @brief Definition of charge carrier trapping models
 *
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_TRAPPING_MODELS_H
#define ALLPIX_TRAPPING_MODELS_H

#include "exceptions.h"

#include "core/utils/unit.h"
#include "objects/SensorCharge.hpp"

namespace allpix {

    /**
     * @ingroup Models
     * @brief Charge carrier trapping models
     */
    class TrappingModel {
    public:
        /**
         * Default constructor
         */
        TrappingModel() = default;

        /**
         * Default virtual destructor
         */
        virtual ~TrappingModel() = default;

        /**
         * Function call operator to obtain trapping time for the given carrier
         * @param type Type of charge carrier (electron or hole)
         * @param fluence Fluence 1MeV-neutron equivalent
         * @param efield_mag Magnitude of the electric field
         * @param trapping_prob Current trapping probability for this charge carrier
         * @param timestep Current time step performed for the charge carrier
         * @return Trapping status and expected time of the charge carrier being trapped
         */
        virtual std::pair<bool, double>
        operator()(const CarrierType& type, double efield_mag, double trapping_prob, double timestep) const = 0;
    };

    /**
     * @ingroup Models
     * @brief No trapping
     *
     */
    class NoTrapping : virtual public TrappingModel {
    public:
        std::pair<bool, double> operator()(const CarrierType&, double, double, double, double) const override {
            return {false, 0.};
        };
    };

    /**
     * @ingroup Models
     * @brief Ljubljana / Kramberger effective trapping model for charge carriers in silicon
     *
     * Parametrization taken from https://doi.org/10.1016/S0168-9002(01)01263-3, effective trapping time from Eq. 4 with beta
     * values from Table 2 (pions/protons), temperature dependency according to Eq. 9, scaling factors kappa from Table 3.
     */
    class Ljubljana : virtual public TrappingModel {
    public:
        Ljubljana(double temperature, double fluence)
            : tau_eff_electron_(1. / Units::get(5.6e-16 * std::pow(temperature / 300, -0.86), "cm*cm/ns") / fluence),
              tau_eff_hole_(1. / Units::get(7.7e-16 * std::pow(temperature / 300, -1.52), "cm*cm/ns") / fluence) {}

        std::pair<bool, double>
        operator()(const CarrierType& type, double, double trapping_prob, double timestep) const override {
            return {trapping_prob <
                        (1 - std::exp(-1. * timestep / (type == CarrierType::ELECTRON ? tau_eff_electron_ : tau_eff_hole_))),
                    std::numeric_limits<double>::max()};
        };

    private:
        double tau_eff_electron_;
        double tau_eff_hole_;
    };

    /**
     * @ingroup Models
     * @brief Dortmund / Krasel effective trapping model for charge carriers in silicon
     *
     * Parametrization taken from https://doi.org/10.1109/TNS.2004.839096, effective trapping time from Eq. 3 with gamma
     * values from Eqs. 5 & 6
     */
    class Dortmund : virtual public TrappingModel {
    public:
        Dortmund()
            : tau_eff_electron_(1. / Units::get(5.13e-16, "cm*cm/ns")),
              tau_eff_hole_(1. / Units::get(5.04e-16, "cm*cm/ns")) {}

        std::pair<bool, double>
        operator()(const CarrierType& type, double, double trapping_prob, double timestep) const override {
            return {trapping_prob <
                        (1 - std::exp(-1. * timestep / (type == CarrierType::ELECTRON ? tau_eff_electron_ : tau_eff_hole_))),
                    std::numeric_limits<double>::max()};
        };

    private:
        double tau_eff_electron_;
        double tau_eff_hole_;
    };

    /**
     * @brief Wrapper class and factory for trapping models.
     *
     * This class allows to store trapping  objects independently of the model chosen and simplifies access to the
     * function call operator. The constructor acts as factory, generating model objects from the model name provided, e.g.
     * from a configuration file.
     */
    class Trapping {
    public:
        /**
         * Default constructor
         */
        Trapping() = default;

        /**
         * Recombination constructor
         * @param model       Name of the trapping model
         * @param temperature Temperature for which the trapping model should be initialized
         */
        Trapping(const std::string& model, double temperature, double fluence) {
            if(model == "ljubljana" || model == "kramberger") {
                model_ = std::make_unique<Ljubljana>(temperature, fluence);
            } else if(model == "dortmund" || model == "krasel") {
                model_ = std::make_unique<Dortmund>();
            } else if(model == "none") {
                LOG(INFO) << "No charge carrier trapping model chosen, no trapping simulated";
                model_ = std::make_unique<NoTrapping>();
            } else {
                throw InvalidModelError(model);
            }
            LOG(DEBUG) << "Selected trapping model \"" << model << "\"";
        }

        /**
         * Function call operator forwarded to the trapping model
         * @return Trapping time
         */
        template <class... ARGS> std::pair<bool, double> operator()(ARGS&&... args) const {
            return model_->operator()(std::forward<ARGS>(args)...);
        }

    private:
        std::unique_ptr<TrappingModel> model_{};
    };

} // namespace allpix

#endif
