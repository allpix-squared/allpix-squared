/**
 * @file
 * @brief Definition of pulse object
 *
 * @copyright Copyright (c) 2019-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_PULSE_H
#define ALLPIX_PULSE_H

#include <vector>

#include <TObject.h>

namespace allpix {
    /**
     * @ingroup Objects
     * @brief Pulse holding induced charges as a function of time
     * @warning This object is special and is not meant to be written directly to a tree (not inheriting from \ref Object)
     */
    class Pulse : public std::vector<double> {
    public:
        /**
         * @brief Construct a new pulse
         * @param time_bin Length in time of a single bin of the pulse
         */
        explicit Pulse(double time_bin) noexcept;

        /**
         * @brief
         * @param time_bin Length in time of a single bin of the pulse
         * @param total_time Expected total length of the pulse used to pre-allocate memory
         * @throws PulseBadAllocException if memory allocation failed
         */
        Pulse(double time_bin, double total_time);

        /**
         * @brief Construct default pulse, uninitialized
         */
        Pulse() = default;
        virtual ~Pulse() = default;
        Pulse(const Pulse&) = default;
        Pulse& operator=(const Pulse&) = default;
        Pulse(Pulse&&) = default;
        Pulse& operator=(Pulse&&) = default;

        /**
         * @brief adding induced charge to the pulse
         * @param charge induced charge
         * @param time   time when it has been induced
         */
        void addCharge(double charge, double time);

        /**
         * @brief Function to retrieve the integral (net) charge from the full pulse
         * @return Integrated charge
         */
        int getCharge() const;

        /**
         * @brief Function to retrieve time binning of pulse
         * @return Width of one pulse bin in nanoseconds
         */
        double getBinning() const;

        /**
         * @brief Method to check if this is an initialized or empty pulse
         * @return Initialization status of the pulse object
         */
        bool isInitialized() const;

        /**
         * @brief compound assignment operator to sum different pulses
         * @throws IncompatibleDatatypesException If the binning of the pulses does not match
         */
        Pulse& operator+=(const Pulse& rhs);

        /**
         * @brief Default constructor for ROOT I/O
         */
        ClassDef(Pulse, 3); // NOLINT

    private:
        double bin_{};
        bool initialized_{};
    };

} // namespace allpix

#endif /* ALLPIX_PULSE_H */
