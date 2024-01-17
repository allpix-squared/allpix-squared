/**
 * @file
 * @brief Definition of [LCIOWriter] module
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include <string>
#include <vector>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Event.hpp"
#include "core/module/Module.hpp"

#include "objects/PixelHit.hpp"

#include <IO/LCWriter.h>

namespace allpix {

    /**
     * @ingroup Modules
     * @brief Module to write hit data to LCIO file
     *
     * Create LCIO file, compatible to EUTelescope analysis framework.
     */
    class LCIOWriterModule : public SequentialModule {
    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param geo_manager Pointer to the geometry manager, containing the detectors
         */
        LCIOWriterModule(Configuration& config, Messenger* messenger, GeometryManager* geo_manager);

        /**
         * @brief Initialize LCIO and GEAR output files
         */
        void initialize() override;

        /**
         * @brief Receive pixel hit messages, create lcio event, add hit collection and write event to file.
         */
        void run(Event* event) override;

        /**
         * @brief Close the output file
         */
        void finalize() override;

    private:
        Messenger* messenger_;
        GeometryManager* geo_mgr_{};
        std::shared_ptr<IO::LCWriter> lcWriter_{};

        std::vector<std::string> collection_names_vector_;
        std::map<unsigned, size_t> detector_ids_to_colllection_index_;
        std::map<std::string, unsigned> detector_names_to_id_;
        std::map<std::string, std::vector<std::string>> collections_to_detectors_map_;

        int pixel_type_;

        bool dump_mc_truth_;
        std::string detector_name_;
        std::string lcio_file_name_;
        std::string geometry_file_name_;
        std::atomic<int> write_cnt_{0};
    };
} // namespace allpix
