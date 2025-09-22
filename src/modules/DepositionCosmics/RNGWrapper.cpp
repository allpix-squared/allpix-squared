/**
 * @file
 * @brief Implementation of the CRY RNG wrapper class
 *
 * @copyright Copyright (c) 2021-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "RNGWrapper.hpp"

using namespace allpix;

template CLHEP::HepRandomEngine* RNGWrapper<CLHEP::HepRandomEngine>::m_obj;
template double (CLHEP::HepRandomEngine::* RNGWrapper<CLHEP::HepRandomEngine>::m_func)();
