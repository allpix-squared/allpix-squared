/**
 * @file
 * @brief Definition of Geant4 GDML file construction module
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_MODULE_GDML_OUTPUT_WRITER_H
#define ALLPIX_MODULE_GDML_OUTPUT_WRITER_H

#include <map>
#include <memory>
#include <string>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to construct a GDML Output of the geometry

     */
    class GDMLOutputWriterModule : public Module {
    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param geo_manager Pointer to the geometry manager, containing the detectors
         */
        GDMLOutputWriterModule(Configuration& config, Messenger* messenger, GeometryManager* geo_manager);
        ~GDMLOutputWriterModule() override = default;

        /**
         * @brief Initializes Geant4 and construct the GDML output file from the internal geometry
         */
        void initialize() override;

    private:
        std::string output_file_name_{};
    };
} // namespace allpix

#endif /* ALLPIX_MODULE_GDML_OUTPUT_WRITER_H */
