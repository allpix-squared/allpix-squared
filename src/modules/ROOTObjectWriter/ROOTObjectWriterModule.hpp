/**
 * @file
 * @brief Definition of ROOT data file writer module
 *
 * @copyright Copyright (c) 2017-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include <atomic>
#include <map>
#include <string>

#include <TFile.h>
#include <TTree.h>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Event.hpp"
#include "core/module/Module.hpp"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to write object data to ROOT trees in file for persistent storage
     *
     * Listens to all objects dispatched in the framework. Creates a tree as soon as a new type of object is encountered and
     * saves the data in those objects to tree for every event. The tree name is the class name of the object. A separate
     * branch is created for every combination of detector name and message name that outputs this object.
     */
    class ROOTObjectWriterModule : public SequentialModule {
    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param geo_mgr Pointer to the geometry manager, containing the detectors
         */
        ROOTObjectWriterModule(Configuration& config, Messenger* messenger, GeometryManager* geo_mgr);
        /**
         * @brief Destructor deletes the internal objects used to build the ROOT Tree
         */
        ~ROOTObjectWriterModule() override;

        /**
         * @brief Receive a single message containing objects of arbitrary type
         * @param message Message dispatched in the framework
         * @param name Name of the message
         */
        bool filter(const std::shared_ptr<BaseMessage>& message, const std::string& name) const;

        /**
         * @brief Opens the file to write the objects to
         */
        void initialize() override;

        /**
         * @brief Writes the objects fetched to their specific tree, constructing trees on the fly for new objects.
         */
        void run(Event* event) override;

        /**
         * @brief Add the main configuration and the detector setup to the data file and write it, also write statistics
         * information.
         */
        void finalize() override;

    private:
        Messenger* messenger_;
        GeometryManager* geo_mgr_;

        // Object names to include or exclude from writing
        std::set<std::string> include_;
        std::set<std::string> exclude_;

        // Output data file to write
        std::unique_ptr<TFile> output_file_;
        std::string output_file_name_{};

        // Current event
        uint64_t current_event_{0};

        // Current random seed
        uint64_t current_seed_{0};

        // List of trees that are stored in data file
        std::map<std::string, std::unique_ptr<TTree>> trees_;

        // List of objects of a particular type, bound to a specific detector and having a particular name
        std::map<std::tuple<std::type_index, std::string, std::string>, std::vector<Object*>*> write_list_;

        // Statistical information about number of objects
        std::atomic<unsigned long> write_cnt_{};
    };
} // namespace allpix
