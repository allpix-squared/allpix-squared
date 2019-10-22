/**
 * @file
 * @brief Building of the geometry
 *
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include <Math/Rotation3D.h>
#include <Math/Translation3D.h>

#include "GeometryBuilder.hpp"
#include "core/module/exceptions.h"

using namespace allpix;

template <typename WorldVolume, typename Materials>
void GeometryBuilder<WorldVolume, Materials>::build(WorldVolume*, std::map<std::string, Materials*>) {}
