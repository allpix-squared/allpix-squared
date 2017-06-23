#include <string>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

namespace allpix {
    class DefaultModelReaderModule : public Module {
    public:
        // Read the models from the global configuration
        DefaultModelReaderModule(Configuration config, Messenger* messenger, GeometryManager* geo_mgr);

    private:
        // Parses the file and construct a model
        std::shared_ptr<DetectorModel> parse_config(const Configuration&);

        Configuration config_;
        GeometryManager* geo_mgr_;
    };
} // namespace allpix
