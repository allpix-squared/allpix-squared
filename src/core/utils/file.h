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
} // namespace allpix

#endif /* ALLPIX_FILE_H */
