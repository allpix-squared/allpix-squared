#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include "TCanvas.h"
#include "TH2.h"
#include "TStyle.h"

int main(int argc, char** argv) {
    // Set ROOT params
    gStyle->SetOptStat(0);
    gStyle->SetNumberContours(999);

    bool print_help = false;
    int return_code = 0;
    if(argc == 1) {
        print_help = true;
        return_code = 1;
    }

    // Read parameters
    std::string file_name;
    std::string output_file_name = "efield.png";
    std::string plane = "yz";
    std::string data = "n";
    int slice_index = 0;
    int slice_cut = 0;
    int xdiv = 100;
    int ydiv = 100;
    int zdiv = 100;
    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-h") == 0) {
            print_help = true;
        } else if(strcmp(argv[i], "-f") == 0 && (i + 1 < argc)) {
            file_name = std::string(argv[++i]);
        } else if(strcmp(argv[i], "-o") == 0 && (i + 1 < argc)) {
            output_file_name = std::string(argv[++i]);
        } else if(strcmp(argv[i], "-p") == 0 && (i + 1 < argc)) {
            plane = std::string(argv[++i]);
        } else if(strcmp(argv[i], "-d") == 0 && (i + 1 < argc)) {
            data = std::string(argv[++i]);
        } else if(strcmp(argv[i], "-c") == 0 && (i + 1 < argc)) {
            slice_cut = std::atoi(argv[++i]);
        } else if(strcmp(argv[i], "-x") == 0 && (i + 1 < argc)) {
            xdiv = std::atoi(argv[++i]);
        } else if(strcmp(argv[i], "-y") == 0 && (i + 1 < argc)) {
            ydiv = std::atoi(argv[++i]);
        } else if(strcmp(argv[i], "-z") == 0 && (i + 1 < argc)) {
            zdiv = std::atoi(argv[++i]);
        } else {
            std::cout << "Unrecognized command line argument or missing value\"" << argv[i] << std::endl;
            print_help = true;
            return_code = 1;
        }
    }

    if(file_name.empty()) {
        print_help = true;
        return_code = 1;
    }

    if(print_help) {
        std::cerr << "Usage: ./tcad_dfise_reader -f <file_name> [<options>]" << std::endl;
        std::cout << "\t -f <file_name>         init file name" << std::endl;
        std::cout << "\t -o <output_file_name>  name of the file to output (default is efield.png)" << std::endl;
        std::cout << "\t -p <plane>             plane to be ploted. xy, yz or zx (default is xy)" << std::endl;
        std::cout << "\t -d <data>              data to be read (default is n). See README file." << std::endl;
        std::cout << "\t -c <cut>               projection height index (default is mesh_pitch / 2)" << std::endl;
        std::cout << "\t -x <mesh x_pitch>      plot regular mesh X binning (default is 100)" << std::endl;
        std::cout << "\t -y <mesh_y_pitch>      plot regular mesh Y binning (default is 100)" << std::endl;
        std::cout << "\t -z <mesh_z_pitch>      plot regular mesh Z binning (default is 100)" << std::endl;
        return return_code;
    }

    // Read file
    std::cout << "Reading file: " << file_name;

    std::ifstream input_file;
    input_file.open(file_name);
    if(input_file.is_open() != false) {
        std::cout << "  OK" << std::endl;
    } else {
        std::cout << "  FAILED" << std::endl;
        return 1;
    }

    // Find plotting indices
    int x_bin = 0;
    int y_bin = 0;
    int x_bin_index = 0;
    int y_bin_index = 0;
    if(strcmp(plane.c_str(), "xy") == 0) {
        x_bin = xdiv;
        y_bin = ydiv;
        slice_cut = zdiv / 2;
        x_bin_index = 0;
        y_bin_index = 1;
        slice_index = 2;
    }
    if(strcmp(plane.c_str(), "yz") == 0) {
        x_bin = ydiv;
        y_bin = zdiv;
        slice_cut = xdiv / 2;
        x_bin_index = 1;
        y_bin_index = 2;
        slice_index = 0;
    }
    if(strcmp(plane.c_str(), "zx") == 0) {
        x_bin = zdiv;
        y_bin = xdiv;
        slice_cut = ydiv / 2;
        x_bin_index = 2;
        y_bin_index = 0;
        slice_index = 1;
    }

    size_t data_index;
    if(strcmp(data.c_str(), "x") == 0) {
        data_index = 3;
    }
    if(strcmp(data.c_str(), "y") == 0) {
        data_index = 4;
    }
    if(strcmp(data.c_str(), "z") == 0) {
        data_index = 5;
    }

    // Create and fill histogram
    auto efield_map = new TH2D("Electric Field", "Electric Field", x_bin, 0, x_bin, y_bin, 0, y_bin);
    auto c1 = new TCanvas();

    double dummy;
    double vector[6];
    int line = 1;
    std::string file_line;
    while(getline(input_file, file_line)) {
        int p = 0;
        if(line < 6) {
            line++;
            continue;
        }
        std::stringstream mystream(file_line);
        while(mystream >> dummy) {
            vector[p] = dummy;
            p++;
        }
        if(vector[slice_index] == slice_cut) {
            if(strcmp(data.c_str(), "n") == 0) {
                efield_map->Fill(vector[x_bin_index],
                                 vector[y_bin_index],
                                 sqrt(pow(vector[3], 2) + pow(vector[4], 2) + pow(vector[5], 2)));
            } else {
                efield_map->Fill(vector[x_bin_index], vector[y_bin_index], vector[data_index]);
            }
        }
        line++;
    }
    c1->cd();
    efield_map->Draw("colz");
    c1->SaveAs(output_file_name.c_str());
    return 0;
}
