/*
 * Read the deposit charges and just display them
 */

#ifndef ALLPIX_TEST_DEPOSIT_READER_MODULE_H
#define ALLPIX_TEST_DEPOSIT_READER_MODULE_H

#include <memory>
#include <string>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

#include "objects/DepositedCharge.hpp"

namespace allpix {
    class DepositionMessage;

    // define the module to inherit from the module base class
    class TestDepositReaderModule : public Module {
    public:
        // constructor should take a pointer to the Configuration, the Messenger and the Geometry Manager
        TestDepositReaderModule(Configuration, Messenger*, GeometryManager*);
        ~TestDepositReaderModule() override;

        // show the deposition results
        void run(unsigned int) override;

    private:
        // configuration for this module
        Configuration config_;

        // list of the messages
        std::vector<std::shared_ptr<DepositedChargeMessage>> deposit_messages_;
    };
} // namespace allpix

#endif /* ALLPIX_TEST_DEPOSIT_READER_MODULE_H */
