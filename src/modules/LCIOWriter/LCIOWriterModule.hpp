/**
 * @file
 * @brief Definition of [LCIOWriter] module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
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
    class LCIOWriterModule : public WriterModule {
    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param geo_manager Pointer to the geometry manager, containing the detectors
         */
        LCIOWriterModule(Configuration& config, Messenger* messenger, GeometryManager* geo_manager);

        ~LCIOWriterModule(){};

        /**
         * @brief Initialize LCIO and GEAR output files
         */
        void init(std::mt19937_64&) override;

        /**
         * @brief Receive pixel hit messages, create lcio event, add hit collection and write event to file.
         */
        void run(Event*) const override;

        /**
         * @brief Close the output file
         */
        void finalize() override;

        /**
         * @brief Checks if the module is ready to run in the given event
         * @param Event pointer to the event the module will run
         */
        virtual bool isSatisfied(Event* event) const override;

    private:
        GeometryManager* geo_mgr_{};
        mutable std::shared_ptr<IO::LCWriter> lcWriter_{};

        mutable std::vector<std::string> collection_names_vector_;
        mutable std::map<unsigned, size_t> detector_ids_to_colllection_index_;
        mutable std::map<std::string, unsigned> detector_names_to_id_;
        mutable std::map<std::string, std::vector<std::string>> collections_to_detectors_map_;

        int pixel_type_;

        bool dump_mc_truth_;
        std::string detector_name_;
        std::string lcio_file_name_;
        std::string geometry_file_name_;
        mutable int write_cnt_{0};
    };
} // namespace allpix
