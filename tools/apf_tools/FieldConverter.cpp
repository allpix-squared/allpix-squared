/**
 * @file
 * @brief Small converter for field data INIT <-> APF
 */

#include <algorithm>
#include <fstream>
#include <string>

#include "core/utils/log.h"
#include "tools/field_parser.h"
#include "tools/units.h"

using namespace allpix;

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
        FileType format_to = FileType::UNKNOWN;
        std::string file_input;
        std::string file_output;
        std::string units;
        bool scalar = false;
        for(int i = 1; i < argc; i++) {
            if(strcmp(argv[i], "-h") == 0) {
                print_help = true;
            } else if(strcmp(argv[i], "-v") == 0 && (i + 1 < argc)) {
                try {
                    LogLevel log_level = Log::getLevelFromString(std::string(argv[++i]));
                    Log::setReportingLevel(log_level);
                } catch(std::invalid_argument& e) {
                    LOG(ERROR) << "Invalid verbosity level \"" << std::string(argv[i]) << "\", ignoring overwrite";
                }
            } else if(strcmp(argv[i], "--to") == 0 && (i + 1 < argc)) {
                std::string format = std::string(argv[++i]);
                std::transform(format.begin(), format.end(), format.begin(), ::tolower);
                format_to = (format == "init" ? FileType::INIT : format == "apf" ? FileType::APF : FileType::UNKNOWN);
            } else if(strcmp(argv[i], "--input") == 0 && (i + 1 < argc)) {
                file_input = std::string(argv[++i]);
            } else if(strcmp(argv[i], "--output") == 0 && (i + 1 < argc)) {
                file_output = std::string(argv[++i]);
            } else if(strcmp(argv[i], "--units") == 0 && (i + 1 < argc)) {
                units = std::string(argv[++i]);
            } else if(strcmp(argv[i], "--scalar") == 0) {
                scalar = true;
            } else {
                LOG(ERROR) << "Unrecognized command line argument \"" << argv[i] << "\"";
                print_help = true;
                return_code = 1;
            }
        }

        // Print help if requested or no arguments given
        if(print_help) {
            std::cout << "Allpix Squared Field Converter Tool" << std::endl;
            std::cout << std::endl;
            std::cout << "Usage: field_converter <parameters>" << std::endl;
            std::cout << std::endl;
            std::cout << "Parameters (all mandatory):" << std::endl;
            std::cout << "  --to <format>    file format of the output file" << std::endl;
            std::cout << "  --input <file>   input field file" << std::endl;
            std::cout << "  --output <file>  output field file" << std::endl;
            std::cout << "  --units <units>  units the field is provided in" << std::endl << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --scalar         Convert scalar field. Default is vector field" << std::endl;
            std::cout << std::endl;
            std::cout << "For more help, please see <https://cern.ch/allpix-squared>" << std::endl;
            return return_code;
        }

        FieldQuantity quantity = (scalar ? FieldQuantity::SCALAR : FieldQuantity::VECTOR);

        FieldParser<double> field_parser(quantity);
        LOG(STATUS) << "Reading input file from " << file_input;
        auto field_data = field_parser.getByFileName(file_input, units);
        FieldWriter<double> field_writer(quantity);
        LOG(STATUS) << "Writing output file to " << file_output;
        field_writer.writeFile(field_data, file_output, format_to, (format_to == FileType::INIT ? units : ""));
    } catch(std::exception& e) {
        LOG(FATAL) << "Fatal internal error" << std::endl << e.what() << std::endl << "Cannot continue.";
        return_code = 127;
    }

    return return_code;
}
