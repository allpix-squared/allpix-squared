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

#include <string>
#include <utility>

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

    node_name_ = config.get<std::string>("node_name");
    node_enumerator_ = std::make_unique<TFormula>("node_enumerator", (config_.get<std::string>("node_enumerator")).c_str());

    if(!node_enumerator_->IsValid()) {
        throw InvalidValueError(config_, "node_enumerator", "Node enumerator is not a valid ROOT::TFormula expression.");
    }

    auto parameters = config_.getArray<double>("node_enumerator_parameters");

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

    // FIXME parse initial netlist input
}

void SPICENetlistWriterModule::run(Event* event) {

    // Messages: Fetch the (previously registered) messages for this event from the messenger:
    auto message = messenger_->fetchMessage<PixelChargeMessage>(this, event);

    // Prepare output file for this event:
    const auto file_name = createOutputFile("event" + std::to_string(event->number) + ".scs");
    auto file = std::make_unique<std::ofstream>(file_name);

    // FIXME write top part of netlist
    // *file << [...]

    for(const auto& pixel_charge : message->getData()) {
        auto pixel = pixel_charge.getPixel();
        auto pixel_index = pixel.getIndex();
        auto inputcharge = static_cast<double>(pixel_charge.getCharge());

        LOG(DEBUG) << "Received pixel " << pixel_index << ", charge " << Units::display(inputcharge, "e");

        const auto& pulse = pixel_charge.getPulse(); // the pulse containing charges and times

        // FIXME maybe some output PWL do not require a full pulse?
        if(!pulse.isInitialized()) {
            throw ModuleError("No pulse information available.");
        }

        // auto timestep = pulse.getBinning();

        // FIXME write IPWL to netlist
        *file << node_name_ << "\\<" << node_enumerator_->Eval(pixel_index.x(), pixel_index.y()) << "\\>";
    }

    // FIXME write bottom part to netlist
}

void SPICENetlistWriterModule::finalize() {
    // Possibly perform finalization of the module - if not, this method does not need to be implemented and can be removed!
    LOG(INFO) << "Successfully finalized!";
}
