/**
 * @file
 * @brief Implementation of SDAndFieldConstruction
 *
 * @copyright Copyright (c) 2019-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "SDAndFieldConstruction.hpp"
#include "DepositionGeant4Module.hpp"

using namespace allpix;

void SDAndFieldConstruction::ConstructSDandField() { module_->construct_sensitive_detectors_and_fields(); }
