/**
 * @file
 * @brief Definition of NetlistWriter module
 *
 * @copyright Copyright (c) 2024-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 *
 * Contains minimal dummy module to use as a start for the development of your own module
 *
 * Please refer to the User Manual for more details on the different files of a module and the methods of the module class..
 */

#include <filesystem>
#include <fstream>
#include <regex>
#include <string>

#include "core/config/Configuration.hpp"
#include "core/geometry/DetectorModel.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Event.hpp"
#include "core/module/Module.hpp"

#include "objects/PixelCharge.hpp"

#include <TFormula.h>

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module which generates netlists to be fed to SPICE simulators
     */
    class NetlistWriterModule : public Module {
    public:
        enum class Target {
            SPICE,
            SPECTRE,
        };

        enum class SourceType {
            isource,
            isource_pulse,
            // vsource_pulse,
        };

        /**
         * @brief Constructor for the NetlistWriter module
         *
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param detector Pointer to the detector for this module instance
         */
        NetlistWriterModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector);

        /**
         * @brief Initialization method of the module
         */
        void initialize() override;

        /**
         * @brief Run method of the module
         */
        void run(Event* event) override;

        /**
         * @brief Finalization method of the module
         */
        void finalize() override;

    private:
        // Pointers to the central geometry manager and the messenger for interaction with the framework core:
        std::shared_ptr<Detector> detector_;
        Messenger* messenger_;

        // Module parameters
        std::filesystem::path netlist_path_;
        std::string extension;
        std::string file_name_{};
        Target target_;
        SourceType source_type_;

        std::string source_name_{};
        std::string subckt_instance_name_{};

        std::unique_ptr<TFormula> net_enumerator_{};
        std::string connections_;
        std::set<std::string> common_nets_;

        std::set<std::string> waveform_to_save_;
        // waveform to be saved
        std::ostringstream to_be_saved;

        bool run_netlist_simulation_{};
        std::string simulator_command_{};

        // isource_pulse
        double delay_{};
        double rise_{};
        double fall_{};
        double width_{};

        std::string source_net1_;
        std::string source_net2_;

        std::vector<std::string> net_list;

        std::string source_line;
        std::string subckt_name;
        size_t subckt_line_number = 0;
        size_t source_line_number = 0;
        double nanoCoulomb = 1.6e-10;
        double i_diode = 0;

        std::vector<std::string> file_lines;
    };
} // namespace allpix
