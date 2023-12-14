/**
 * @file
 * @brief Template implementation of configuration
 *
 * @copyright Copyright (c) 2017-2023 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

namespace allpix {
    /**
     * @throws MissingKeyError If the requested key is not defined
     * @throws InvalidKeyError If the conversion to the requested type did not succeed
     * @throws InvalidKeyError If an overflow happened while converting the key
     */
    template <typename T> T Configuration::get(const std::string& key) const {
        try {
            auto node = parse_value(config_.at(key));
            used_keys_.markUsed(key);
            try {
                return allpix::from_string<T>(node->value);
            } catch(std::invalid_argument& e) {
                throw InvalidKeyError(key, getName(), node->value, typeid(T), e.what());
            }
        } catch(std::out_of_range& e) {
            throw MissingKeyError(key, getName());
        } catch(std::invalid_argument& e) {
            throw InvalidKeyError(key, getName(), config_.at(key), typeid(T), e.what());
        } catch(std::overflow_error& e) {
            throw InvalidKeyError(key, getName(), config_.at(key), typeid(T), e.what());
        }
    }
    /**
     * @throws InvalidKeyError If the conversion to the requested type did not succeed
     * @throws InvalidKeyError If an overflow happened while converting the key
     */
    template <typename T> T Configuration::get(const std::string& key, const T& def) const {
        if(has(key)) {
            return get<T>(key);
        }
        return def;
    }

    /**
     * @throws MissingKeyError If the requested key is not defined
     * @throws InvalidKeyError If the conversion to the requested type did not succeed
     * @throws InvalidKeyError If an overflow happened while converting the key
     */
    template <typename T> std::vector<T> Configuration::getArray(const std::string& key) const {
        try {
            std::string str = config_.at(key);
            used_keys_.markUsed(key);

            std::vector<T> array;
            auto node = parse_value(std::move(str));
            for(auto& child : node->children) {
                try {
                    array.push_back(allpix::from_string<T>(child->value));
                } catch(std::invalid_argument& e) {
                    throw InvalidKeyError(key, getName(), child->value, typeid(T), e.what());
                }
            }
            return array;
        } catch(std::out_of_range& e) {
            throw MissingKeyError(key, getName());
        } catch(std::invalid_argument& e) {
            throw InvalidKeyError(key, getName(), config_.at(key), typeid(T), e.what());
        } catch(std::overflow_error& e) {
            throw InvalidKeyError(key, getName(), config_.at(key), typeid(T), e.what());
        }
    }
    /**
     * @throws InvalidKeyError If the conversion to the requested type did not succeed
     * @throws InvalidKeyError If an overflow happened while converting the key
     */
    template <typename T> std::vector<T> Configuration::getArray(const std::string& key, const std::vector<T> def) const {
        if(has(key)) {
            return getArray<T>(key);
        }
        return std::move(def);
    }

    /**
     * @throws MissingKeyError If the requested key is not defined
     * @throws InvalidKeyError If the conversion to the requested type did not succeed
     * @throws InvalidKeyError If an overflow happened while converting the key
     */
    template <typename T> Matrix<T> Configuration::getMatrix(const std::string& key) const {
        try {
            std::string str = config_.at(key);
            used_keys_.markUsed(key);

            Matrix<T> matrix;
            auto node = parse_value(std::move(str));
            for(auto& child : node->children) {
                if(child->children.empty()) {
                    throw std::invalid_argument("matrix has less than two dimensions, enclosing brackets might be missing");
                }

                std::vector<T> array;
                // Create subarray of matrix
                for(auto& subchild : child->children) {
                    try {
                        array.push_back(allpix::from_string<T>(subchild->value));
                    } catch(std::invalid_argument& e) {
                        throw InvalidKeyError(key, getName(), subchild->value, typeid(T), e.what());
                    }
                }
                matrix.push_back(array);
            }
            return matrix;
        } catch(std::out_of_range& e) {
            throw MissingKeyError(key, getName());
        } catch(std::invalid_argument& e) {
            throw InvalidKeyError(key, getName(), config_.at(key), typeid(T), e.what());
        } catch(std::overflow_error& e) {
            throw InvalidKeyError(key, getName(), config_.at(key), typeid(T), e.what());
        }
    }

    /**
     * @throws InvalidKeyError If the conversion to the requested type did not succeed
     * @throws InvalidKeyError If an overflow happened while converting the key
     */
    template <typename T> Matrix<T> Configuration::getMatrix(const std::string& key, const Matrix<T> def) const {
        if(has(key)) {
            return getMatrix<T>(key);
        }
        return def;
    }

    template <typename T> void Configuration::set(const std::string& key, const T& val, bool mark_used) {
        config_[key] = allpix::to_string(val);
        used_keys_.registerMarker(key);
        if(mark_used) {
            used_keys_.markUsed(key);
        }
    }

    template <typename T>
    void Configuration::set(const std::string& key, const T& val, std::initializer_list<std::string> units) {
        auto split = allpix::split<Units::UnitType>(allpix::to_string(val));

        std::string ret_str;
        for(auto& element : split) {
            ret_str += Units::display(element, units);
            ret_str += ",";
        }
        ret_str.pop_back();
        config_[key] = ret_str;
        used_keys_.registerMarker(key);
    }

    template <typename T> void Configuration::setArray(const std::string& key, const std::vector<T>& val, bool mark_used) {
        // NOTE: not the most elegant way to support arrays
        std::string str;
        for(auto& el : val) {
            str += allpix::to_string(el);
            str += ",";
        }
        str.pop_back();
        config_[key] = std::move(str);
        used_keys_.registerMarker(key);
        if(mark_used) {
            used_keys_.markUsed(key);
        }
    }

    template <typename T> void Configuration::setMatrix(const std::string& key, const Matrix<T>& val) {
        // NOTE: not the most elegant way to support arrays
        if(val.empty()) {
            return;
        }

        std::string str = "[";
        for(auto& col : val) {
            str += "[";
            for(auto& el : col) {
                str += allpix::to_string(el);
                str += ",";
            }
            str.pop_back();
            str += "],";
        }
        str.pop_back();
        str += "]";
        config_[key] = str;
        used_keys_.registerMarker(key);
    }

    template <typename T> void Configuration::setDefault(const std::string& key, const T& val) {
        if(!has(key)) {
            set<T>(key, val, true);
        }
    }

    template <typename T> void Configuration::setDefaultArray(const std::string& key, const std::vector<T>& val) {
        if(!has(key)) {
            setArray<T>(key, val, true);
        }
    }
} // namespace allpix
