/**
 * @file
 * @brief Definition of pulse object
 * @copyright Copyright (c) 2018-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
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
    class Pulse {
    public:
        /**
         * @brief Construct a new pulse
         */
        explicit Pulse(double time_bin);

        /**
         * @brief Construct default pulse, uninitialized
         */
        Pulse() = default;

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
         * @brief Function to retrieve the full pulse shape
         * @return Constant reference to the pulse vector
         */
        const std::vector<double>& getPulse() const;

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
        ClassDef(Pulse, 2);

    private:
        std::vector<double> pulse_;
        double bin_;
        bool initialized_{};
    };

} // namespace allpix

#endif /* ALLPIX_PULSE_H */
