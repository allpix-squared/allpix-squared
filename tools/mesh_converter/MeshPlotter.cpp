/**
 * @file
 *
 * @copyright Copyright (c) 2017-2024 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 * SPDX-License-Identifier: MIT
 */

#include <cmath>
#include <csignal>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include "TCanvas.h"
#include "TFile.h"
#include "TH2.h"
#include "TStyle.h"

#include "core/utils/log.h"
#include "core/utils/unit.h"
#include "tools/field_parser.h"
#include "tools/units.h"

using namespace allpix;

void interrupt_handler(int);

/**
 * @brief Handle termination request (CTRL+C)
 */
void interrupt_handler(int) {
    LOG(STATUS) << "Interrupted! Aborting conversion...";
    allpix::Log::finish();
    std::exit(0);
}

int main(int argc, char** argv) {

    try {

        // Register the default set of units with this executable:
        allpix::register_units();

        // Set ROOT params
        gStyle->SetOptStat(0);
        gStyle->SetNumberContours(999);

        bool print_help = false;
        int return_code = 0;
        if(argc == 1) {
            print_help = true;
            return_code = 1;
        }

        // Add stream and set default logging level
        allpix::Log::addStream(std::cout);

        // Install abort handler (CTRL+\) and interrupt handler (CTRL+C)
        std::signal(SIGQUIT, interrupt_handler);
        std::signal(SIGINT, interrupt_handler);

        // Read parameters
        std::string file_name;
        std::string output_file_name;
        std::string output_name_log;
        std::string plane = "yz";
        std::string units;
        bool scalar_field = false;
        bool flag_cut = false;
        size_t slice_cut = 0;
        bool log_scale = false;
        allpix::LogLevel log_level = allpix::LogLevel::INFO;

        for(int i = 1; i < argc; i++) {
            if(strcmp(argv[i], "-h") == 0) {
                print_help = true;
            } else if(strcmp(argv[i], "-v") == 0 && (i + 1 < argc)) {
                try {
                    log_level = allpix::Log::getLevelFromString(std::string(argv[++i]));
                } catch(std::invalid_argument& e) {
                    LOG(ERROR) << "Invalid verbosity level \"" << std::string(argv[i]) << "\", ignoring overwrite";
                    return_code = 1;
                }
            } else if(strcmp(argv[i], "-f") == 0 && (i + 1 < argc)) {
                file_name = std::string(argv[++i]);
            } else if(strcmp(argv[i], "-o") == 0 && (i + 1 < argc)) {
                output_file_name = std::string(argv[++i]);
            } else if(strcmp(argv[i], "-p") == 0 && (i + 1 < argc)) {
                plane = std::string(argv[++i]);
            } else if(strcmp(argv[i], "-u") == 0 && (i + 1 < argc)) {
                units = std::string(argv[++i]);
            } else if(strcmp(argv[i], "-s") == 0) {
                scalar_field = true;
            } else if(strcmp(argv[i], "-c") == 0 && (i + 1 < argc)) {
                slice_cut = static_cast<size_t>(std::atoi(argv[++i]));
                flag_cut = true;
            } else if(strcmp(argv[i], "-l") == 0) {
                log_scale = true;
            } else {
                std::cout << "Unrecognized command line argument or missing value\"" << argv[i] << std::endl;
                print_help = true;
                return_code = 1;
            }
        }

        // Set log level:
        allpix::Log::setReportingLevel(log_level);

        if(file_name.empty()) {
            print_help = true;
            return_code = 1;
        }

        if(print_help) {
            std::cerr << "Usage: mesh_plotter -f <file_name> [<options>]" << std::endl;
            std::cout << "Required parameters:" << std::endl;
            std::cout << "\t -f <file_name>         name of the interpolated file in INIT or APF format" << std::endl;
            std::cout << "Optional parameters:" << std::endl;
            std::cout << "\t -c <cut>               projection height index (default is mesh_pitch / 2)" << std::endl;
            std::cout << "\t -h                     display this help text" << std::endl;
            std::cout << "\t -l                     plot with logarithmic scale if set" << std::endl;
            std::cout << "\t -o <output_file_name>  name of the file to output (default is efield.png)" << std::endl;
            std::cout << "\t -p <plane>             plane to be plotted. xy, yz or zx (default is yz)" << std::endl;
            std::cout << "\t -u <units>             units to interpret the field data in" << std::endl;
            std::cout << "\t -s                     parsed observable is a scalar field" << std::endl;
            std::cout << "\t -v <level>        verbosity level (default reporiting level is INFO)" << std::endl;

            allpix::Log::finish();
            return return_code;
        }

        // Read file
        LOG(STATUS) << "Welcome to the Mesh Plotter Tool of Allpix^2 " << ALLPIX_PROJECT_VERSION;
        LOG(STATUS) << "Reading file: " << file_name;

        size_t firstindex = file_name.find_last_of('_');
        size_t lastindex = file_name.find_last_of('.');
        std::string observable = file_name.substr(firstindex + 1, lastindex - (firstindex + 1));

        // FIXME this should be done in a more elegant way
        FieldQuantity quantity = (scalar_field ? FieldQuantity::SCALAR : FieldQuantity::VECTOR);

        FieldParser<double> field_parser(quantity);
        auto field_data = field_parser.getByFileName(file_name, units);
        size_t xdiv = field_data.getDimensions()[0], ydiv = field_data.getDimensions()[1],
               zdiv = field_data.getDimensions()[2];

        LOG(STATUS) << "Number of divisions in x/y/z: " << xdiv << "/" << ydiv << "/" << zdiv;

        // Find plotting indices
        int x_bin = 0;
        int y_bin = 0;
        size_t start_x = 0, start_y = 0, start_z = 0;
        size_t stop_x = xdiv, stop_y = ydiv, stop_z = zdiv;
        std::string axis_titles;
        if(plane == "xy") {
            if(!flag_cut) {
                slice_cut = (zdiv - 1) / 2;
            }

            // z is the slice:
            start_z = slice_cut;
            stop_z = start_z + 1;

            // scale the plot axes:
            x_bin = static_cast<int>(xdiv);
            y_bin = static_cast<int>(ydiv);
            axis_titles = "x [bins];y [bins]";
        } else if(plane == "yz") {
            if(!flag_cut) {
                slice_cut = (xdiv - 1) / 2;
            }

            // x is the slice:
            start_x = slice_cut;
            stop_x = start_x + 1;

            x_bin = static_cast<int>(ydiv);
            y_bin = static_cast<int>(zdiv);
            axis_titles = "y [bins];z [bins]";
        } else {
            if(!flag_cut) {
                slice_cut = (ydiv - 1) / 2;
            }

            // y is the slice:
            start_y = slice_cut;
            stop_y = start_y + 1;

            x_bin = static_cast<int>(zdiv);
            y_bin = static_cast<int>(xdiv);
            axis_titles = "z [bins];x [bins]";
        }

        // Create and fill histogram
        auto* efield_map = new TH2D(Form("%s", observable.c_str()),
                                    Form("%s;%s", observable.c_str(), axis_titles.c_str()),
                                    x_bin,
                                    0,
                                    x_bin,
                                    y_bin,
                                    0,
                                    y_bin);
        auto* exfield_map = new TH2D(Form("%s X component", observable.c_str()),
                                     Form("%s X component;%s", observable.c_str(), axis_titles.c_str()),
                                     x_bin,
                                     0,
                                     x_bin,
                                     y_bin,
                                     0,
                                     y_bin);
        auto* eyfield_map = new TH2D(Form("%s Y component", observable.c_str()),
                                     Form("%s Y component;%s", observable.c_str(), axis_titles.c_str()),
                                     x_bin,
                                     0,
                                     x_bin,
                                     y_bin,
                                     0,
                                     y_bin);
        auto* ezfield_map = new TH2D(Form("%s Z component", observable.c_str()),
                                     Form("%s Z component;%s", observable.c_str(), axis_titles.c_str()),
                                     x_bin,
                                     0,
                                     x_bin,
                                     y_bin,
                                     0,
                                     y_bin);

        auto* c1 = new TCanvas();

        if(log_scale) {
            c1->SetLogz();
            output_name_log = "_log";
        }

        int plot_x = 0, plot_y = 0;
        auto data = field_data.getData();
        for(size_t x = start_x; x < stop_x; x++) {
            for(size_t y = start_y; y < stop_y; y++) {
                for(size_t z = start_z; z < stop_z; z++) {
                    // Select the indices for plotting:
                    if(plane == "xy") {
                        plot_x = static_cast<int>(x);
                        plot_y = static_cast<int>(y);
                    } else if(plane == "yz") {
                        plot_x = static_cast<int>(y);
                        plot_y = static_cast<int>(z);
                    } else {
                        plot_x = static_cast<int>(z);
                        plot_y = static_cast<int>(x);
                    }

                    if(quantity == FieldQuantity::VECTOR) {
                        // Fill field maps for the individual vector components as well as the magnitude
                        auto base = x * ydiv * zdiv * 3 + y * zdiv * 3 + z * 3;
                        efield_map->Fill(
                            plot_x,
                            plot_y,
                            sqrt(pow(data->at(base + 0), 2) + pow(data->at(base + 1), 2) + pow(data->at(base + 2), 2)));
                        exfield_map->Fill(plot_x, plot_y, static_cast<double>(Units::convert(data->at(base + 0), units)));
                        eyfield_map->Fill(plot_x, plot_y, static_cast<double>(Units::convert(data->at(base + 1), units)));
                        ezfield_map->Fill(plot_x, plot_y, static_cast<double>(Units::convert(data->at(base + 2), units)));

                    } else {
                        // Fill one map with the scalar quantity
                        efield_map->Fill(
                            plot_x,
                            plot_y,
                            static_cast<double>(Units::convert(data->at(x * ydiv * zdiv + y * zdiv + z), units)));
                    }
                }
            }
        }

        if(output_file_name.empty()) {
            output_file_name = file_name.substr(0, lastindex);
            output_file_name = output_file_name + "_" + plane + "_" + std::to_string(slice_cut) + output_name_log + ".png";
        }

        std::string root_file_name = file_name.substr(0, lastindex);
        root_file_name = root_file_name + "_Interpolation_plots_" + plane + "_" + std::to_string(slice_cut) + ".root";
        auto* tf = new TFile(root_file_name.c_str(), "RECREATE");

        if(quantity == FieldQuantity::VECTOR) {
            exfield_map->Write(Form("%s X component", observable.c_str()));
            eyfield_map->Write(Form("%s Y component", observable.c_str()));
            ezfield_map->Write(Form("%s Z component", observable.c_str()));
        }

        efield_map->Write(Form("%s Norm", observable.c_str()));
        c1->cd();
        efield_map->Draw("colz");
        c1->SaveAs(output_file_name.c_str());
        tf->Close();

        allpix::Log::finish();
        return 0;
    } catch(std::exception& e) {
        LOG(FATAL) << "Failed to plot mesh:\n" << e.what();
        allpix::Log::finish();
        return 1;
    }
}
