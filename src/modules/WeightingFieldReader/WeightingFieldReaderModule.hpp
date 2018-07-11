/**
 * @file
 * @brief Definition of module to read weighting fields
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied
 * verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities
 * granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
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
    /**
     * @ingroup Modules
     * @brief Module to read weighting fields from INIT format
     */
    class WeightingFieldReaderModule : public Module {
        using FieldData = std::pair<std::shared_ptr<std::vector<double>>, std::array<size_t, 3>>;

    public:
        /**
         * @brief Constructor for this detector-specific module
         * @param config Configuration object for this module as retrieved from the
         * steering file
         * @param messenger Pointer to the messenger object to allow binding to
         * messages on the bus
         * @param detector Pointer to the detector for this module instance
         */
        WeightingFieldReaderModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector);

        /**
         * @brief Read weighting field and apply it to the bound detectors
         */
        void init() override;

    private:
        std::shared_ptr<Detector> detector_;

        /**
         * @brief Create and apply a weighting field equivalent to a pixel/pad in a plane condenser
         * @param thickness_domain Domain of the thickness where the field is defined
         */
        ElectricFieldFunction get_pad_field_function(const ROOT::Math::XYVector& implant,
                                                     std::pair<double, double> thickness_domain);

        /**
         * @brief Read field in the init format and apply it
         */
        FieldData read_init_field();

        /**
         * @brief Create output plots of the electric field profile
         */
        void create_output_plots();

        /**
         * @brief Get the weighting field from a file name, caching the result between
         * instantiations
         */
        static FieldData get_by_file_name(const std::string& name, Detector&);
        static std::map<std::string, FieldData> field_map_;
    };
} // namespace allpix
