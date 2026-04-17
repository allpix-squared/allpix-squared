/**
 * @file
 * @brief Definition of charge carrier trapping models
 *
 * @copyright Copyright (c) 2021-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_TRAPPING_MODELS_H
#define ALLPIX_TRAPPING_MODELS_H

#include <TFormula.h>

#include "exceptions.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include "core/config/Configuration.hpp"
#include "core/utils/log.h"
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
         * @param probability Current trapping probability for this charge carrier
         * @param timestep Current time step performed for the charge carrier
         * additional possible parameter: efield_mag Magnitude of the electric field
         * @return Trapping status of the charge carrier
         */
        virtual bool operator()(const CarrierType& type, double probability, double timestep, double) const {
            return probability <
                   (1 - std::exp(-1. * timestep / (type == CarrierType::ELECTRON ? tau_eff_electron_ : tau_eff_hole_)));
        }

        // new interface for time + step length - d
        virtual bool operator()(
            const CarrierType& type, double probability, double timestep, double efield_mag, double /* dx_drift */) const {
            // default: ignore dx_drift and fall back to old behavior
            return (*this)(type, probability, timestep, efield_mag);
        }

    protected:
        double tau_eff_electron_{std::numeric_limits<double>::max()};
        double tau_eff_hole_{std::numeric_limits<double>::max()};
    };

    /**
     * @ingroup Models
     * @brief No trapping
     */
    class NoTrapping : virtual public TrappingModel {
    public:
        bool operator()(const CarrierType&, double, double, double) const override { return false; };
    };

    /**
     * @ingroup Models
     * @brief Constant trapping rate of charge carriers
     */
    class ConstantTrapping : virtual public TrappingModel {
    public:
        ConstantTrapping(double electron_lifetime, double hole_lifetime) {
            tau_eff_electron_ = electron_lifetime;
            tau_eff_hole_ = hole_lifetime;
        }
    };

    /**
     * @ingroup Models
     * @brief Ljubljana / Kramberger effective trapping model for charge carriers in silicon
     *
     * Parametrization taken from https://doi.org/10.1016/S0168-9002(01)01263-3, effective trapping time from Eq. 4 with beta
     * values from Table 2 (pions/protons), temperature dependency according to Eq. 9, scaling factors kappa from Table 3.
     * The reference temperature at which the measurements were conducted is 263K.
     */
    class Ljubljana : virtual public TrappingModel {
    public:
        Ljubljana(double temperature, double fluence) {
            tau_eff_electron_ = 1. / Units::get(5.6e-16 * std::pow(temperature / 263, -0.86), "cm*cm/ns") / fluence;
            tau_eff_hole_ = 1. / Units::get(7.7e-16 * std::pow(temperature / 263, -1.52), "cm*cm/ns") / fluence;
        }
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
        explicit Dortmund(double fluence) {
            tau_eff_electron_ = 1. / Units::get(5.13e-16, "cm*cm/ns") / fluence;
            tau_eff_hole_ = 1. / Units::get(5.04e-16, "cm*cm/ns") / fluence;
        }
    };

    /**
     * @ingroup Models
     * @brief Effective trapping model developed by the CMS Tracker Group
     *
     * Parametrization taken from https://doi.org/10.1088/1748-0221/11/04/P04023, effective trapping time from Table 2.
     * Interpolation between evaluated fluence points by M. Bomben
     *
     * FIXME no temperature dependence
     */
    class CMSTracker : virtual public TrappingModel {
    public:
        explicit CMSTracker(double fluence) {
            tau_eff_electron_ = 1. / (Units::get(1.71e-16, "cm*cm/ns") * fluence + Units::get(0.114, "/ns"));
            tau_eff_hole_ = 1. / (Units::get(2.79e-16, "cm*cm/ns") * fluence + Units::get(0.093, "/ns"));
        }
    };

    /**
     * @ingroup Models
     * @brief Ljubljana High-Fluence / Mandic effective trapping model
     *
     * Parametrization taken from https://doi.org/10.1088/1748-0221/15/11/P11018, section 5.
     * Updated the c_e based on https://doi.org/10.1088/1748-0221/16/03/E03001
     */
    class Mandic : virtual public TrappingModel {
    public:
        explicit Mandic(double fluence) {
            tau_eff_electron_ = 0.54 * pow(fluence / Units::get(1e16, "/cm/cm"), -0.62);
            tau_eff_hole_ = tau_eff_electron_;
        }
    };

    /**
     * @ingroup Models
     * @brief Polycrystalline diamond trapping model based on charge collection distance (CCD)
     *  https://doi.org/10.48550/arXiv.2603.13146
     * This model derives effective electron and hole trapping lengths from the
     * sensor thickness and the input charge collection distance (CCD), assuming
     * a fixed ratio (configurable) between hole and electron trapping lengths (mean drift path).
     *
     * The CCD is described using the Hecht relation, and the resulting trapping
     * lengths are used to calculate the carrier trapping probability during drift.
     *
     * Hole trapping lengths are related to electron trapping lengths via
     * lambda_h / lambda_e = r.
     */

    // ------------------------------------------------------------------
    // Polycrystalline diamond trapping model based on the charge
    // collection distance (CCD).
    //
    // The CCD is related to the collected charge by
    //
    //   CCD = (Q_collected / Q_generated) * t
    //
    // where t is the sensor thickness.
    //
    // For polycrystalline diamond, the CCD is described as the sum of the
    // electron and hole Hecht contributions:
    //
    //   CCD = f(lambda_e, t) + f(lambda_h, t)
    //
    // with
    //
    //   f(lambda, t) = lambda * [ 1 - (lambda / t) * (1 - exp(-t / lambda)) ]
    //
    // Here:
    //   lambda_e = electron trapping length
    //   lambda_h = hole trapping length
    //   t        = sensor thickness
    //
    // We assume
    //
    //   lambda_h = r * lambda_e
    //
    // where r is the hole-to-electron trapping-length ratio.
    //
    // Given CCD and sensor thickness, this model solves for lambda_e and
    // derives lambda_h from the chosen ratio.
    //
    // During transport, the trapping probability over a drift distance dx is
    //
    //   P_trap = 1 - exp(-dx / lambda)
    //
    // where lambda is the trapping length (Mean Drift Path) of the carrier type.
    // ------------------------------------------------------------------

    class PCDiamond : virtual public TrappingModel {
    public:
        // ccd and thickness must be in the SAME length units as dx_drift (Allpix internal length units)
        PCDiamond(double ccd, double thickness, double lambda_ratio_h_over_e = 1.3)
            : thickness_(thickness), ratio_(lambda_ratio_h_over_e) {

            if(thickness_ <= 0) {
                throw ModelError("pcdiamond trapping: sensor thickness must be > 0");
            }
            if(ccd <= 0) {
                // CCD<=0 means essentially fully trapped immediately
                lambda_electron_ = 0;
                lambda_hole_ = 0;
                return;
            }
            if(ccd >= thickness_) {
                // Full collection limit: lambdas -> infinity. for this condition, Use a very large number.
                lambda_electron_ = 1e12 * thickness_;
                lambda_hole_ = ratio_ * lambda_electron_;
                return;
            }
            if(ratio_ <= 0) {
                throw ModelError("pcdiamond trapping: lambda_ratio_h_over_e must be > 0");
            }

            solve_lambdas_from_ccd(ccd);
        }

        using TrappingModel::operator();

        bool operator()(const CarrierType& type,
                        double probability,
                        double /*timestep*/,
                        double /*efield_mag*/,
                        double dx_drift) const override {

            const double lambda = (type == CarrierType::ELECTRON ? lambda_electron_ : lambda_hole_);
            if(lambda <= 0) {
                return true; // immediate trapping
            }
            return probability < (1.0 - std::exp(-dx_drift / lambda));
        }

    private:
        double thickness_{};
        double ratio_{1.3};

        double lambda_electron_{};
        double lambda_hole_{};

        // Hecht contribution for one carrier (matches pcdiamond equation).
        // contribution = λ * [ 1 - (λ/t) * (1 - exp(-t/λ)) ]
        static double hecht_contrib(double lambda, double t) {
            if(lambda <= 0) {
                return 0.0;
            }
            // 1 - exp(-t/lambda) computation
            const double one_minus_exp = -std::expm1(-t / lambda);
            return lambda * (1.0 - (lambda / t) * one_minus_exp);
        }

        void solve_lambdas_from_ccd(double ccd) {
            auto f = [&](double lambda_e) {
                const double lambda_h = ratio_ * lambda_e;
                return hecht_contrib(lambda_e, thickness_) + hecht_contrib(lambda_h, thickness_) - ccd;
            };

            // Choosing initial bounds such that the solution lies between,
            // f(low) < 0 and f(high) > 0
            double low = 1e-15 * thickness_;
            double high = 1e6 * thickness_;

            // If even a very small lambda already overshoots the target CCD,
            // treat this as the extremely small-CCD limit.
            if(f(low) > 0) {

                lambda_electron_ = low;
                lambda_hole_ = ratio_ * lambda_electron_;
                return;
            }
            // Increase the upper bound until it lies above the solution.
            while(f(high) < 0) {
                high *= 2.0;
                if(high > 1e18 * thickness_) {
                    // No valid solution found within a reasonable range.
                    throw ModelError("pcdiamond trapping: could not solve lambdas from CCD (check units/values)");
                }
            }

            // Repeatedly halving the interval to locate the solution.
            for(int i = 0; i < 200; ++i) {
                const double mid = 0.5 * (low + high);
                if(f(mid) > 0) {
                    high = mid;
                } else {
                    low = mid;
                }
            }
            // Setting the final electron and hole trapping lengths (mean drift pahths)
            lambda_electron_ = 0.5 * (low + high);
            lambda_hole_ = ratio_ * lambda_electron_;
        }
    };

    /**
     * @ingroup Models
     * @brief Custom trapping model for charge carriers
     */
    class CustomTrapping : virtual public TrappingModel {
    public:
        explicit CustomTrapping(const Configuration& config) {
            tf_tau_eff_electron_ = configure_tau_eff(config, CarrierType::ELECTRON);
            tf_tau_eff_hole_ = configure_tau_eff(config, CarrierType::HOLE);
        };

        bool operator()(const CarrierType& type, double probability, double timestep, double efield_mag) const override {
            return probability < (1 - std::exp(-1. * timestep /
                                               (type == CarrierType::ELECTRON ? tf_tau_eff_electron_->Eval(efield_mag)
                                                                              : tf_tau_eff_hole_->Eval(efield_mag))));
        };

    private:
        std::unique_ptr<TFormula> tf_tau_eff_electron_;
        std::unique_ptr<TFormula> tf_tau_eff_hole_;

        std::unique_ptr<TFormula> configure_tau_eff(const Configuration& config, const CarrierType type) {
            std::string name = (type == CarrierType::ELECTRON ? "electrons" : "holes");
            auto function = config.get<std::string>("trapping_function_" + name);
            auto parameters = config.getArray<double>("trapping_parameters_" + name, {});

            auto trapping = std::make_unique<TFormula>(("trapping_" + name).c_str(), function.c_str());

            if(!trapping->IsValid()) {
                throw InvalidValueError(
                    config, "trapping_function_" + name, "The provided model is not a valid ROOT::TFormula expression");
            }

            // Check if number of parameters match up
            if(static_cast<size_t>(trapping->GetNpar()) != parameters.size()) {
                throw InvalidValueError(config,
                                        "trapping_parameters_" + name,
                                        "The number of provided parameters and parameters in the function do not match");
            }

            // Set the parameters
            for(size_t n = 0; n < parameters.size(); ++n) {
                trapping->SetParameter(static_cast<int>(n), parameters[n]);
            }

            return trapping;
        };
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
         * Trapping model constructor
         * @param config Configuration of the calling module
         */
        explicit Trapping(const Configuration& config) {
            try {
                auto model = config.get<std::string>("trapping_model", "none");
                std::transform(model.begin(), model.end(), model.begin(), [](unsigned char c) {
                    return static_cast<char>(std::tolower(c));
                });
                auto temperature = config.get<double>("temperature");
                auto fluence = config.get<double>("fluence", 0);

                // Warn for fluences >= 1e17 neq/cm^2 since this might be the result of a wrong unit and not intentional
                if(fluence >= 1e15) {
                    LOG(WARNING) << "High fluence of " << Units::display(fluence, "neq/cm/cm")
                                 << " detected, units might not be set correctly";
                }

                if(model == "ljubljana" || model == "kramberger") {
                    model_ = std::make_unique<Ljubljana>(temperature, fluence);
                } else if(model == "dortmund" || model == "krasel") {
                    model_ = std::make_unique<Dortmund>(fluence);
                } else if(model == "cmstracker") {
                    model_ = std::make_unique<CMSTracker>(fluence);
                } else if(model == "mandic" || model == "ljubljana_highfluence") {
                    model_ = std::make_unique<Mandic>(fluence);
                } else if(model == "pcdiamond") {
                    model_ = std::make_unique<PCDiamond>(
                        config.get<double>("pcdiamond_ccd"),
                        config.get<double>("pcdiamond_thickness"),
                        config.get<double>("pcdiamond_lambda_ratio",
                                           1.3) //  the assumed ratio of h-drift distance to e-drift distance
                    );
                } else if(model == "constant") {
                    model_ = std::make_unique<ConstantTrapping>(config.get<double>("trapping_time_electron"),
                                                                config.get<double>("trapping_time_hole"));
                } else if(model == "none") {
                    LOG(INFO) << "No charge carrier trapping model chosen, no trapping simulated";
                    model_ = std::make_unique<NoTrapping>();
                } else if(model == "custom") {
                    model_ = std::make_unique<CustomTrapping>(config);
                } else {
                    throw InvalidModelError(model);
                }
                LOG(INFO) << "Selected trapping model \"" << model << "\"";
            } catch(const ModelError& e) {
                throw InvalidValueError(config, "trapping_model", e.what());
            }
        }

        /**
         * Function call operator forwarded to the trapping model
         * @return Trapping state
         */
        template <class... ARGS> bool operator()(ARGS&&... args) const {
            return model_->operator()(std::forward<ARGS>(args)...);
        }

    private:
        std::unique_ptr<TrappingModel> model_;
    };

} // namespace allpix

#endif
