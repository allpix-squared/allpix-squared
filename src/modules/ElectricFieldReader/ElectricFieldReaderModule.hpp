/**
 * @file
 * @brief Definition of module to read electric fields
 * @copyright MIT License
 */

/*
 * Module to read electric fields in the INIT format
 * See https://github.com/simonspa/pixelav
 *
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"

#include "core/module/Module.hpp"

namespace allpix {
    // Define the module inheriting from the module base class
    class ElectricFieldReaderModule : public Module {
        using FieldData = std::pair<std::shared_ptr<std::vector<double>>, std::array<size_t, 3>>;

    public:
        // Constructor of the module
        ElectricFieldReaderModule(Configuration config, Messenger*, std::shared_ptr<Detector>);

        // Read the electric field and set it for the detectors
        void init() override;

    private:
        // Construct linear field
        FieldData construct_linear_field();

        // Read init field
        FieldData read_init_field();

        // Create output plots
        void create_output_plots();

        // Get the electric field from a file name
        static FieldData get_by_file_name(const std::string& name, Detector&);
        static std::map<std::string, FieldData> field_map_;

        // configuration for this reader
        Configuration config_;

        // linked detector for this instantiation
        std::shared_ptr<Detector> detector_;
    };
} // namespace allpix
