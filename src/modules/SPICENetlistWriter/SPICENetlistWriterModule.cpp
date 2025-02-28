/**
 * @file
 * @brief Implementation of SPICENetlistWriter module
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include "SPICENetlistWriterModule.hpp"

#include <filesystem>
#include <string>
#include <utility>
#include <regex>

#include "core/utils/log.h"

#include <TFormula.h>

using namespace allpix;

SPICENetlistWriterModule::SPICENetlistWriterModule(Configuration& config,
                                                Messenger* messenger,
                                                std::shared_ptr<Detector> detector)
    : Module(config, detector), detector_(std::move(detector)), messenger_(messenger) {

    // Enable multithreading of this module if multithreading is enabled
    allow_multithreading();



    // Require PixelCharge message for single detector
    messenger_->bindSingle<PixelChargeMessage>(this, MsgFlags::REQUIRED);

    // Set the default target SPICE dialect
    config_.setDefault<Target>("target", Target::SPECTRE);
    target_ = config.get<Target>("target");
    
    // Get the template netlist to modify:
    netlist_path_ = config.getPath("netlist_template", true);

    // Get the generated netlist name
    config_.setDefault<std::string>("file_name", "output_netlist_event_");
    file_name_ = config.get<std::string>("file_name");

    // Default source type set to isource_pulse 
    config_.setDefault<SourceType>("source_type", SourceType::isource_pulse);
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


    auto detector_model = detector_->getModel();
    std::string default_enumerator = "x * " + std::to_string(detector_model->getNPixels().Y()) + " + y";
    net_enumerator_ = std::make_unique<TFormula>("net_enumerator", (config_.get<std::string>("net_enumerator", default_enumerator)).c_str());
    
    if(!net_enumerator_->IsValid()) {
        throw InvalidValueError(config_, "net_enumerator", "net enumerator is not a valid ROOT::TFormula expression.");
    }

    // Get the input electrode capacitance, used when selecting "vsource" 
    config_.setDefault<double>("electrode_capacitance", Units::get(5e-15, "C/V"));
    electrode_capacitance_ = config_.get<double>("electrode_capacitance");

    // Boolean to execut or not the external uelec simulation
    config_.setDefault<bool>("run_netlist_sim", false);
    run_netlist_simulation_ = config_.get<bool>("run_netlist_sim");

    // Options to add the the uelec simulation command
    simulator_options_ = config.get<std::string>("simulator_options");
    
    // Parameters for the isource_pulse option (pulse shape)
    config_.setDefault<double>("t_delay", Units::get(100, "ns"));
    config_.setDefault<double>("t_rise", Units::get(1, "ns"));
    config_.setDefault<double>("t_fall", Units::get(1, "ns"));
    config_.setDefault<double>("t_width", Units::get(3, "ns"));

    auto parameters = config_.getArray<double>("net_enumerator_parameters", {});

    // check if number of parameters match up
    if(static_cast<size_t>(net_enumerator_->GetNpar()) != parameters.size()) {
        throw InvalidValueError(
            config_,
            "net_enumerator_parameters",
            "The number of function parameters does not line up with the number of parameters in the function.");
    }

    for(size_t n = 0; n < parameters.size(); ++n) {
        net_enumerator_->SetParameter(static_cast<int>(n), parameters[n]);
    }

    LOG(DEBUG) << "Net enumerator function successfully initialized with " << parameters.size() << " parameters";
}

void SPICENetlistWriterModule::initialize() {

    // Reads the template netlist specified
    int line_number = 0;
    std::ifstream netlist_file (netlist_path_);
    std::string line;
    std::ostringstream eof_netlist;
    std::ostringstream header_netlist;
    // regex for the isource line
    std::regex source_regex("\\((.+)\\)");
    // regex for the circuit line
    std::regex subckt_regex("^(\\w+)\\s+\\((.+)\\)\\s+(\\w+)");
    std::smatch connection_match;
    

    while (getline(netlist_file, line)) {
        // Writes the content of the netlist file to the new one
        // Identifies the isource declaration line
        // Identifies the isource instance nets names
        line_number++;
        file_lines.push_back(line);

        // For the isource nets
        if(line.rfind(source_name_, 0) == 0){
            source_line_number = line_number;
            if(std::regex_search(line, connection_match, source_regex)) {
                LOG(INFO) << "connections: " << connection_match[0];
                // connections_[1] instead of connections_[0], to get back the nets without the ()
                connections_ = connection_match[1];

                // the three following lines allow to have the two net names in separate variables
                size_t space_pos = connections_.find(" ");
                source_net1_ = connections_.substr(0, space_pos);
                source_net2_ = connections_.substr(space_pos + 1);
            } else {
                throw ModuleError("Could not find net connections");
            }
            LOG(INFO) << "Found the source line!";
        }


        if(line.rfind(subckt_instance_name_, 0) == 0){
            // For the subckt nets
            subckt_line_number = line_number;
            if(std::regex_search(line, connection_match, subckt_regex)) {
                // connections_[1] instead of connections_[0], to get back the nets without the ()
                connections_ = connection_match[1];
                LOG(INFO) << "Subckt connections: " << connections_;
                std::istringstream iss(connection_match[2]);
                std::string net;
                LOG(INFO) << "Subckt nets: " << net;
                // Reads each word separated by a space and adds it to net_list
                while (iss >> net) { 
                    net_list.push_back(net);
                }
                subckt_name = connection_match[3];
                LOG(INFO) << "Subckt name: " << subckt_name;
            } else {
                throw ModuleError("Could not find net connections of the subckt");
            }
            LOG(INFO) << "Found the subckt line!";
        }
    }

    // find position of the last character to keep in the isource declaration
    size_t p = file_lines[source_line_number].find("[");

    // only keep the isource line before the waveform
    source_line = file_lines[source_line_number].substr(0, p);

    LOG(DEBUG) << "End of initialize";
}



void SPICENetlistWriterModule::run(Event* event) {
    LOG(DEBUG) << "Module entered the run loop";

    // Messages: Fetch the (previously registered) messages for this event from the messenger:
    auto message = messenger_->fetchMessage<PixelChargeMessage>(this, event);
    
    if (message->getData().empty()) {
            LOG(INFO) << "No pixels fired, skipping even";
            return;
        }

    // Prepare output file for this event:
    const auto file_name = createOutputFile(file_name_ + "_event_" + std::to_string(event->number) + ".scs");
    auto file = std::ofstream(file_name);
    LOG(INFO) << "Output file(s) created";

    // Write the header on the new netlist
    size_t current_line = 0;
    for (; current_line < std::min(source_line_number, subckt_line_number) - 1; current_line++){
        file << file_lines[current_line] << '\n';
    }
    
    // For loop over all pixels fired
    for(const auto& pixel_charge : message->getData()) {
        auto pixel = pixel_charge.getPixel();
        auto pixel_index = pixel.getIndex();
        auto inputcharge = static_cast<double>(pixel_charge.getCharge());
        
        if (inputcharge != 0) {

            // Charge value for vsource
            //auto charge = static_cast<double>(pixel_charge.getAbsoluteCharge());
            //std::ostringstream pulse_string;
            
            LOG(INFO) << "Received pixel " << pixel_index << ", charge " << Units::display(inputcharge, "e");

            ///////////////////////////////////////////////

            // Get pulse and timepoints
            //const auto& pulse = pixel_charge.getPulse(); // the pulse containing charges and times

            // Get pixel address
            const auto idx = net_enumerator_->Eval(pixel_index.x(), pixel_index.y());

            file << source_name_ << "\\<" << idx << "\\> (";

            // The source "gnd" (written "0") doesn't need to be incremented, this double "if" checks which net of the source the gnd is connected to
            // Also a condition if none of the 2 nets are gnd
            if (source_net1_ == "0") {
                file << source_net1_ << " " << source_net2_ << "\\<" << idx << "\\>";
            } else if (source_net2_ == "0") {
                file << source_net1_ << "\\<" << idx << "\\> " << source_net2_;
            } else if (source_net1_ != "0" && source_net2_ != "0") {
                file <<  source_net1_ << "\\<" << idx << "\\> " << source_net2_ << "\\<" << idx << "\\>";
            }
            
            // ------- isource -------

            if (source_type_ == SourceType::isource) {

                // Get pulse and timepoints
                const auto& pulse = pixel_charge.getPulse(); // the pulse containing charges and times
                if(!pulse.isInitialized()) {
                    throw ModuleError("No pulse information available.");
                }
                double step = pulse.getBinning();
            
                file << ") isource type=pwl wave=[";

                for (auto bin = pulse.begin(); bin != pulse.end(); ++bin){
                    auto time = step * static_cast<double>(std::distance(pulse.begin(), bin)) * 1e-9;
                    double current_bin = *bin / step;
                    double current = current_bin* nanoCoulomb;

                    file << std::setprecision(15) << time << " " << current << (bin < pulse.end() - 1 ? " " : "");
                }
                file << "]\n";

            // ------- isource_pulse -------

            } else if (source_type_ == SourceType::isource_pulse) {

                // ?!?!?!? not sure of having correctly configured the units
                delay_ = config_.get<double>("t_delay");
                //delay_ = Units::convert(config_.get<double>("t_delay"), "ns");
                rise_  = config_.get<double>("t_rise");
                //rise_ = Units::convert(config_.get<double>("t_rise"), "ns");
                fall_  = config_.get<double>("t_fall");
                //fall_ = Units::convert(config_.get<double>("t_fall"), "ns");
                width_  = config_.get<double>("t_width");
                //width_ = Units::convert(config_.get<double>("t_width"), "ns");

                i_diode = (inputcharge*nanoCoulomb)/(rise_/2+width_+fall_/2);
                
                file << ") isource type=pulse val0=0 val1=" << i_diode << " delay=" << delay_ << "n rise=" << rise_ << "n fall=" << fall_ << "n width=" << width_ << "n\n";


            // ------- vsource -------
            } else if (source_type_ == SourceType::vsource) {
                // Capacitance
                // Calculates the voltage on the net using the charge collected by the diode and the pixel capacitance
                v_diode = (inputcharge*elementalCharge)/electrode_capacitance_;
                // Charge
                file << ") vsource type=dc dc=" << std::setprecision(9) << v_diode << "\n";
            }
            

            // Writing the subckt instance declaration
            file << subckt_instance_name_ << "\\<" << idx << "\\> (";

            // Select whether the net needs to be iterated or not
            for (const auto& net : net_list) {
                if (common_nets_.find(net) != common_nets_.end()) {
                    // must NOT be iterated !
                    file << net << " ";
                } else {
                    // must be iterated !
                    file << net << "\\<" << idx << "\\>" << " ";
                }
            }
            file.seekp(-1, std::ios_base::cur);

            file << ") " << subckt_name << "\n";

            // Add the increment (address of fired pixel) on each waveform to save, and concatenate a single string (added later to the generated netlist)
            for (const auto& wave: waveform_to_save_) {
                to_be_saved << wave << "\\<" << idx << "\\> ";
            }

        }
    }

    for (current_line++; current_line < std::max(source_line_number, subckt_line_number)-1; current_line++){
        file << file_lines[current_line] << '\n';
    }

    for (current_line++; current_line < file_lines.size(); current_line++){
        file << file_lines[current_line] << '\n';
    }

    //'save' line
    file << "save " << to_be_saved.str() << '\n';

    to_be_saved.str("");
    to_be_saved.clear();

    file.close();
    LOG(DEBUG) << "File closed";


    // Runs the external uelec simulation, if selected in the configuration file, on the same terminal (ie. with uelec soft. env variables loaded)
    if (run_netlist_simulation_ == true) {

        const auto nutascii_file_name = createOutputFile("output_simulator_event_" + std::to_string(event->number) + ".raw");
        // const auto directory = config_.get<std::string>("_output_dir");

        std::string uelec_sim_command = "spectre ";
        uelec_sim_command += simulator_options_;
        uelec_sim_command += " -f nutascii -r ";
        uelec_sim_command += nutascii_file_name;
        uelec_sim_command += " ";
        uelec_sim_command += file_name;

        LOG(INFO) << uelec_sim_command;

        std::system(uelec_sim_command.c_str());
        
    }

}


void SPICENetlistWriterModule::finalize() {
    // Possibly perform finalization of the module - if not, this method does not need to be implemented and can be removed!
    LOG(INFO) << "Successfully finalized!";
}
