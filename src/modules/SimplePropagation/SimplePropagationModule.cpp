/**
 * @author Paul Schuetze <paul.schuetze@desy.de>
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "SimplePropagationModule.hpp"

#include <string>

#include "core/config/Configuration.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/utils/log.h"

using namespace allpix;

const std::string SimplePropagationModule::name = "SimplePropagation";

SimplePropagationModule::SimplePropagationModule(Configuration config,
                                                 Messenger* messenger,
                                                 std::shared_ptr<Detector> detector)
    : Module(detector), config_(std::move(config)), detector_(std::move(detector)), deposits_message_(nullptr) {
    messenger->bindSingle(this, &SimplePropagationModule::deposits_message_);
}
SimplePropagationModule::~SimplePropagationModule() = default;

// run the deposition
void SimplePropagationModule::run() {
    LOG(DEBUG) << "TEST";
}
