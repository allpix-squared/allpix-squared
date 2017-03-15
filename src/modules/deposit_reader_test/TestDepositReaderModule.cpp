/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "TestDepositReaderModule.hpp"

#include "core/messenger/Messenger.hpp"
#include "core/utils/log.h"
#include "messages/DepositionMessage.hpp"

using namespace allpix;

const std::string TestDepositReaderModule::name = "deposit_reader_test";

TestDepositReaderModule::TestDepositReaderModule(AllPix* apx, Configuration config)
    : Module(apx), config_(std::move(config)), deposit_messages_() {
    getMessenger()->bindMulti(this, &TestDepositReaderModule::deposit_messages_);
}
TestDepositReaderModule::~TestDepositReaderModule() = default;

// run the deposition
void TestDepositReaderModule::run() {
    LOG(INFO) << "DEPOSIT READER RESULTS";

    LOG(INFO) << "got " << deposit_messages_.size() << " deposits";
    // FIXME: detector logic is not good yet
    for(auto& message : deposit_messages_) {
        LOG(DEBUG) << " list of deposits";
        for(auto& deposit : message->getDeposits()) {
            TVector3 pos = deposit.getPosition();
            LOG(DEBUG) << "  energy " << deposit.getEnergy() << " at point (" << pos.x() << "," << pos.y() << "," << pos.z()
                       << ")"; //<< " in detector " << getDetector().getName();
        }
    }
}
