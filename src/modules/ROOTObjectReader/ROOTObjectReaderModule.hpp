/**
 * @file
 * @brief Definition of ROOT data file reader module
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include <functional>
#include <map>
#include <string>

#include <TFile.h>
#include <TTree.h>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Event.hpp"
#include "core/module/Module.hpp"

// Contains tuple of all defined objects
#include "objects/objects.h"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to read data stored in ROOT file back to allpix messages
     *
     * Reads the tree of objects in the data format of the \ref ROOTObjectWriterModule. Converts all the stored objects that
     * are supported back to messages containing those objects and dispatches those messages.
     */
    class ROOTObjectReaderModule : public Module {
    public:
        using MessageCreatorMap =
            std::map<std::type_index,
                     std::function<std::shared_ptr<BaseMessage>(std::vector<Object*>, std::shared_ptr<Detector>)>>;

        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param geo_mgr Pointer to the geometry manager, containing the detectors
         */
        ROOTObjectReaderModule(Configuration& config, Messenger* messenger, GeometryManager* geo_mgr);
        /**
         * @brief Destructor deletes the internal objects read from ROOT Tree
         */
        ~ROOTObjectReaderModule() override;

        /**
         * @brief Open the ROOT file containing the stored output data
         */
        void initialize() override;

        /**
         * @brief Convert the objects stored for the current event to messages
         */
        void run(Event*) override;

        /**
         * @brief Output summary and close the ROOT file
         */
        void finalize() override;

    private:
        Messenger* messenger_;
        GeometryManager* geo_mgr_;

        /**
         * @brief Internal object storing objects and information to construct a message from tree
         */
        struct message_info {
            std::vector<Object*>* objects;
            std::shared_ptr<Detector> detector;
            std::string name;
            std::shared_ptr<BaseMessage> message;
        };

        // Object names to include or exclude from reading
        std::set<std::string> include_;
        std::set<std::string> exclude_;

        // File containing the objects
        std::unique_ptr<TFile> input_file_;

        // Object trees in the file
        std::vector<TTree*> trees_;

        // List of objects and message information converted from the trees
        std::list<message_info> message_info_array_;

        // Statistics for total amount of objects stored
        std::atomic<unsigned long> read_cnt_{};

        // Internal map to construct an object from it's type index
        MessageCreatorMap message_creator_map_;
    };
} // namespace allpix
