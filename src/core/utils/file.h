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

#include <climits>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include <dirent.h>
#include <ftw.h>
#include <libgen.h>
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
    inline std::string get_canonical_path(const std::string& path) {
        char* path_ptr = realpath(path.c_str(), nullptr);
        if(path_ptr == nullptr) {
            // Throw an error if the path does not exist
            throw std::invalid_argument("path " + path + " not found");
        }
        std::string abs_path(path_ptr);
        free(static_cast<void*>(path_ptr)); // NOLINT
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
            std::string file_name = ent->d_name;
            std::string full_file_name = path;
            full_file_name += "/";
            full_file_name += file_name;

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

    /**
     * @brief Recursively removes a path from the file system
     * @param path Path to the top directory to remove
     * @throws std::invalid_argument If the path cannot be removed
     * @warning This method is not thread-safe
     *
     * All the required directories are deleted recursively from the top-directory (use this with caution).
     */
    inline void remove_path(const std::string& path) {
        int status = nftw(
            path.c_str(),
            [](const char* remove_path, const struct stat*, int, struct FTW*) { return remove(remove_path); },
            64,
            FTW_DEPTH);

        if(status != 0) {
            throw std::invalid_argument("path cannot be completely deleted");
        }
    }

    /*
     * @brief Removes a single file from the file system
     * @param path Path to the file
     * @throws std::invalid_argument If the file cannot be removed
     *
     * Remove a single file at the given path. If the function returns the deletion was successful.
     */
    inline void remove_file(const std::string& path) {
        int status = unlink(path.c_str());

        if(status != 0) {
            throw std::invalid_argument("file cannot be deleted");
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
        // Add separating dot if not present:
        if(extension.at(0) != '.') {
            extension.insert(0, 1, '.');
        }

        if(path.size() > extension.size()) {
            if(std::equal(extension.rbegin(), extension.rend(), path.rbegin())) {
                return path;
            }
        }

        return path + extension;
    }

    /**
     * @brief Get the name of the file together with the extension
     * @param path Absolute path to the file
     * @return Pair of name of the file and the possible extension
     */
    inline std::pair<std::string, std::string> get_file_name_extension(const std::string& path) {
        auto char_string = std::make_unique<char[]>(path.size() + 1);
        std::copy(path.begin(), path.end(), char_string.get());
        char_string[path.size()] = '\0';

        std::string base_name = basename(char_string.get());
        auto idx = base_name.find('.');
        if(idx != std::string::npos) {
            return std::make_pair(base_name.substr(0, idx), base_name.substr(idx));
        }
        return std::make_pair(base_name, "");
    }
} // namespace allpix

#endif /* ALLPIX_FILE_H */
