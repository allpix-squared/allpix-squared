/**
 * @file
 * @brief Small converter for field data INIT <-> APF
 *
 * @copyright Copyright (c) 2019-2025 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/utils/log.h"
#include "core/utils/unit.h"
#include "tools/field_parser.h"
#include "tools/units.h"

using namespace allpix;

template <typename T> static void print_info(allpix::FieldData<T> field_data, size_t n, std::string units) {
    std::cout << "Header:     \"" << field_data.getHeader() << "\"" << std::endl;
    std::cout << "Field size: " << Units::display(field_data.getSize()[0], "um") << " x "
              << Units::display(field_data.getSize()[1], "um") << " x " << Units::display(field_data.getSize()[2], "um")
              << std::endl;
    std::cout << "Dimensions: " << field_data.getDimensions()[0] << " x " << field_data.getDimensions()[1] << " x "
              << field_data.getDimensions()[2] << " cells" << std::endl;
    std::cout << "Field vector with " << field_data.getData()->size() << " entries" << std::endl;

    if(n > 0) {
        std::cout << "First " << n << " entries of field data:" << '\n';
        for(size_t i = 0; i < field_data.getData()->size() && i < n; i++) {
            std::cout << Units::display(field_data.getData()->at(i), units) << " ";
        }
        std::cout << '\n';
    }
}

/**
 * @brief Main function running the application
 */
int main(int argc, const char* argv[]) {

    int return_code = 0;
    try {

        // Register the default set of units with this executable:
        register_units();

        // Add cout as the default logging stream
        Log::addStream(std::cout);

        // If no arguments are provided, print the help:
        bool print_help = false;
        if(argc == 1) {
            print_help = true;
            return_code = 1;
        }

        // Parse arguments
        std::vector<std::string> file_names;
        std::string units;
        size_t n = 0;
        for(int i = 1; i < argc; i++) {
            if(strcmp(argv[i], "-h") == 0) {
                print_help = true;
            } else if(strcmp(argv[i], "-v") == 0 && (i + 1 < argc)) {
                try {
                    LogLevel const log_level = Log::getLevelFromString(std::string(argv[++i]));
                    Log::setReportingLevel(log_level);
                } catch(std::invalid_argument& e) {
                    LOG(ERROR) << "Invalid verbosity level \"" << std::string(argv[i]) << "\", ignoring overwrite";
                }
            } else if(strcmp(argv[i], "--values") == 0 && (i + 1 < argc)) {
                n = static_cast<size_t>(std::atoi(argv[++i]));
            } else if(strcmp(argv[i], "--units") == 0 && (i + 1 < argc)) {
                units = std::string(argv[++i]);
            } else {
                file_names.emplace_back(argv[i]);
            }
        }

        // Print help if requested or no arguments given
        if(print_help) {
            std::cout << "Allpix Squared APF Field Dump Tool" << '\n';
            std::cout << '\n';
            std::cout << "Usage: apf_dump <file(s)>" << '\n';
            std::cout << '\n';
            std::cout << "Options:" << '\n';
            std::cout << "  --values <N>     also print the first N values from the file" << '\n';
            std::cout << "  --units  <units> units the field should be represented in" << '\n';
            std::cout << '\n';
            std::cout << "For more help, please see <https://cern.ch/allpix-squared>" << '\n';
            return return_code;
        }

        for(auto& file_input : file_names) {
            std::cout << "FILE:       " << file_input << '\n';
            try {
                FieldParser<double> field_parser(FieldQuantity::VECTOR);
                auto field_data = field_parser.getByFileName(file_input);
                print_info(field_data, n, units);
            } catch(std::runtime_error& e) {
                FieldParser<double> field_parser(FieldQuantity::SCALAR);
                auto field_data = field_parser.getByFileName(file_input);
                print_info(field_data, n, units);
            }
        }

    } catch(std::exception& e) {
        LOG(FATAL) << "Fatal internal error" << '\n' << e.what() << '\n' << "Cannot continue.";
        return_code = 127;
    }

    return return_code;
}
