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
    config_.setDefault<TargetSpice>("target", TargetSpice::SPECTRE);
    target_ = config.get<TargetSpice>("target");

    // Get the template netlist to modify:
    netlist_path_ = config.getPath("netlist_template", true);

    node_name_ = config.get<std::string>("node_name");
    auto detector_model = detector_->getModel();
    std::string default_enumerator = "x * " + std::to_string(detector_model->getNPixels().Y()) + " + y";
    node_enumerator_ = std::make_unique<TFormula>("node_enumerator", (config_.get<std::string>("node_enumerator", default_enumerator)).c_str());

    config_.setDefault<NodeType>("node_type", NodeType::CURRENTSOURCE);
    node_type_ = config.get<NodeType>("node_type");

    if(!node_enumerator_->IsValid()) {
        throw InvalidValueError(config_, "node_enumerator", "Node enumerator is not a valid ROOT::TFormula expression.");
    }

    auto parameters = config_.getArray<double>("node_enumerator_parameters", {});

    // check if number of parameters match up
    if(static_cast<size_t>(node_enumerator_->GetNpar()) != parameters.size()) {
        throw InvalidValueError(
            config_,
            "node_enumerator_parameters",
            "The number of function parameters does not line up with the number of parameters in the function.");
    }

    for(size_t n = 0; n < parameters.size(); ++n) {
        node_enumerator_->SetParameter(static_cast<int>(n), parameters[n]);
    }

    LOG(DEBUG) << "Node enumerator function successfully initialized with " << parameters.size() << " parameters";
}

void SPICENetlistWriterModule::initialize() {
   
    int line_number = 0;
    std::ifstream netlist_file (netlist_path_);
    std::string line;
    std::ostringstream eof_netlist;
    std::ostringstream header_netlist;
    std::regex connection_regex("\\((.+)\\)");

    while (getline(netlist_file, line)) {
        // Writes the content of the netlist file to the new one
        // Identifies the isource declaration line
        // Identifies the isource instance nodes names
        line_number++;
        file_lines.push_back(line);

        if(line.rfind(node_name_, 0) == 0){
            isource_line_number = line_number;
            if(std::regex_search(line, connection_match, connection_regex)) {
                LOG(STATUS) << "connections: " << connection_match[0];
                connections_ = connection_match[0];
            } else {
                throw ModuleError("Could not find node connections");
            }
            LOG(STATUS) << "Found the isource line!";
        }
    }

    // find position of the last character to keep in the isource declaration
    size_t p = file_lines[isource_line_number].find("[");

    // only keep the isource line before the waveform
    isource_line = file_lines[isource_line_number].substr(0, p);
    LOG(STATUS) << "End of initialize";
}

void SPICENetlistWriterModule::run(Event* event) {
    LOG(STATUS) << "Module entered the run loop";
    // Messages: Fetch the (previously registered) messages for this event from the messenger:
    auto message = messenger_->fetchMessage<PixelChargeMessage>(this, event);

    // Prepare output file for this event:
    const auto file_name = createOutputFile("test_event" + std::to_string(event->number) + ".scs");
    auto file = std::ofstream(file_name);
    LOG(STATUS) << "Output file(s) created";

        

    for (int i=0; i < isource_line_number-1; i++){
        file << file_lines[i] << '\n';
    }

    for(const auto& pixel_charge : message->getData()) {
        auto pixel = pixel_charge.getPixel();
        auto pixel_index = pixel.getIndex();
        auto inputcharge = static_cast<double>(pixel_charge.getCharge());
        std::ostringstream pulse_string;
        
        LOG(STATUS) << "Received pixel " << pixel_index << ", charge " << Units::display(inputcharge, "e");

        ///////////////////////////////////////////////

        // Get pulse and timepoints
        const auto& pulse = pixel_charge.getPulse(); // the pulse containing charges and times


        // FIXME maybe some output PWL do not require a full pulse?
        if(!pulse.isInitialized()) {
            throw ModuleError("No pulse information available.");
        }

        double step = pulse.getBinning();

        file << node_name_ << "\\<" << node_enumerator_->Eval(pixel_index.x(), pixel_index.y()) << "\\> ";
        file << connections_ << " isource type=pwl wave=[";

        for (auto bin = pulse.begin(); bin != pulse.end(); ++bin){
            auto time = step * static_cast<double>(std::distance(pulse.begin(), bin)) * 1e-9;
            double current_bin = *bin / step;
            double current = current_bin* nanoCoulomb;

            file << std::setprecision(15) << time << " " << current << (bin < pulse.end() - 1 ? " " : "");
        }
        file << "]\n";

    

        // FIXME write IPWL to netlist
    }

    for (int i=isource_line_number; i < file_lines.size(); i++){
        file << file_lines[i] << '\n';
    }

    file.close();
    LOG(STATUS) << "File closed";

    
}

void SPICENetlistWriterModule::finalize() {
    // Possibly perform finalization of the module - if not, this method does not need to be implemented and can be removed!
    LOG(INFO) << "Successfully finalized!";
}
