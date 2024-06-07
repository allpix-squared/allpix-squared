/**
 * @file
 * @brief Core object of the configuration system
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#ifndef ALLPIX_CONFIGURATION_H
#define ALLPIX_CONFIGURATION_H

#include <atomic>
#include <filesystem>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include "core/config/exceptions.h"
#include "core/utils/text.h"

namespace allpix {

    template <typename T> using Matrix = std::vector<std::vector<T>>;

    /**
     * @brief Generic configuration object storing keys
     *
     * The configuration holds a set of keys with arbitrary values that are internally stored as strings. It has special
     * logic for reading paths (relative to the configuration file). All types are converted to their appropriate type using
     * the library of \ref StringConversions.
     */
    class Configuration {

        /**
         * @brief Helper class to keep track of key access
         *
         * This class holds all configuration keys in a map together with an atomic boolean marking whether they have been
         * accessed already. This allows to find out which keys have not been accessed at all. This wrapper allows to use
         * atomics for non-locking access but requires to register all keys beforehand.
         */
        class AccessMarker {
        public:
            /**
             * Default constructor
             */
            AccessMarker() = default;

            /**
             * @brief Explicit copy constructor to allow copying of the map keys
             */
            AccessMarker(const AccessMarker& rhs);

            /**
             * @brief Explicit copy assignment operator to allow copying of the map keys
             */
            AccessMarker& operator=(const AccessMarker& rhs);

            /**
             * @brief Method to register a key for a new access marker
             * @param key Key of the marker
             * @warning This operation is not thread-safe
             */
            void registerMarker(const std::string& key);

            /**
             * @brief Method to mark existing marker as accessed/used.
             * @param key Key of the marker
             * @note This is an atomic operation and thread-safe.
             * @throws std::out_of_range if the key has not been registered beforehand
             */
            void markUsed(const std::string& key) { markers_.at(key).store(true); };

            /**
             * @brief Method to retrieve access status of an existing marker.
             * @param key Key of the marker
             * @note This is an atomic operation and thread-safe.
             * @throws std::out_of_range if the key has not been registered beforehand
             */
            bool isUsed(const std::string& key) { return markers_.at(key).load(); }

        private:
            std::map<std::string, std::atomic<bool>> markers_;
        };

    public:
        /**
         * @brief Construct a configuration object
         * @param name Name of the section header (empty section if not specified)
         * @param path Path to the file containing the configuration (or empty if not stored in a file)
         */
        explicit Configuration(std::string name = "", std::filesystem::path path = "");

        /// @{
        /**
         * @brief Allow copying the configuration
         */
        Configuration(const Configuration&) = default;
        Configuration& operator=(const Configuration&) = default;
        /// @}

        /// @{
        /**
         * @brief Allow moving the configuration
         */
        Configuration(Configuration&&) noexcept = default; // NOLINT
        Configuration& operator=(Configuration&&) = default;
        /// @}

        /**
         * @brief Check if key is defined
         * @param key Key to check for existence
         * @return True if key exists, false otherwise
         */
        bool has(const std::string& key) const;

        /**
         * @brief Check how many of the given keys are defined
         * @param keys Keys to check for existence
         * @return number of existing keys from the given list
         */
        unsigned int count(std::initializer_list<std::string> keys) const;

        /**
         * @brief Get value of a key in requested type
         * @param key Key to get value of
         * @return Value of the key in the type of the requested template parameter
         */
        template <typename T> T get(const std::string& key) const;
        /**
         * @brief Get value of a key in requested type or default value if it does not exists
         * @param key Key to get value of
         * @param def Default value to use if key is not defined
         * @return Value of the key in the type of the requested template parameter
         *         or the default value if the key does not exists
         */
        template <typename T> T get(const std::string& key, const T& def) const;

        /**
         * @brief Get values for a key containing an array
         * @param key Key to get values of
         * @return List of values in the array in the requested template parameter
         */
        // TODO [doc] Provide second template parameter to specify the vector type to return it in
        template <typename T> std::vector<T> getArray(const std::string& key) const;

        /**
         * @brief Get values for a key containing an array or default array if it does not exists
         * @param key Key to get values of
         * @param def Default value array to use if key is not defined
         * @return List of values in the array in the requested template parameter
         *         or the default array if the key does not exist
         */
        // TODO [doc] Provide second template parameter to specify the vector type to return it in
        template <typename T> std::vector<T> getArray(const std::string& key, const std::vector<T> def) const;

        /**
         * @brief Get values for a key containing a 2D matrix
         * @param key Key to get values of
         * @return Matrix of values from the requested template parameter
         */
        template <typename T> Matrix<T> getMatrix(const std::string& key) const;

        /**
         * @brief Get values for a key containing a 2D matrix
         * @param key Key to get values of
         * @param def Default value matrix to use if key is not defined
         * @return Matrix of values from the requested template parameter
         */
        template <typename T> Matrix<T> getMatrix(const std::string& key, const Matrix<T> def) const;

        /**
         * @brief Get literal value of a key as string
         * @param key Key to get values of
         * @return Literal value of the key
         * @note This function does also not remove quotation marks in strings
         */
        std::string getText(const std::string& key) const;
        /**
         * @brief Get literal value of a key as string or a default if it does not exists
         * @param key Key to get values of
         * @param def Default value to use if key is not defined
         * @return Literal value of the key or the default value if the key does not exists
         * @note This function does also not remove quotation marks in strings
         */
        std::string getText(const std::string& key, const std::string& def) const;

        /**
         * @brief Get absolute path to file with paths relative to the configuration
         * @param key Key to get path of
         * @param check_exists If the file should be checked for existence (if yes always returns a canonical path)
         * @return Absolute path to a file
         */
        std::filesystem::path getPath(const std::string& key, bool check_exists = false) const;

        /**
         * @brief Get absolute path to file with paths relative to the configuration
         * @param key Key to get path of
         * @param extension File extension to be added to path if not present
         * @param check_exists If the file should be checked for existence (if yes always returns a canonical path)
         * @return Absolute path to a file
         */
        std::filesystem::path
        getPathWithExtension(const std::string& key, const std::string& extension, bool check_exists) const;
        /**
         * @brief Get array of absolute paths to files with paths relative to the configuration
         * @param key Key to get path of
         * @param check_exists If the files should be checked for existence (if yes always returns a canonical path)
         * @return List of absolute path to all the requested files
         */
        // TODO [doc] Provide second template parameter to specify the vector type to return it in
        std::vector<std::filesystem::path> getPathArray(const std::string& key, bool check_exists = false) const;

        /**
         * @brief Set value for a key in a given type
         * @param key Key to set value of
         * @param val Value to assign to the key
         * @param mark_used Flag whether key should be marked as "used" directly
         */
        template <typename T> void set(const std::string& key, const T& val, bool mark_used = false);

        /**
         * @brief Store value for a key in a given type, including units
         * @param key Key to set value of
         * @param val Value to assign to the key
         * @param units List of possible output units
         */
        template <typename T> void set(const std::string& key, const T& val, std::initializer_list<std::string> units);

        /**
         * @brief Set list of values for a key in a given type
         * @param key Key to set values of
         * @param val List of values to assign to the key
         * @param mark_used Flag whether key should be marked as "used" directly
         */
        // TODO [doc] Provide second template parameter to specify the vector type to return it in
        template <typename T> void setArray(const std::string& key, const std::vector<T>& val, bool mark_used = false);

        /**
         * @brief Set matrix of values for a key in a given type
         * @param key Key to set values of
         * @param val List of values to assign to the key
         */
        template <typename T> void setMatrix(const std::string& key, const Matrix<T>& val);

        /**
         * @brief Set default value for a key only if it is not defined yet
         * @param key Key to possible set value of
         * @param val Value to assign if the key is not defined yet
         * @note This marks the default key as "used" automatically
         */
        template <typename T> void setDefault(const std::string& key, const T& val);
        /**
         * @brief Set default list of values for a key only if it is not defined yet
         * @param key Key to possible set values of
         * @param val List of values to assign to the key if the key is not defined yet
         * @note This marks the default key as "used" automatically
         */
        template <typename T> void setDefaultArray(const std::string& key, const std::vector<T>& val);

        /**
         * @brief Set literal value of a key as string
         * @param key Key to set value of
         * @param val Literal string to assign key to
         */
        void setText(const std::string& key, const std::string& val);

        /**
         * @brief Set alias name for an already existing key
         * @param new_key New alias to be created
         * @param old_key Key the alias is created for
         * @param warn Optionally print a warning message to notify of deprecation
         * @note This marks the old key as "used" automatically
         */
        void setAlias(const std::string& new_key, const std::string& old_key, bool warn = false);

        /**
         * @brief Return total number of key / value pairs
         * @return Number of settings
         */
        unsigned int countSettings() const { return static_cast<unsigned int>(config_.size()); }

        /**
         * @brief Get name of the configuration header
         * @return Configuration name
         */
        const std::string& getName() const { return name_; }

        /**
         * @brief Get path to the file containing the configuration if it has one
         * @return Absolute path to configuration file or empty if not linked to a file
         * @warning Parameter should be used with care as not all configurations are required to have a file
         */
        std::filesystem::path getFilePath() const { return path_; }

        /**
         * @brief Merge other configuration, only adding keys that are not yet defined in this configuration
         * @param other Configuration to merge this one with
         */
        void merge(const Configuration& other);

        /**
         * @brief Get all key value pairs
         * @return List of all key value pairs
         */
        // FIXME Better name for this function
        std::vector<std::pair<std::string, std::string>> getAll() const;

        /**
         * @brief Obtain all keys which have not been accessed yet
         *
         * This method returns all keys from the configuration object which have not yet been accessed, Default values as
         * well as aliases are marked as used automatically and are therefore never returned.
         */
        std::vector<std::string> getUnusedKeys() const;

    private:
        /**
         * @brief Make relative paths absolute from this configuration file
         * @param path Path to make absolute (if it is not already absolute)
         * @param canonicalize_path If the path should be canonicalized (throws an error if the path does not exist)
         */
        std::filesystem::path path_to_absolute(std::filesystem::path path, bool canonicalize_path) const;

        /**
         * @brief Node in a parse tree
         */
        struct parse_node {
            std::string value;
            std::vector<std::unique_ptr<parse_node>> children;
        };
        /**
         * @brief Generate parse tree from configuration string
         * @param str String to parse
         * @param depth Current depth of the parsing (starts at zero)
         * @return Root node of the parsed tree
         */
        static std::unique_ptr<parse_node> parse_value(std::string str, int depth = 0);

        std::string name_;
        std::filesystem::path path_;

        using ConfigMap = std::map<std::string, std::string>;
        ConfigMap config_;
        mutable AccessMarker used_keys_;
    };
} // namespace allpix

// Include template members
#include "Configuration.tpp"

#endif /* ALLPIX_CONFIGURATION_H */
