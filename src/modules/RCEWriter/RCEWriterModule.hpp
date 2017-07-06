#include <map>
#include <string>

#include <TFile.h>
#include <TTree.h>

#include "core/config/Configuration.hpp"

#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"

#include "core/module/Module.hpp"
#include "objects/PixelCharge.hpp"
#include "objects/PixelHit.hpp"

namespace allpix {
    class RCEWriterModule : public Module {
    public:
        RCEWriterModule(Configuration config, Messenger*, GeometryManager*);
        ~RCEWriterModule() override;

        // Open the ROOT file to write to
        void init() override;

        // Write current event messages to the output
        void run(unsigned int) override;

        // Finalize the reading
        void finalize() override;

    private:
        Configuration config_;

        GeometryManager* geo_mgr_;

        std::vector<std::string> detector_names_;

        struct SensorData {
            std::unique_ptr<TTree> tree;
            Int_t nhits_;
            Int_t pix_x_[1024];
            Int_t pix_y_[1024];
            Int_t value_[1024];
            Int_t timing_[1024];
            Int_t hit_in_cluster_[1024];
        };

        std::unique_ptr<TFile> output_file_;
        std::unique_ptr<TTree> event_tree_;
        std::map<std::string, SensorData> sensors_;
        std::string det_dir_name;

        ULong64_t timestamp_;
        ULong64_t frame_number_;
        Int_t trigger_offset_;
        Int_t trigger_info_;
        Int_t trigger_time_;
        Bool_t invalid_;

        // retrieve message containing collected pixel hits for all detectors
        std::vector<std::shared_ptr<PixelHitMessage>> pixel_hit_messages_;
    };
} // namespace allpix
