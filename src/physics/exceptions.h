/**
 * @file
 * @brief Collection of model exceptions
 *
 * @copyright Copyright (c) 2021-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_MODEL_EXCEPTIONS_H
#define ALLPIX_MODEL_EXCEPTIONS_H

#include <string>

#include "core/utils/exceptions.h"

namespace allpix {
    /**
     * @ingroup Exceptions
     * @brief Base class for all model exceptions in the framework.
     */
    class ModelError : public RuntimeError {};

    /**
     * @ingroup Exceptions
     * @brief Notifies of an invalid model
     */
    class InvalidModelError : public ModelError {
    public:
        /**
         * @brief Construct an error for a model that is not found
         * @param model_name Name of the requested model
         */
        explicit InvalidModelError(const std::string& model_name) {
            error_message_ = "Model with name \"" + model_name + "\" does not exist";
        }
    };

    /**
     * @ingroup Exceptions
     * @brief Notifies of a model unsuitable for the current simulation
     */
    class ModelUnsuitable : public ModelError {
    public:
        /**
         * @brief Construct an error for a model that is not suitable for the current simulation
         * @param reason Message explaining why this model is unsuitable
         */
        explicit ModelUnsuitable(const std::string& reason = "") {
            error_message_ = "Model not suitable for this simulation: " + reason;
        }
    };
} // namespace allpix

#endif /* ALLPIX_MODEL_EXCEPTIONS_H */
