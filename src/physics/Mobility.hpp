/**
 * @file
 * @brief Definition of mobility models
 *
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_MOBILITY_MODELS_H
#define ALLPIX_MOBILITY_MODELS_H

#include "core/utils/unit.h"
#include "objects/SensorCharge.hpp"

namespace allpix {

    /**
     * @ingroup Models
     * @brief Charge carrier mobility models
     */
    class MobilityModel {
    public:
        /**
         * Default constructor
         */
        MobilityModel() = default;

        /**
         * Function call operator to obtain mobility value for the given carrier type and electric field magnitude
         * @param type Type of charge carrier (electron or hole)
         * @param efield_mag Magnitude of the electric field
         * @return Mobility of the charge carrier
         */
        virtual double operator()(const CarrierType& type, double efield_mag) const = 0;
    };

    /**
     * @ingroup Models
     * @brief Jacoboni/Canali mobility model for charge carriers in silicon
     *
     * Parameterization variables from https://doi.org/10.1016/0038-1101(77)90054-5 (section 5.2). All parameters are taken
     * from Table 5.
     */
    class JacoboniCanali : public MobilityModel {
    public:
        JacoboniCanali(double temperature)
            : electron_Vm_(Units::get(1.53e9 * std::pow(temperature, -0.87), "cm/s")),
              electron_Ec_(Units::get(1.01 * std::pow(temperature, 1.55), "V/cm")),
              electron_Beta_(2.57e-2 * std::pow(temperature, 0.66)),
              hole_Vm_(Units::get(1.62e8 * std::pow(temperature, -0.52), "cm/s")),
              hole_Ec_(Units::get(1.24 * std::pow(temperature, 1.68), "V/cm")),
              hole_Beta_(0.46 * std::pow(temperature, 0.17)) {}

        double operator()(const CarrierType& type, double efield_mag) const override {
            // Compute carrier mobility from constants and electric field magnitude
            if(type == CarrierType::ELECTRON) {
                return electron_Vm_ / electron_Ec_ /
                       std::pow(1. + std::pow(efield_mag / electron_Ec_, electron_Beta_), 1.0 / electron_Beta_);
            } else {
                return hole_Vm_ / hole_Ec_ / std::pow(1. + std::pow(efield_mag / hole_Ec_, hole_Beta_), 1.0 / hole_Beta_);
            }
        };

    protected:
        double electron_Vm_;

    private:
        double electron_Ec_;
        double electron_Beta_;
        double hole_Vm_;
        double hole_Ec_;
        double hole_Beta_;
    };

    /**
     * @ingroup Models
     * @brief Canali mobility model
     *
     * This model differs from the Jacoboni version only by the value of the electron v_m. The difference is most likely a
     * typo in the Jacoboni reproduction of the parametrization, so this one can be considered the "priginal".
     */
    class Canali2 : public JacoboniCanali {
    public:
        Canali2(double temperature) : JacoboniCanali(temperature) {
            electron_Vm_ = Units::get(1.43e9 * std::pow(temperature, -0.87), "cm/s");
        }
    };

    /**
     * @brief Wrapper class and factory for mobility models.
     *
     * This class allows to store mobility objects independently of the model chosen and simplifies access to the function
     * call operator. The constructor acts as factory, generating model objects from the model name provided, e.g. from a
     * configuration file.
     */
    class Mobility {
    public:
        /**
         * Default constructor
         */
        Mobility() = default;

        /**
         * Mobility constructor
         * @param model       Name of the mobility model
         * @param temperature Temperature for which the mobility model should be initialized
         */
        Mobility(const std::string& model, double temperature) {
            if(model == "jacoboni") {
                model_ = std::make_unique<JacoboniCanali>(temperature);
            }
        }

        /**
         * Function call operator forwarded to the mobility model
         * @return Mobility value
         */
        template <class... ARGS> double operator()(ARGS&&... args) const {
            return model_->operator()(std::forward<ARGS>(args)...);
        }

    private:
        std::unique_ptr<MobilityModel> model_{};
    };

} // namespace allpix

#endif
