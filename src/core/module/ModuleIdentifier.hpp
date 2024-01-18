/**
 * @file
 * @brief Provide an identifier for module instances
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_MODULE_IDENTIFIER_H
#define ALLPIX_MODULE_IDENTIFIER_H

#include <memory>
#include <string>
#include <vector>

namespace allpix {
    /**
     * @brief Internal identifier for a module
     *
     * Used by the framework to distinguish between different module instantiations and their priority.
     */
    class ModuleIdentifier {
    public:
        /**
         * @brief Constructs an empty identifier
         */
        // TODO [doc] Is this method really necessary
        ModuleIdentifier() = default;
        /**
         * @brief Construct an identifier
         * @param module_name Name of the module
         * @param identifier Unique identifier for the instantiation
         * @param prio Priority of this module
         */
        ModuleIdentifier(std::string module_name, std::string identifier, int prio)
            : name_(std::move(module_name)), identifier_(std::move(identifier)), prio_(prio) {}

        /**
         * @brief Get the name of the module
         * @return Module name
         */
        const std::string& getName() const { return name_; }
        /**
         * @brief Get the identifier of the instantiation
         * @return Module identifier
         */
        const std::string& getIdentifier() const { return identifier_; }
        /**
         * @brief Get the unique name of the instantiation
         * @return Unique module name
         *
         * The unique name of the module is the name combined with its identifier separated by a semicolon.
         */
        std::string getUniqueName() const {
            std::string unique_name = name_;
            if(!identifier_.empty()) {
                unique_name += ":" + identifier_;
            }
            return unique_name;
        }
        /**
         * @brief Get the priority of the instantiation
         * @return Priority level
         *
         * A lower number indicates a higher priority
         *
         * @warning It is important to realize that the priority is ordered from high to low numbers
         */
        int getPriority() const { return prio_; }

        /// @{
        /**
         * @brief Operators for comparing identifiers
         *
         * Identifiers are compared on their unique name and their priorities
         */
        bool operator==(const ModuleIdentifier& other) const {
            return getUniqueName() == other.getUniqueName() && getPriority() == other.getPriority();
        }
        bool operator!=(const ModuleIdentifier& other) const {
            return getUniqueName() != other.getUniqueName() || getPriority() != other.getPriority();
        }
        bool operator<(const ModuleIdentifier& other) const {
            return getUniqueName() != other.getUniqueName() ? getUniqueName() < other.getUniqueName()
                                                            : getPriority() < other.getPriority();
        }
        bool operator<=(const ModuleIdentifier& other) const {
            return getUniqueName() != other.getUniqueName() ? getUniqueName() <= other.getUniqueName()
                                                            : getPriority() <= other.getPriority();
        }
        bool operator>(const ModuleIdentifier& other) const {
            return getUniqueName() != other.getUniqueName() ? getUniqueName() > other.getUniqueName()
                                                            : getPriority() > other.getPriority();
        }
        bool operator>=(const ModuleIdentifier& other) const {
            return getUniqueName() != other.getUniqueName() ? getUniqueName() >= other.getUniqueName()
                                                            : getPriority() >= other.getPriority();
        }
        /// @}

    private:
        std::string name_;
        std::string identifier_;
        int prio_{};
    };
} // namespace allpix

#endif /*ALLPIX_MODULE_IDENTIFIER_H */
