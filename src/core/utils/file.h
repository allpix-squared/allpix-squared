/**
 * @file
 * @brief Collection of simple file system utilities
 * @copyright MIT License
 */

#ifndef ALLPIX_FILE_H
#define ALLPIX_FILE_H

#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// NOTE: can be replaced by STL filesystem API when we move to C++17
// TODO [DOC] this should be moved to a sub namespace

namespace allpix {

    /**
     * @brief Get canonical path from a file or directory name
     * @return The canonical path (without dots)
     * @throws std::invalid_argument If absolute path does not exist on the system
     */
    // TODO [DOC] should be renamed get_canonical_path
    inline std::string get_absolute_path(const std::string& path) {
        char* path_ptr = realpath(path.c_str(), nullptr);
        if(path_ptr == nullptr) {
            // Throw an error if the path does not exist
            throw std::invalid_argument("path " + path + " not found");
        }
        std::string abs_path(path_ptr);
        free(static_cast<void*>(path_ptr));
        return abs_path;
    }

    /**
     * @brief Check if path is an existing directory
     * @param path The path to check
     * @return True if the path is a directory, false otherwise
     */
    inline bool path_is_directory(const std::string& path) {
        struct stat path_stat;
        if(stat(path.c_str(), &path_stat) == -1) {
            return false;
        }
        return S_ISDIR(path_stat.st_mode);
    }

    /**
     * @brief Check if path is an existing file
     * @param path The path to check
     * @return True if the path is a file, false otherwise
     */
    inline bool path_is_file(const std::string& path) {
        struct stat path_stat;
        if(stat(path.c_str(), &path_stat) == -1) {
            return false;
        }
        return S_ISREG(path_stat.st_mode);
    }

    /**
     * @brief Get all files in a directory
     * @param path The base directory
     * @return A list with the full names of all files in the directory
     *
     * Does not recurse on subdirectories, but only return the files that are directly in the directory.
     */
    // TODO [doc] check if path exists and ensure canonical paths
    inline std::vector<std::string> get_files_in_directory(const std::string& path) {
        std::vector<std::string> files;
        struct dirent* ent = nullptr;
        struct stat st;

        // Loop through all files
        DIR* dir = opendir(path.c_str());
        while((ent = readdir(dir)) != nullptr) {
            const std::string file_name = ent->d_name;
            const std::string full_file_name = path + "/" + file_name;

            // Ignore useless or wrong paths
            if(!file_name.empty() && file_name[0] == '.') {
                continue;
            }
            if(stat(full_file_name.c_str(), &st) == -1) {
                continue;
            }

            // Ignore subdirectories
            const bool is_directory = (st.st_mode & S_IFDIR) != 0;
            if(is_directory) {
                continue;
            }

            // Add full file paths
            files.push_back(full_file_name);
        }
        closedir(dir);
        return files;
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
        struct stat st;

        path += "/";
        size_t pos = 1;
        // Loop through all subpaths of directories that possibly need to be created
        while((pos = path.find('/', pos)) != std::string::npos) {
            std::string sub_path = path.substr(0, pos);

            // Try to create the directory
            if(mkdir(sub_path.c_str(), mode) != 0 && errno != EEXIST) {
                throw std::invalid_argument("cannot create folder (" + std::string(strerror(errno)) + ")");
            }
            // Check if subpath can accessed
            if(stat(sub_path.c_str(), &st) != 0) {
                throw std::invalid_argument("cannot access path (" + std::string(strerror(errno)) + ")");
            } else if(!S_ISDIR(st.st_mode)) {
                errno = ENOTDIR;
                throw std::invalid_argument("part of path already exists as a file");
            }

            pos++;
        }
    }
}

#endif /* ALLPIX_FILE_H */
