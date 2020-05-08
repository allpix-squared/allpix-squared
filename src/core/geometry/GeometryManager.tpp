/**
 * @file
 * @brief Template implementation of geometry manager
 *
 * @copyright Copyright (c) 2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

namespace allpix {
    /**
     * If the returned object is not a null pointer it is guaranteed to be of the correct type
     */
    template <typename T>
    std::shared_ptr<T> GeometryManager::getExternalObject(const std::string& associated_name, const std::string& id) {
        return std::static_pointer_cast<T>(external_objects_[typeid(T)][std::make_pair(associated_name, id)]);
    }

    /**
     * Stores external representations of objects in this detector that need to be shared between modules.
     */
    template <typename T>
    void
    GeometryManager::setExternalObject(const std::string& associated_name, const std::string& id, std::shared_ptr<T> model) {
        external_objects_[typeid(T)][std::make_pair(associated_name, id)] = std::static_pointer_cast<void>(model);
        if(std::find(external_object_names_.begin(), external_object_names_.end(), associated_name) ==
           external_object_names_.end()) {
            external_object_names_.push_back(associated_name);
        }
    }
} // namespace allpix
