#include <functional>
#include <map>
#include <string>

#include <TFile.h>
#include <TTree.h>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"

#include "core/module/Module.hpp"

#include "objects/objects.h"

namespace allpix {
    class ROOTObjectReaderModule : public Module {
    public:
        ROOTObjectReaderModule(Configuration config, Messenger*, GeometryManager*);
        ~ROOTObjectReaderModule() override;

        // Receive single messages
        void receive(std::shared_ptr<BaseMessage> message, std::string name);

        // Open the ROOT file to write to
        void init() override;

        // Write current event messages to the output
        void run(unsigned int) override;

        // Finalize the reading
        void finalize() override;

        using MessageCreatorMap =
            std::map<std::type_index,
                     std::function<std::shared_ptr<BaseMessage>(std::vector<Object*>, std::shared_ptr<Detector>)>>;

    private:
        struct message_info {
            std::vector<Object*>* objects;
            std::shared_ptr<Detector> detector;
            std::string name;
        };

        Configuration config_;
        Messenger* messenger_;
        GeometryManager* geo_mgr_;

        std::unique_ptr<TFile> input_file_;
        TTree* tree_;
        std::list<message_info> message_info_array_;
        unsigned long read_cnt_{};

        MessageCreatorMap message_creator_map_;
    };
} // namespace allpix
