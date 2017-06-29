#include <map>
#include <string>

#include <TFile.h>
#include <TTree.h>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"

#include "core/module/Module.hpp"

namespace allpix {
    class ROOTObjectWriterModule : public Module {
    public:
        ROOTObjectWriterModule(Configuration config, Messenger*, GeometryManager*);
        ~ROOTObjectWriterModule() override;

        // Receive single messages
        void receive(std::shared_ptr<BaseMessage> message, std::string name);

        // Open the ROOT file to write to
        void init() override;

        // Write current event messages to the output
        void run(unsigned int) override;

        // Finalize the reading
        void finalize() override;

    private:
        std::vector<std::shared_ptr<BaseMessage>> keep_messages_;
        std::map<std::tuple<std::type_index, std::string, std::string>, std::vector<Object*>*> write_list_;
        unsigned long write_cnt_{};

        Configuration config_;
        GeometryManager* geo_mgr_;

        std::unique_ptr<TFile> output_file_;
        std::map<std::string, std::unique_ptr<TTree>> trees_;
    };
} // namespace allpix
