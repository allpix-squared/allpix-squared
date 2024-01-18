/**
 * @file
 * @brief Definition of charge carrier detrapping models
 *
 * @copyright Copyright (c) 2022-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_DETRAPPING_MODELS_H
#define ALLPIX_DETRAPPING_MODELS_H

#include <TFormula.h>

#include "exceptions.h"

#include "core/config/Configuration.hpp"
#include "core/utils/log.h"
#include "core/utils/unit.h"
#include "objects/SensorCharge.hpp"

namespace allpix {

    /**
     * @ingroup Models
     * @brief Charge carrier detrapping time models
     */
    class DetrappingModel {
    public:
        /**
         * Default constructor
         */
        DetrappingModel() = default;

        /**
         * Default virtual destructor
         */
        virtual ~DetrappingModel() = default;

        /**
         * Function call operator to obtain detrapping time for the given carrier
         * @param type Type of charge carrier (electron or hole)
         * @param probability Current detrapping probability for this charge carrier
         * @param efield_mag Magnitude of the electric field
         * @return Expected time of the charge carrier being detrapped
         */
        virtual double operator()(const CarrierType& type, double probability, double efield_mag) const = 0;
    };

    /**
     * @ingroup Models
     * @brief No detrapping
     */
    class NoDetrapping : virtual public DetrappingModel {
    public:
        double operator()(const CarrierType&, double, double) const override { return std::numeric_limits<double>::max(); };
    };

    /**
     * @ingroup Models
     * @brief Constant detrapping rate of charge carriers
     */
    class ConstantDetrapping : virtual public DetrappingModel {
    public:
        ConstantDetrapping(double electron_lifetime, double hole_lifetime)
            : tau_eff_electron_(electron_lifetime), tau_eff_hole_(hole_lifetime){};

        double operator()(const CarrierType& type, double probability, double) const override {
            return -1 * log(1 - probability) * (type == CarrierType::ELECTRON ? tau_eff_electron_ : tau_eff_hole_);
        }

    protected:
        double tau_eff_electron_{std::numeric_limits<double>::max()};
        double tau_eff_hole_{std::numeric_limits<double>::max()};
    };

    /**
     * @brief Wrapper class and factory for detrapping models.
     *
     * This class allows to store detrapping  objects independently of the model chosen and simplifies access to the
     * function call operator. The constructor acts as factory, generating model objects from the model name provided, e.g.
     * from a configuration file.
     */
    class Detrapping {
    public:
        /**
         * Default constructor
         */
        Detrapping() = default;

        /**
         * Detrapping model constructor
         * @param config Configuration of the calling module
         */
        explicit Detrapping(const Configuration& config) {
            try {
                auto model = config.get<std::string>("detrapping_model", "none");

                if(model == "constant") {
                    model_ = std::make_unique<ConstantDetrapping>(config.get<double>("detrapping_time_electron"),
                                                                  config.get<double>("detrapping_time_hole"));
                } else if(model == "none") {
                    LOG(INFO) << "No charge carrier detrapping model chosen, no detrapping simulated";
                    model_ = std::make_unique<NoDetrapping>();
                } else {
                    throw InvalidModelError(model);
                }
            } catch(const ModelError& e) {
                throw InvalidValueError(config, "detrapping_model", e.what());
            }
        }

        /**
         * Function call operator forwarded to the detrapping model
         * @return Detrapping time
         */
        template <class... ARGS> double operator()(ARGS&&... args) const {
            return model_->operator()(std::forward<ARGS>(args)...);
        }

    private:
        std::unique_ptr<DetrappingModel> model_{};
    };

} // namespace allpix

#endif
