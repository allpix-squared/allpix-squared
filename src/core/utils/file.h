/**
 * @file
 * @brief Collection of simple file system utilities
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_FILE_H
#define ALLPIX_FILE_H

#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include <sys/stat.h>

namespace allpix {

    /**
     * @brief Get canonical path from a file or directory name
     * @return The canonical path (without dots)
     * @throws std::invalid_argument If absolute path does not exist on the system
     */
    inline std::string get_canonical_path(const std::string& path) {
        if(!std::filesystem::exists(path)) {
            // Throw an error if the path does not exist
            throw std::invalid_argument("path " + path + " not found");
        }
        return std::filesystem::canonical(path);
    }

    /**
     * @brief Check if path is an existing directory
     * @param path The path to check
     * @return True if the path is a directory, false otherwise
     */
    inline bool path_is_directory(const std::string& path) { return std::filesystem::is_directory(path); }

    /**
     * @brief Check if path is an existing file
     * @param path The path to check
     * @return True if the path is a file, false otherwise
     */
    inline bool path_is_file(const std::string& path) { return std::filesystem::is_regular_file(path); }

    /**
     * @brief Check if the file is a binary file
     * @param path The path to the file to be checked check
     * @return True if the file contains null bytes, false otherwise
     *
     * This helper function checks the first 256 characters of a file for the occurrence of a nullbyte.
     * For binary files it is very unlikely not to have at least one. This approach is also used e.g. by diff
     */
    inline bool file_is_binary(const std::string& path) {
        std::ifstream file(path);
        for(size_t i = 0; i < 256; i++) {
            if(file.get() == '\0') {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Create a directory
     * @param path The path to create
     * @param mode The flags permissions of the file to create
     * @throws std::invalid_argument If the directory or one of its subpaths cannot be created
     *
     * All the required directories are created from the top-directory until the last folder. Therefore it can be used to
     * create a structure of directories.
     */
    inline void create_directories(std::string path, mode_t mode = 0777) {
        try {
            std::filesystem::create_directories(path);
            std::filesystem::permissions(path, std::filesystem::perms(mode));
        } catch(std::filesystem::filesystem_error& e) {
            throw std::invalid_argument("cannot create path: " + std::string(e.what()));
        }
    }

    /**
     * @brief Check for the existence of the file extension and add it if not present
     * @param path File name or path to file
     * @param extension File extension (without separating dot) to be checked for or added
     * @return File name or path to file including the appropriate file extension
     */
    inline std::string add_file_extension(const std::string& path, std::string extension) {
        if(extension.empty()) {
            return path;
        }
        return std::filesystem::path(path).replace_extension(extension);
    }
} // namespace allpix

#endif /* ALLPIX_FILE_H */
