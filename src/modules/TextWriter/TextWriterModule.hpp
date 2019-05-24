/**
 * @file
 * @brief Definition of ASCII text file writer module
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <fstream>
#include <map>
#include <string>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Event.hpp"
#include "core/module/Module.hpp"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to write object data to simple ASCII text files
     *
     * Listens to all objects dispatched in the framework and stores an ASCII representation of every object to file.
     */
    class TextWriterModule : public WriterModule {
    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param geo_mgr Pointer to the geometry manager, containing the detectors
         */
        TextWriterModule(Configuration& config, Messenger* messenger, GeometryManager* geo_mgr);
        /**
         * @brief Destructor deletes the internal objects used to build the ROOT Tree
         */
        ~TextWriterModule() override;

        /**
         * @brief Receive a single message containing objects of arbitrary type
         * @param message Message dispatched in the framework
         * @param name Name of the message
         */
        bool filter(const std::shared_ptr<BaseMessage>& message, const std::string& name) const;

        /**
         * @brief Opens the file to write the objects to
         */
        void init(std::mt19937_64&) override;

        /**
         * @brief Writes the objects fetched to their specific tree, constructing trees on the fly for new objects.
         */
        void run(Event*) const override;

        /**
         * @brief Add the main configuration and the detector setup to the data file and write it, also write statistics
         * information.
         */
        void finalize() override;

        /**
         * @brief Checks if the module is ready to run in the given event
         * @param Event pointer to the event the module will run
         */
        virtual bool isSatisfied(Event* event) const override;

    private:
        // Object names to include or exclude from writing
        std::set<std::string> include_;
        std::set<std::string> exclude_;

        // Output data file to write
        std::string output_file_name_{};
        std::unique_ptr<std::ofstream> output_file_;

        // List of objects of a particular type, bound to a specific detector and having a particular name
        std::map<std::tuple<std::type_index, std::string, std::string>, std::vector<Object*>*> write_list_;

        // Statistical information about number of objects
        mutable unsigned long write_cnt_{};
        mutable unsigned long msg_cnt_{};
    };
} // namespace allpix
