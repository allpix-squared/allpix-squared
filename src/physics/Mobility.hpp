/**
 * @file
 * @brief Definition of mobility models
 *
 * @copyright Copyright (c) 2021-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_MOBILITY_MODELS_H
#define ALLPIX_MOBILITY_MODELS_H

#include <TFormula.h>

#include "exceptions.h"

#include "core/config/Configuration.hpp"
#include "core/geometry/DetectorModel.hpp"
#include "core/utils/log.h"
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
        explicit JacoboniCanali(SensorMaterial material, double temperature)
            : electron_Vm_(Units::get(1.53e9 * std::pow(temperature, -0.87), "cm/s")),
              electron_Beta_(2.57e-2 * std::pow(temperature, 0.66)),
              hole_Vm_(Units::get(1.62e8 * std::pow(temperature, -0.52), "cm/s")),
              hole_Beta_(0.46 * std::pow(temperature, 0.17)),
              electron_Ec_(Units::get(1.01 * std::pow(temperature, 1.55), "V/cm")),
              hole_Ec_(Units::get(1.24 * std::pow(temperature, 1.68), "V/cm")) {
            if(material != SensorMaterial::SILICON) {
                LOG(WARNING) << "Sensor material " << allpix::to_string(material) << " not valid for this model.";
            }
        }

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
        explicit Canali(SensorMaterial material, double temperature) : JacoboniCanali(material, temperature) {
            electron_Vm_ = Units::get(1.43e9 * std::pow(temperature, -0.87), "cm/s");
        }
    };

    /**
     * @ingroup Models
     * @brief Canali mobility model
     *
     * This model differs from the Jacoboni version only by the value of the electron v_m. The difference is most likely a
     * typo in the Jacoboni reproduction of the parametrization, so this one can be considered the "original".
     */
    class CanaliFast : virtual public Canali {
    public:
        explicit CanaliFast(SensorMaterial material, double temperature)
            : JacoboniCanali(material, temperature), Canali(material, temperature) {
            LOG(WARNING) << "This mobility model uses an approximative pow implementation and might be less accurate.";
        }

        double operator()(const CarrierType& type, double efield_mag, double) const override {
            // Compute carrier mobility from constants and electric field magnitude
            if(type == CarrierType::ELECTRON) {
                return electron_Vm_ / electron_Ec_ /
                       fastPow(1. + fastPow(efield_mag / electron_Ec_, electron_Beta_), 1.0 / electron_Beta_);
            } else {
                return hole_Vm_ / hole_Ec_ / fastPow(1. + fastPow(efield_mag / hole_Ec_, hole_Beta_), 1.0 / hole_Beta_);
            }
        };

    private:
        // Fast approximative pow implementation from
        // https://martin.ankerl.com/2012/01/25/optimized-approximative-pow-in-c-and-cpp/
        inline double fastPow(double a, double b) const {
            union {
                double d;
                int x[2];
            } u = {a};
            u.x[1] = static_cast<int>(b * (u.x[1] - 1072632447) + 1072632447);
            u.x[0] = 0;
            return u.d;
        };
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
        explicit Hamburg(SensorMaterial material, double temperature)
            : electron_mu0_(Units::get(1530 * std::pow(temperature / 300, -2.42), "cm*cm/V/s")),
              electron_vsat_(Units::get(1.03e7 * std::pow(temperature / 300, -0.226), "cm/s")),
              hole_mu0_(Units::get(464 * std::pow(temperature / 300, -2.20), "cm*cm/V/s")),
              hole_param_b_(Units::get(9.57e-8 * std::pow(temperature / 300, -0.101), "s/cm")),
              hole_param_c_(Units::get(-3.31e-13, "s/V")),
              hole_E0_(Units::get(2640 * std::pow(temperature / 300, 0.526), "V/cm")) {
            if(material != SensorMaterial::SILICON) {
                LOG(WARNING) << "Sensor material " << allpix::to_string(material) << " not valid for this model.";
            }
        }

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
        explicit HamburgHighField(SensorMaterial material, double temperature) : Hamburg(material, temperature) {
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
     * The values are taken from Table I, for Phosphorus and Boron
     */
    class Masetti : virtual public MobilityModel {
    public:
        Masetti(SensorMaterial material, double temperature, bool doping)
            : electron_mu0_(Units::get(68.5, "cm*cm/V/s")),
              electron_mumax_(Units::get(1414, "cm*cm/V/s") * std::pow(temperature / 300, -2.5)),
              electron_cr_(Units::get(9.20e16, "/cm/cm/cm")), electron_alpha_(0.711),
              electron_mu1_(Units::get(56.1, "cm*cm/V/s")), electron_cs_(Units::get(3.41e20, "/cm/cm/cm")),
              electron_beta_(1.98), hole_mu0_(Units::get(44.9, "cm*cm/V/s")), hole_pc_(Units::get(9.23e16, "/cm/cm/cm")),
              hole_mumax_(Units::get(470.5, "cm*cm/V/s") * std::pow(temperature / 300, -2.2)),
              hole_cr_(Units::get(2.23e17, "/cm/cm/cm")), hole_alpha_(0.719), hole_mu1_(Units::get(29.0, "cm*cm/V/s")),
              hole_cs_(Units::get(6.1e20, "/cm/cm/cm")), hole_beta_(2.0) {
            if(!doping) {
                throw ModelUnsuitable("No doping profile available");
            }
            if(material != SensorMaterial::SILICON) {
                LOG(WARNING) << "Sensor material " << allpix::to_string(material) << " not valid for this model.";
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
        MasettiCanali(SensorMaterial material, double temperature, bool doping)
            : JacoboniCanali(material, temperature), Canali(material, temperature), Masetti(material, temperature, doping) {}

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
        Arora(SensorMaterial material, double temperature, bool doping)
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
            if(material != SensorMaterial::SILICON) {
                LOG(WARNING) << "Sensor material " << allpix::to_string(material) << " not valid for this model.";
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
     * @ingroup Models
     * @brief Ruch-Kino mobility model for charge carriers in GaAs:Cr
     *
     * Model from https://doi.org/10.1103/PhysRev.174.921
     * Parameterization variables from https://10.1088/1748-0221/15/03/c03013
     */
    class RuchKino : virtual public MobilityModel {
    public:
        explicit RuchKino(SensorMaterial material)
            : E0_gaas_(Units::get(3100.0, "V/cm")), mu_e_gaas_(Units::get(7600.0, "cm*cm/V/s")),
              Ec_gaas_(Units::get(1360.0, "V/cm")), mu_h_gaas_(Units::get(320.0, "cm*cm/V/s")) {
            if(material != SensorMaterial::GALLIUM_ARSENIDE) {
                LOG(WARNING) << "Sensor material " << allpix::to_string(material) << " not valid for this model.";
            }
        }

        double operator()(const CarrierType& type, double efield_mag, double) const override {
            // Compute carrier mobility from constants and electric field magnitude
            if(type == CarrierType::ELECTRON) {
                if(efield_mag < E0_gaas_) {
                    return mu_e_gaas_;
                } else {
                    return mu_e_gaas_ / sqrt(1 + std::pow((efield_mag - E0_gaas_), 2) / std::pow(Ec_gaas_, 2));
                }
            } else {
                return mu_h_gaas_;
            }
        };

    private:
        double E0_gaas_;
        double mu_e_gaas_;
        double Ec_gaas_;
        double mu_h_gaas_;
    };

    /*
     * @ingroup Models
     * @brief Quay mobility model for charge carriers in different semiconductor materials
     *
     * Quay (https://doi.org/10.1016/0038-1101(87)90063-3) uses a parametrization of the saturation velocity VSat taken from
     * https://doi.org/10.1016/S1369-8001(00)00015-9. This parametrisation has the advantage that it can be applied to most
     * common semiconductor materials. The mobility is a function of VSat and the critical field Ec, which is defined as Vsat
     * divided by the mobility at zero field:
     *
     *     Ec = Vsat / mu_zero
     *
     * The mobility at zero field is parametrised as
     *
     *     mu_zero = alpha * temperature^-p.
     *
     * The parameters alpha (mobility at 0K) and p have to be taken from parametrizations to data. Landolt-Bornstein - Group
     * III Condensed Matter (https://doi.org/10.1007/b80447) provides a good general summary of the parameters.
     *
     * N.B: the parameter p is generally between 1.5 and 2.3, translating theoretical predictions that mobility at low
     * temperatures (below ~100K) generally goes as T^-1.5 dominated by acoustic phonon scattering, while the mobility at
     * higher temperatures (which goes as T^-2.3) has a contribution from both acoustical and optical phonon scattering.
     */
    class Quay : public MobilityModel {
    public:
        Quay(SensorMaterial material, double temperature) {
            if(material == SensorMaterial::SILICON) {
                electron_Vsat_ = vsat(Units::get(1.02e7, "cm/s"), 0.74, temperature);
                hole_Vsat_ = vsat(Units::get(0.72e7, "cm/s"), 0.37, temperature);

                // parameters for mobility at zero field defined in Jacoboni et al
                // https://doi.org/10.1016/0038-1101(77)90054-5
                electron_Ec_ = electron_Vsat_ / (Units::get(1.43e9, "cm*cm*K/V/s") / std::pow(temperature, 2.42));
                hole_Ec_ = hole_Vsat_ / (Units::get(1.35e8, "cm*cm*K/V/s") / std::pow(temperature, 2.20));
            } else if(material == SensorMaterial::GERMANIUM) {
                electron_Vsat_ = vsat(Units::get(0.7e7, "cm/s"), 0.45, temperature);
                hole_Vsat_ = vsat(Units::get(0.63e7, "cm/s"), 0.39, temperature);

                // Parameters for mobility at zero field defined in Omar et al for electrons
                // https://doi.org/10.1016/0038-1101(87)90063-3 and in Landolt-Bornstein - Group III Condensed Matter
                // https://doi.org/10.1007/b80447 for holes
                electron_Ec_ = electron_Vsat_ / (Units::get(5.66e7, "cm*cm*K/V/s") / std::pow(temperature, 1.68));
                hole_Ec_ = hole_Vsat_ / (Units::get(1.05e9, "cm*cm*K/V/s") / std::pow(temperature, 2.33));

            } else if(material == SensorMaterial::GALLIUM_ARSENIDE) {
                electron_Vsat_ = vsat(Units::get(0.72e7, "cm/s"), 0.44, temperature);
                hole_Vsat_ = vsat(Units::get(0.9e7, "cm/s"), 0.59, temperature);

                electron_Ec_ = electron_Vsat_ / (Units::get(2.5e6, "cm*cm*K/V/s") / std::pow(temperature, 1.));
                hole_Ec_ = hole_Vsat_ / (Units::get(6.3e7, "cm*cm*K/V/s") / std::pow(temperature, 2.1));
            } else {
                throw ModelUnsuitable("Sensor material " + allpix::to_string(material) + " not valid for this model.");
            }
        };

        double operator()(const CarrierType& type, double efield_mag, double) const override {
            if(type == CarrierType::ELECTRON) {
                return electron_Vsat_ / electron_Ec_ / std::sqrt(1. + efield_mag * efield_mag / electron_Ec_ / electron_Ec_);
            } else {
                return hole_Vsat_ / hole_Ec_ / std::sqrt(1. + efield_mag * efield_mag / hole_Ec_ / hole_Ec_);
            }
        };

    private:
        double vsat(double vsat300, double a, double temperature) const {
            return vsat300 / ((1 - a) + a * (temperature / 300));
        };

        double electron_Vsat_;
        double hole_Vsat_;
        double electron_Ec_;
        double hole_Ec_;
    };

    /**
     * @ingroup Models
     * @brief Levinshtein mobility models for charge carriers in gallium nitride
     *
     * Model and parameters are based on https://doi.org/10.1016/S0038-1101(02)00256-3
     */
    class Levinshtein : public MobilityModel {
    public:
        Levinshtein(SensorMaterial material, double temperature, bool doping)
            : electron_mumin_(Units::get(55, "cm*cm/V/s")), electron_mumax_(Units::get(1000, "cm*cm/V/s")),
              electron_nref_(Units::get(2e17, "/cm/cm/cm")), electron_t_alpha_(std::pow(temperature / 300, 2.)),
              electron_t_beta_(std::pow(temperature / 300, 0.7)), electron_gamma_(1.),
              hole_mumin_(Units::get(3, "cm*cm/V/s")), hole_mumax_(Units::get(170, "cm*cm/V/s")),
              hole_nref_(Units::get(3e17, "/cm/cm/cm")), hole_t_alpha_(std::pow(temperature / 300, 5.)), hole_gamma_(2.) {
            if(!doping) {
                throw ModelUnsuitable("No doping profile available");
            }
            if(material != SensorMaterial::GALLIUM_NITRIDE) {
                LOG(WARNING) << "Sensor material " << allpix::to_string(material) << " not valid for this model.";
            }
        }

        double operator()(const CarrierType& type, double temperature, double doping) const override {
            if(type == CarrierType::ELECTRON) {
                double B =
                    (electron_mumin_ + electron_mumax_ * std::pow(electron_nref_ / std::fabs(doping), electron_gamma_)) /
                    (electron_mumax_ - electron_mumin_);
                return electron_mumax_ / (1 / (B * electron_t_beta_) + electron_t_alpha_);
            } else {
                double B = (hole_mumin_ + hole_mumax_ * std::pow(hole_nref_ / std::fabs(doping), hole_gamma_)) /
                           (hole_mumax_ - hole_mumin_);
                return hole_mumax_ / (1 / B + std::pow(temperature / 300, hole_t_alpha_));
            }
        };

    private:
        double electron_mumin_;
        double electron_mumax_;
        double electron_nref_;
        double electron_t_alpha_;
        double electron_t_beta_;
        double electron_gamma_;
        double hole_mumin_;
        double hole_mumax_;
        double hole_nref_;
        double hole_t_alpha_;
        double hole_gamma_;
    };

    /**
     * @ingroup Models
     * @brief Constant mobility of electrons and holes
     */
    class ConstantMobility : public MobilityModel {
    public:
        ConstantMobility(double electron_mobility, double hole_mobility)
            : electron_mobility_(electron_mobility), hole_mobility_(hole_mobility) {}

        double operator()(const CarrierType& type, double, double) const override {
            return (type == CarrierType::ELECTRON ? electron_mobility_ : hole_mobility_);
        };

    private:
        double electron_mobility_;
        double hole_mobility_;
    };

    /**
     * @ingroup Models
     * @brief Custom mobility model for charge carriers
     */
    class Custom : public MobilityModel {
    public:
        Custom(const Configuration& config, bool doping) {
            electron_mobility_ = configure_mobility(config, CarrierType::ELECTRON, doping);
            hole_mobility_ = configure_mobility(config, CarrierType::HOLE, doping);
        };

        double operator()(const CarrierType& type, double efield_mag, double doping) const override {
            if(type == CarrierType::ELECTRON) {
                return electron_mobility_->Eval(efield_mag, doping);
            } else {
                return hole_mobility_->Eval(efield_mag, doping);
            }
        };

    private:
        std::unique_ptr<TFormula> electron_mobility_;
        std::unique_ptr<TFormula> hole_mobility_;

        std::unique_ptr<TFormula> configure_mobility(const Configuration& config, const CarrierType type, bool doping) {
            std::string name = (type == CarrierType::ELECTRON ? "electrons" : "holes");
            auto function = config.get<std::string>("mobility_function_" + name);
            auto parameters = config.getArray<double>("mobility_parameters_" + name, {});

            auto mobility = std::make_unique<TFormula>(("mobility_" + name).c_str(), function.c_str());

            if(!mobility->IsValid()) {
                throw InvalidValueError(
                    config, "mobility_function_" + name, "The provided model is not a valid ROOT::TFormula expression");
            }

            // Check if a doping concentration dependency can be detected by checking for the number of dimensions:
            if(!doping && mobility->GetNdim() == 2) {
                throw ModelUnsuitable("No doping profile available but doping dependence found");
            }

            // Check if number of parameters match up
            if(static_cast<size_t>(mobility->GetNpar()) != parameters.size()) {
                throw InvalidValueError(config,
                                        "mobility_parameters_" + name,
                                        "The number of provided parameters and parameters in the function do not match");
            }

            // Set the parameters
            for(size_t n = 0; n < parameters.size(); ++n) {
                mobility->SetParameter(static_cast<int>(n), parameters[n]);
            }

            return mobility;
        };
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
         * @param config    Configuration of the calling module
         * @param material  Material of the sensor in question
         * @param doping    Boolean to indicate presence of doping profile information
         */
        explicit Mobility(const Configuration& config, SensorMaterial material, bool doping = false) {
            try {
                auto model = config.get<std::string>("mobility_model");
                auto temperature = config.get<double>("temperature");
                if(model == "jacoboni") {
                    model_ = std::make_unique<JacoboniCanali>(material, temperature);
                } else if(model == "canali") {
                    model_ = std::make_unique<Canali>(material, temperature);
                } else if(model == "canali_fast") {
                    model_ = std::make_unique<CanaliFast>(material, temperature);
                } else if(model == "hamburg") {
                    model_ = std::make_unique<Hamburg>(material, temperature);
                } else if(model == "hamburg_highfield") {
                    model_ = std::make_unique<HamburgHighField>(material, temperature);
                } else if(model == "masetti") {
                    model_ = std::make_unique<Masetti>(material, temperature, doping);
                } else if(model == "masetti_canali") {
                    model_ = std::make_unique<MasettiCanali>(material, temperature, doping);
                } else if(model == "arora") {
                    model_ = std::make_unique<Arora>(material, temperature, doping);
                } else if(model == "ruch_kino") {
                    model_ = std::make_unique<RuchKino>(material);
                } else if(model == "quay") {
                    model_ = std::make_unique<Quay>(material, temperature);
                } else if(model == "levinshtein") {
                    model_ = std::make_unique<Levinshtein>(material, temperature, doping);
                } else if(model == "constant") {
                    model_ = std::make_unique<ConstantMobility>(config.get<double>("mobility_electron"),
                                                                config.get<double>("mobility_hole"));
                } else if(model == "custom") {
                    model_ = std::make_unique<Custom>(config, doping);
                } else {
                    throw InvalidModelError(model);
                }
                LOG(INFO) << "Selected mobility model \"" << model << "\"";
            } catch(const ModelError& e) {
                throw InvalidValueError(config, "mobility_model", e.what());
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
