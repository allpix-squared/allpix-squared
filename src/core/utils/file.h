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

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

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

    // Check if path is a existing directory
    inline bool path_is_directory(const std::string& path) {
        struct stat path_stat;
        if(stat(path.c_str(), &path_stat) == -1) {
            return false;
        }
        return S_ISDIR(path_stat.st_mode);
    }

    // Check if path is a existing file
    inline bool path_is_file(const std::string& path) {
        struct stat path_stat;
        if(stat(path.c_str(), &path_stat) == -1) {
            return false;
        }
        return S_ISREG(path_stat.st_mode);
    }

    // Get all files in directory
    inline std::vector<std::string> get_files_in_directory(const std::string& path) {
        std::vector<std::string> files;
        struct dirent* ent = nullptr;
        struct stat st;

        // loop through all files
        DIR* dir = opendir(path.c_str());
        while((ent = readdir(dir)) != nullptr) {
            const std::string file_name = ent->d_name;
            const std::string full_file_name = path + "/" + file_name;

            // ignore useless or wrong paths
            if(!file_name.empty() && file_name[0] == '.')
                continue;
            if(stat(full_file_name.c_str(), &st) == -1)
                continue;

            // ignore subdirectories
            const bool is_directory = (st.st_mode & S_IFDIR) != 0;
            if(is_directory)
                continue;

            // add full file paths
            files.push_back(full_file_name);
        }
        closedir(dir);
        return files;
    }
}

#endif /* ALLPIX_FILE_H */
