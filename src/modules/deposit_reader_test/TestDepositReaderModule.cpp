/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "TestDepositReaderModule.hpp"

#include "core/messenger/Messenger.hpp"
#include "core/utils/log.h"

#include "tools/ROOT.h"

using namespace allpix;

const std::string TestDepositReaderModule::name = "deposit_reader_test";

TestDepositReaderModule::TestDepositReaderModule(Configuration config, Messenger* messenger, GeometryManager*)
    : config_(std::move(config)), deposit_messages_() {
    messenger->bindMulti(this, &TestDepositReaderModule::deposit_messages_);
}
TestDepositReaderModule::~TestDepositReaderModule() = default;

// run the deposition
void TestDepositReaderModule::run() {
    LOG(INFO) << "READ DEPOSITS";

    LOG(INFO) << "got " << deposit_messages_.size() << " deposits";
    for(auto& message : deposit_messages_) {
        LOG(DEBUG) << " list of deposits";
        for(auto& deposit : message->getData()) {
            auto pos = deposit.getPosition();
            LOG(DEBUG) << "  energy " << deposit.getEnergy() << " at point (" << pos.x() << "," << pos.y() << "," << pos.z()
                       << ")"
                       << " in detector " << message->getDetector()->getName();
        }
    }
}
