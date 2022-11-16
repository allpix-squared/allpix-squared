/**
 * @file
 * @brief Definition of object for charges in sensor
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "SensorCharge.hpp"

using namespace allpix;

SensorCharge::SensorCharge(ROOT::Math::XYZPoint local_position,
                           ROOT::Math::XYZPoint global_position,
                           CarrierType type,
                           unsigned int charge,
                           double local_time,
                           double global_time)
    : local_position_(std::move(local_position)), global_position_(std::move(global_position)), local_time_(local_time),
      global_time_(global_time), type_(type), charge_(charge) {}

long SensorCharge::getSign() const {
    return static_cast<std::underlying_type<CarrierType>::type>(type_);
}

void SensorCharge::print(std::ostream& out) const {
    out << "Type: " << (type_ == CarrierType::ELECTRON ? "\"e\"" : "\"h\"") << "\nCharge: " << charge_ << " e"
        << "\nLocal Position: (" << local_position_.X() << ", " << local_position_.Y() << ", " << local_position_.Z()
        << ") mm\n"
        << "Global Position: (" << global_position_.X() << ", " << global_position_.Y() << ", " << global_position_.Z()
        << ") mm\n"
        << "Local time:" << local_time_ << " ns\n"
        << "Global time:" << global_time_ << " ns\n";
}
