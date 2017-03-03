/*
 * Read the deposit charges and just display them
 */

#ifndef ALLPIX_TEST_DEPOSIT_READER_MODULE_H
#define ALLPIX_TEST_DEPOSIT_READER_MODULE_H

#include <memory>
#include <string>

#include "../../core/module/Module.hpp"
#include "../../core/config/Configuration.hpp"

namespace allpix{
    class DepositionMessage;
    
    // define the module to inherit from the module base class
    class TestDepositReaderModule : public Module{
    public:
        // provide a static const variable of type string (required!)
        static const std::string name;
        
        // constructor should take a pointer to AllPix, a ModuleIdentifier and a Configuration as input
        TestDepositReaderModule(AllPix *apx, ModuleIdentifier id, Configuration config);
        ~TestDepositReaderModule();
        
        // show the deposition results
        void run();
        
    private:
        // configuration for this module
        Configuration config_;
        
        // pointer to the visualization manager        
        std::vector<std::shared_ptr<DepositionMessage>> deposit_messages_;
    };
}

#endif /* ALLPIX_TEST_DEPOSIT_READER_MODULE_H */
