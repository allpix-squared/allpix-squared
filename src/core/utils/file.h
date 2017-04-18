/**
 * Collection of simple file system utilities
 *
 * NOTE: can be replaced by STL filesystem API when we move to C++17
 *
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_FILE_H
#define ALLPIX_FILE_H

#include <cstdlib>
#include <stdexcept>
#include <string>

namespace allpix {

    // Get absolute path from relative path (only works if exists)
    inline std::string get_absolute_path(const std::string& path) {
        // convert file name to absolute path
        char* path_ptr = realpath(path.c_str(), nullptr);
        if(path_ptr == nullptr) {
            // NOTE: the path should exists if it is given
            throw std::invalid_argument("path " + path + " not found");
        }
        std::string abs_path(path_ptr);
        free(static_cast<void*>(path_ptr));
        return abs_path;
    }
}

#endif /* ALLPIX_FILE_H */
