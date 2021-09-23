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

#include "exceptions.h"

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
         * Default virtual destructor
         */
        virtual ~MobilityModel() = default;

        /**
         * Function call operator to obtain mobility value for the given carrier type and electric field magnitude
         * @param type Type of charge carrier (electron or hole)
         * @param efield_mag Magnitude of the electric field
         * @param doping (Effective) doping concentration
         * @return Mobility of the charge carrier
         */
        virtual double operator()(const CarrierType& type, double efield_mag, double doping) const = 0;
    };

    /**
     * @ingroup Models
     * @brief Jacoboni/Canali mobility model for charge carriers in silicon
     *
     * Parameterization variables from https://doi.org/10.1016/0038-1101(77)90054-5 (section 5.2). All parameters are taken
     * from Table 5.
     */
    class JacoboniCanali : virtual public MobilityModel {
    public:
        JacoboniCanali(double temperature)
            : electron_Vm_(Units::get(1.53e9 * std::pow(temperature, -0.87), "cm/s")),
              electron_Beta_(2.57e-2 * std::pow(temperature, 0.66)),
              hole_Vm_(Units::get(1.62e8 * std::pow(temperature, -0.52), "cm/s")),
              hole_Beta_(0.46 * std::pow(temperature, 0.17)),
              electron_Ec_(Units::get(1.01 * std::pow(temperature, 1.55), "V/cm")),
              hole_Ec_(Units::get(1.24 * std::pow(temperature, 1.68), "V/cm")) {}

        double operator()(const CarrierType& type, double efield_mag, double) const override {
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
        double electron_Beta_;
        double hole_Vm_;
        double hole_Beta_;

    private:
        double electron_Ec_;
        double hole_Ec_;
    };

    /**
     * @ingroup Models
     * @brief Canali mobility model
     *
     * This model differs from the Jacoboni version only by the value of the electron v_m. The difference is most likely a
     * typo in the Jacoboni reproduction of the parametrization, so this one can be considered the "original".
     */
    class Canali : virtual public JacoboniCanali {
    public:
        Canali(double temperature) : JacoboniCanali(temperature) {
            electron_Vm_ = Units::get(1.43e9 * std::pow(temperature, -0.87), "cm/s");
        }
    };

    /**
     * @ingroup Models
     * @brief Hamburg (Klanner-Scharf) parametrization for <100> silicon
     *
     * http://dx.doi.org/10.1016/j.nima.2015.07.057
     * This implementation takes the parameters from Table 4. No temperature dependence is assumed on hole mobility parameter
     * c, all other parameters are the reference values at 300K and are scaled according to Equation (6).
     */
    class Hamburg : public MobilityModel {
    public:
        Hamburg(double temperature)
            : electron_mu0_(Units::get(1530 * std::pow(temperature / 300, -2.42), "cm*cm/V/s")),
              electron_vsat_(Units::get(1.03e7 * std::pow(temperature / 300, -0.226), "cm/s")),
              hole_mu0_(Units::get(464 * std::pow(temperature / 300, -2.20), "cm*cm/V/s")),
              hole_param_b_(Units::get(9.57e-8 * std::pow(temperature / 300, -0.101), "s/cm")),
              hole_param_c_(Units::get(-3.31e-13, "s/V")),
              hole_E0_(Units::get(2640 * std::pow(temperature / 300, 0.526), "V/cm")) {}

        double operator()(const CarrierType& type, double efield_mag, double) const override {
            if(type == CarrierType::ELECTRON) {
                // Equation (3) of the reference, setting E0 = 0 as suggested
                return 1. / (1. / electron_mu0_ + 1. / electron_vsat_ * efield_mag);
            } else {
                // Equation (5) of the reference
                if(efield_mag < hole_E0_) {
                    return hole_mu0_;
                } else {
                    return 1. / (1. / hole_mu0_ + hole_param_b_ * (efield_mag - hole_E0_) +
                                 hole_param_c_ * (efield_mag - hole_E0_) * (efield_mag - hole_E0_));
                }
            }
        };

    protected:
        double electron_mu0_;
        double electron_vsat_;
        double hole_mu0_;
        double hole_param_b_;
        double hole_param_c_;
        double hole_E0_;
    };

    /**
     * @ingroup Models
     * @brief Hamburg (Klanner-Scharf) high-field parametrization for <100> silicon
     *
     * http://dx.doi.org/10.1016/j.nima.2015.07.057
     * This implementation takes the parameters from Table 3, suitable for electric field strengths above 2.5kV/cm. No
     * temperature dependence is assumed on hole mobility parameter c, all other parameters are the reference values at 300K
     * and are scaled according to Equation (6).
     */
    class HamburgHighField : public Hamburg {
    public:
        HamburgHighField(double temperature) : Hamburg(temperature) {
            electron_mu0_ = Units::get(1430 * std::pow(temperature / 300, -1.99), "cm*cm/V/s");
            electron_vsat_ = Units::get(1.05e7 * std::pow(temperature / 300, -0.302), "cm/s");
            hole_mu0_ = Units::get(457 * std::pow(temperature / 300, -2.80), "cm*cm/V/s");
            hole_param_b_ = Units::get(9.57e-8 * std::pow(temperature / 300, -0.155), "s/cm"),
            hole_param_c_ = Units::get(-3.24e-13, "s/V");
            hole_E0_ = Units::get(2970 * std::pow(temperature / 300, 0.563), "V/cm");
        }
    };

    /**
     * @ingroup Models
     * @brief Masetti mobility model for charge carriers in silicon
     *
     * Parameterization variables from https://doi.org/10.1109/T-ED.1983.21207, formulae (1) for electrons and (4) for holes.
     * The values are taken from Table I, for Arsenic and Boron
     */
    class Masetti : virtual public MobilityModel {
    public:
        Masetti(double temperature, bool doping)
            : electron_mu0_(Units::get(52.2, "cm*cm/V/s")),
              electron_mumax_(Units::get(1417, "cm*cm/V/s") * std::pow(temperature / 300, -2.5)),
              electron_cr_(Units::get(9.68e16, "/cm/cm/cm")), electron_alpha_(0.68),
              electron_mu1_(Units::get(43.4, "cm*cm/V/s")), electron_cs_(Units::get(3.43e20, "/cm/cm/cm")),
              electron_beta_(2.0), hole_mu0_(Units::get(44.9, "cm*cm/V/s")), hole_pc_(Units::get(9.23e16, "/cm/cm/cm")),
              hole_mumax_(Units::get(470.5, "cm*cm/V/s") * std::pow(temperature / 300, -2.2)),
              hole_cr_(Units::get(2.23e17, "/cm/cm/cm")), hole_alpha_(0.719), hole_mu1_(Units::get(29.0, "cm*cm/V/s")),
              hole_cs_(Units::get(6.1e20, "/cm/cm/cm")), hole_beta_(2.0) {
            if(!doping) {
                throw ModelUnsuitable("No doping profile available");
            }
        }

        double operator()(const CarrierType& type, double, double doping) const override {
            if(type == CarrierType::ELECTRON) {
                return electron_mu0_ +
                       (electron_mumax_ - electron_mu0_) /
                           (1. + std::pow(std::fabs(doping) / electron_cr_, electron_alpha_)) -
                       electron_mu1_ / (1. + std::pow(electron_cs_ / std::fabs(doping), electron_beta_));
            } else {
                return hole_mu0_ * std::exp(-hole_pc_ / std::fabs(doping)) +
                       hole_mumax_ / (1. + std::pow(std::fabs(doping) / hole_cr_, hole_alpha_)) -
                       hole_mu1_ / (1. + std::pow(hole_cs_ / std::fabs(doping), hole_beta_));
            }
        };

    private:
        double electron_mu0_;
        double electron_mumax_;
        double electron_cr_;
        double electron_alpha_;
        double electron_mu1_;
        double electron_cs_;
        double electron_beta_;
        double hole_mu0_;
        double hole_pc_;
        double hole_mumax_;
        double hole_cr_;
        double hole_alpha_;
        double hole_mu1_;
        double hole_cs_;
        double hole_beta_;
    };

    /**
     * @ingroup Models
     * @brief Combination of the Masetti and Canali mobility models for charge carriers in silicon ("extended Canali model")
     *
     * Based on the combination of the models as implemented in Synopsys Sentaurus TCAD
     */
    class MasettiCanali : public Canali, public Masetti {
    public:
        MasettiCanali(double temperature, bool doping)
            : JacoboniCanali(temperature), Canali(temperature), Masetti(temperature, doping) {}

        double operator()(const CarrierType& type, double efield_mag, double doping) const override {
            double masetti = Masetti::operator()(type, efield_mag, doping);

            if(type == CarrierType::ELECTRON) {
                return masetti /
                       std::pow(1. + std::pow(masetti * efield_mag / electron_Vm_, electron_Beta_), 1. / electron_Beta_);
            } else {
                return masetti / std::pow(1. + std::pow(masetti * efield_mag / hole_Vm_, hole_Beta_), 1. / hole_Beta_);
            }
        };
    };

    /**
     * @ingroup Models
     * @brief Arora mobility model for charge carriers in silicon
     *
     * Parameterization variables from https://doi.org/10.1109/T-ED.1982.20698 (values from Table 1, formulae 8 for electrons
     * and 13 for holes)
     */
    class Arora : public MobilityModel {
    public:
        Arora(double temperature, bool doping)
            : electron_mumin_(Units::get(88 * std::pow(temperature / 300, -0.57), "cm*cm/V/s")),
              electron_mu0_(Units::get(7.4e8 * std::pow(temperature, -2.33), "cm*cm/V/s")),
              electron_nref_(Units::get(1.26e17 * std::pow(temperature / 300, 2.4), "/cm/cm/cm")),
              hole_mumin_(Units::get(54.3 * std::pow(temperature / 300, -0.57), "cm*cm/V/s")),
              hole_mu0_(Units::get(1.36e8 * std::pow(temperature, -2.23), "cm*cm/V/s")),
              hole_nref_(Units::get(2.35e17 * std::pow(temperature / 300, 2.4), "/cm/cm/cm")),
              alpha_(0.88 * std::pow(temperature / 300, -0.146)) {
            if(!doping) {
                throw ModelUnsuitable("No doping profile available");
            }
        }

        double operator()(const CarrierType& type, double, double doping) const override {
            if(type == CarrierType::ELECTRON) {
                return electron_mumin_ + electron_mu0_ / (1 + std::pow(std::fabs(doping) / electron_nref_, alpha_));
            } else {
                return hole_mumin_ + hole_mu0_ / (1 + std::pow(std::fabs(doping) / hole_nref_, alpha_));
            }
        };

    private:
        double electron_mumin_;
        double electron_mu0_;
        double electron_nref_;
        double hole_mumin_;
        double hole_mu0_;
        double hole_nref_;
        double alpha_;
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
         * @param doping      Boolean to indicate presence of doping profile information
         */
        Mobility(const std::string& model, double temperature, bool doping = false) {
            if(model == "jacoboni") {
                model_ = std::make_unique<JacoboniCanali>(temperature);
            } else if(model == "canali") {
                model_ = std::make_unique<Canali>(temperature);
            } else if(model == "hamburg") {
                model_ = std::make_unique<Hamburg>(temperature);
            } else if(model == "hamburg_highfield") {
                model_ = std::make_unique<HamburgHighField>(temperature);
            } else if(model == "masetti") {
                model_ = std::make_unique<Masetti>(temperature, doping);
            } else if(model == "masetti_canali") {
                model_ = std::make_unique<MasettiCanali>(temperature, doping);
            } else if(model == "arora") {
                model_ = std::make_unique<Arora>(temperature, doping);
            } else {
                throw InvalidModelError(model);
            }
            LOG(DEBUG) << "Selected mobility model \"" << model << "\"";
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
