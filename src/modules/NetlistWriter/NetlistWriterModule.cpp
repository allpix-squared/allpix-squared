/**
 * @file
 * @brief Implementation of NetlistWriter module
 *
 * @copyright Copyright (c) 2024-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "NetlistWriterModule.hpp"

#include <filesystem>
#include <regex>
#include <string>
#include <utility>

#include "core/utils/log.h"

#include <TFormula.h>

using namespace allpix;

NetlistWriterModule::NetlistWriterModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector)
    : Module(config, detector), detector_(std::move(detector)), messenger_(messenger) {

    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();

    // Require PixelCharge message for single detector
    messenger_->bindSingle<PixelChargeMessage>(this, MsgFlags::REQUIRED);

    target_ = config.get<Target>("target");

    // Get the template netlist to modify:
    netlist_path_ = config.getPath("netlist_template", true);

    // Get the generated netlist name
    config_.setDefault<std::string>("file_name", "output_netlist_event_");
    file_name_ = config.get<std::string>("file_name");

    // Default source type set to ISOURCE_PULSE
    config_.setDefault<SourceType>("source_type", SourceType::ISOURCE_PULSE);
    source_type_ = config.get<SourceType>("source_type");

    // Get the source name and the circuit connected to it, as defined in the netlist
    source_name_ = config.get<std::string>("source_name");
    subckt_instance_name_ = config.get<std::string>("subckt_name");

    // Get the names of the common nets of the circuit the source is connected to (for ex. Vdd, etc.)
    auto com_nets = config_.getArray<std::string>("common_nets");
    common_nets_.insert(com_nets.begin(), com_nets.end());

    // Get the names of the waveforms (nets or nets) to save
    auto to_save = config_.getArray<std::string>("waveform_to_save");
    waveform_to_save_.insert(to_save.begin(), to_save.end());

    // Options to add the the uelec simulation command
    if(config_.has("simulator_command")) {
        run_netlist_simulation_ = true;
        simulator_command_ = config_.get<std::string>("simulator_command");
    }

    // Parameters for the ISOURCE_PULSE option (pulse shape)
    config_.setDefault<double>("t_delay", Units::get(0, "ns"));
    delay_ = config_.get<double>("t_delay");

    config_.setDefault<double>("t_rise", Units::get(1, "ns"));
    rise_ = config_.get<double>("t_rise");

    config_.setDefault<double>("t_fall", Units::get(1, "ns"));
    fall_ = config_.get<double>("t_fall");

    config_.setDefault<double>("t_width", Units::get(3, "ns"));
    width_ = config_.get<double>("t_width");
}

void NetlistWriterModule::initialize() {

    // Reads the template netlist specified
    std::ifstream netlist_file(netlist_path_);
    // Store the extension from the template netlist file
    extension_ = netlist_path_.extension().string();

    std::string line;
    size_t line_number = 0;
    while(getline(netlist_file, line)) {
        line_number++;
        // Writes the content of the netlist file to the new one
        file_lines_.push_back(line);

        // Identifies the ISOURCE declaration line
        if(line.rfind(source_name_, 0) == 0) {
            source_line_number_ = line_number;
            // regex for the ISOURCE line
            std::regex source_regex("\\((.+)\\)");
            std::smatch connection_match;
            if(std::regex_search(line, connection_match, source_regex)) {
                LOG(INFO) << "Found connections in netlist template: " << connection_match[0];
                // connections_[1] instead of connections_[0], to get back the nets without the ()
                connections_ = connection_match[1];

                // Identifies the ISOURCE instance nets names, the three following lines allow to have the two net names in
                // separate variables
                size_t space_pos = connections_.find(' ');
                source_net1_ = connections_.substr(0, space_pos);
                source_net2_ = connections_.substr(space_pos + 1);
            } else {
                throw ModuleError("Could not find net connections");
            }
            LOG(DEBUG) << "Found the source line!";
        }

        if(line.rfind(subckt_instance_name_, 0) == 0) {
            // For the subckt nets
            subckt_line_number_ = line_number;
            // regex for the circuit line
            std::regex subckt_regex("^(\\w+)\\s+\\((.+)\\)\\s+(\\w+)");
            std::smatch connection_match;
            if(std::regex_search(line, connection_match, subckt_regex)) {
                // connections_[1] instead of connections_[0], to get back the nets without the ()
                connections_ = connection_match[1];
                LOG(INFO) << "Found subckt connections in netlist template: " << connections_;
                std::istringstream iss(connection_match[2]);
                std::string net;
                // Reads each word separated by a space and adds it to net_list
                while(iss >> net) {
                    net_list_.push_back(net);
                }
                subckt_name_ = connection_match[3];
                LOG(DEBUG) << "Subckt name: " << subckt_name_ << ", nets: ";
                for(const auto& n : net_list_) {
                    LOG(DEBUG) << "\t" << n;
                }
            } else {
                throw ModuleError("Could not find net connections of the subckt");
            }
            LOG(DEBUG) << "Found the subckt line!";
        }
    }
}

void NetlistWriterModule::run(Event* event) {

    // Messages: Fetch the (previously registered) messages for this event from the messenger:
    auto message = messenger_->fetchMessage<PixelChargeMessage>(this, event);

    if(message->getData().empty()) {
        LOG(DEBUG) << "Empty event, skipping";
        return;
    }

    // Prepare output file for this event:
    const auto file_name = createOutputFile(file_name_ + "_event" + std::to_string(event->number), extension_);
    auto file = std::ofstream(file_name);
    LOG(INFO) << "Created output file at " << file_name;

    // Write the header on the new netlist
    size_t current_line = 0;
    for(; current_line < std::min(source_line_number_, subckt_line_number_) - 1; current_line++) {
        file << file_lines_[current_line] << '\n';
    }

    // waveform to be saved
    std::ostringstream to_be_saved;

    // Loop over all pixels
    for(const auto& pixel_charge : message->getData()) {
        const auto index = pixel_charge.getIndex();
        auto inputcharge = static_cast<double>(pixel_charge.getCharge());

        if(std::fabs(inputcharge) > std::numeric_limits<double>::epsilon()) {
            LOG(DEBUG) << "Received pixel " << index << ", charge " << Units::display(inputcharge, "e");

            // Get pixel address
            auto detector_model = detector_->getModel();
            auto idx = std::to_string(index.x() * static_cast<int>(detector_model->getNPixels().Y()) + index.y());

            if(target_ == Target::SPECTRE) {
                file << source_name_ << "\\<" << idx << "\\> (";
                // The source "gnd" (written "0") doesn't need to be incremented, this double "if" checks which net of the
                // source the gnd is connected to Also a condition if none of the 2 nets are gnd
                if(source_net1_ == "0") {
                    file << source_net1_ << " " << source_net2_ << "\\<" << idx << "\\>";
                } else if(source_net2_ == "0") {
                    file << source_net1_ << "\\<" << idx << "\\> " << source_net2_;
                } else {
                    file << source_net1_ << "\\<" << idx << "\\> " << source_net2_ << "\\<" << idx << "\\>";
                }
            } else if(target_ == Target::SPICE) {
                file << "I_" << idx << " ";

                //   The source "gnd" (written "0") doesn't need to be incremented, this double "if" checks which net of the
                //   source the gnd is connected to Also a condition if none of the 2 nets are gnd
                if(source_net1_ == "0") {
                    file << source_net1_ << " " << source_net2_ << "_" << idx << " ";
                } else if(source_net2_ == "0") {
                    file << source_net1_ << "_" << idx << " " << source_net2_;
                } else {
                    file << source_net1_ << "_" << idx << " " << source_net2_ << "_" << idx << " ";
                }
            }

            // ------- ISOURCE-------

            if(source_type_ == SourceType::ISOURCE) {

                // Get pulse and timepoints
                const auto& pulse = pixel_charge.getPulse(); // the pulse containing charges and times
                if(!pulse.isInitialized()) {
                    throw ModuleError("No pulse information available.");
                }
                const auto step = pulse.getBinning();

                (target_ == Target::SPECTRE) ? file << ") isource delay=" << delay_ << "n type=pwl wave=[" : file << "PWL(";

                for(auto bin = pulse.begin(); bin != pulse.end(); ++bin) {
                    auto time = step * static_cast<double>(std::distance(pulse.begin(), bin)) * 1e-9;
                    double current_bin = *bin / step;
                    auto current = Units::convert(current_bin, "nC");

                    file << std::setprecision(15) << time << " " << current << (bin < pulse.end() - 1 ? " " : "");
                }

                (target_ == Target::SPECTRE) ? file << "]\n" : file << ")\n";

                // ------- ISOURCE_PULSE -------

            } else if(source_type_ == SourceType::ISOURCE_PULSE) {

                const auto i_diode = Units::convert(inputcharge, "nC") / (rise_ / 2 + width_ + fall_ / 2);

                (target_ == Target::SPECTRE)
                    ? (file << ") isource type=pulse val0=0 val1=" << i_diode << " delay=" << delay_ << "n rise=" << rise_
                            << "n fall=" << fall_ << "n width=" << width_ << "n\n")
                    : (file << "PULSE(0 " << i_diode << " " << delay_ << "n " << rise_ << "n " << fall_ << "n " << width_
                            << "n)\n");
            }

            // Writing the subckt instance declaration
            (target_ == Target::SPECTRE) ? (file << subckt_instance_name_ << "\\<" << idx << "\\> (")
                                         : (file << subckt_instance_name_ << "_" << idx << " ");

            // Select whether the net needs to be iterated or not
            for(const auto& net : net_list_) {
                if(common_nets_.find(net) != common_nets_.end()) {
                    // must NOT be iterated !
                    file << net << " ";
                } else {
                    // must be iterated !
                    (target_ == Target::SPECTRE) ? file << net << "\\<" << idx << "\\>"
                                                        << " "
                                                 : file << net << "_" << idx << " ";
                }
            }
            file.seekp(-1, std::ios_base::cur);

            (target_ == Target::SPECTRE) ? file << ") " << subckt_name_ << "\n" : file << " " << subckt_name_ << "\n";

            // Add the increment (address of fired pixel) on each waveform to save, and concatenate a single string
            // (added later to the generated netlist)
            for(const auto& wave : waveform_to_save_) {
                (target_ == Target::SPECTRE) ? to_be_saved << wave << "\\<" << idx << "\\> "
                                             : to_be_saved << wave << "_" << idx << " ";
            }
        }
    }

    for(current_line++; current_line < std::max(source_line_number_, subckt_line_number_) - 1; current_line++) {
        file << file_lines_[current_line] << '\n';
    }

    for(current_line++; current_line < file_lines_.size(); current_line++) {
        file << file_lines_[current_line] << '\n';
    }

    //'save' line
    (target_ == Target::SPECTRE) ? file << "save " << to_be_saved.str() << '\n'
                                 : file << ".save " << to_be_saved.str() << '\n';

    file.close();
    LOG(DEBUG) << "Successfully written netlist to file " << file_name;

    // Runs the external uelec simulation, if selected in the configuration file, on the same terminal (ie. with uelec soft.
    // env variables loaded)
    if(run_netlist_simulation_ == true) {
        std::string uelec_sim_command = simulator_command_;
        uelec_sim_command += " ";
        uelec_sim_command += file_name;

        LOG(INFO) << uelec_sim_command;

        std::system(uelec_sim_command.c_str()); // NOLINT
    }
}
