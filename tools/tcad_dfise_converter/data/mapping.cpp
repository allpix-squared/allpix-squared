#include <fstream>
#include <iostream>
#include <math.h>
#include <sstream>
#include <stdio.h>
#include <vector>
#include "TCanvas.h"
#include "TH2.h"
#include "TStyle.h"

int main(int argc, char** argv) {
    gStyle->SetOptStat(0);
    gStyle->SetNumberContours(999);

    bool print_help = false;
    int return_code = 0;
    if(argc == 1) {
        print_help = true;
        return_code = 1;
    }

    std::string file_prefix = "example_pixel";
    std::string plane = "yz";
    int slice_index = 0;
    int slice_cut = 50;
    int xdiv = 100;
    int ydiv = 100;
    int zdiv = 100;

    std::stringstream file_name;

    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-h") == 0) {
            print_help = true;
        } else if(strcmp(argv[i], "-f") == 0 && (i + 1 < argc)) {
            file_prefix = std::string(argv[++i]);
        } else if(strcmp(argv[i], "-p") == 0 && (i + 1 < argc)) {
            plane = std::string(argv[++i]);
        } else if(strcmp(argv[i], "-c") == 0 && (i + 1 < argc)) {
            slice_cut = static_cast<int>(strtod(argv[++i], nullptr));
        } else if(strcmp(argv[i], "-x") == 0 && (i + 1 < argc)) {
            xdiv = static_cast<int>(strtod(argv[++i], nullptr));
        } else if(strcmp(argv[i], "-y") == 0 && (i + 1 < argc)) {
            ydiv = static_cast<int>(strtod(argv[++i], nullptr));
        } else if(strcmp(argv[i], "-z") == 0 && (i + 1 < argc)) {
            zdiv = static_cast<int>(strtod(argv[++i], nullptr));
        } else {
            std::cout << "Unrecognized command line argument \"" << argv[i] << std::endl;
        }
    }

    if(print_help) {
        std::cerr << "Usage: ./tcad_dfise_reader -f <data_file_prefix> [<options>]" << std::endl;
        std::cout << "\t -f <file_prefix>       DF-ISE files prefix" << std::endl;
        std::cout << "\t -R <region>            region name to be meshed" << std::endl;
        std::cout << "\t -r <radius>            initial node neighbors search radius" << std::endl;
        std::cout << "\t -r <radius_step>       radius step if no neighbor is found" << std::endl;
        std::cout << "\t -m <max_radius>        maximum search radius" << std::endl;
        std::cout << "\t -x <mesh x_pitch>      new regular mesh X pitch" << std::endl;
        std::cout << "\t -y <mesh_y_pitch>      new regular mesh Y pitch" << std::endl;
        std::cout << "\t -z <mesh_z_pitch>      new regular mesh Z pitch" << std::endl;
        std::cout << "\t -l <file>              file to log to besides standard output" << std::endl;
        std::cout << "\t -v <level>             verbosity level overwrites global log level,\n"
                  << "\t                        but not the per-module configuration." << std::endl;
        return return_code;
    }

    file_name << file_prefix << "_" << xdiv << "x" << ydiv << "x" << zdiv << ".txt";
    std::cout << "Reading file: " << file_name.str();

    std::ifstream input_file;
    input_file.open((file_name.str()).c_str());
    if(input_file.is_open() != 0)
        std::cout << "OK" << std::endl;
    else {
        std::cout << "FAILD" << std::endl;
        return -1;
    }

    int x_bin = 0;
    int y_bin = 0;
    int x_bin_index = 0;
    int y_bin_index = 0;
    if(strcmp(plane.c_str(), "xy") == 0) {
        x_bin = xdiv;
        y_bin = ydiv;
        x_bin_index = 0;
        y_bin_index = 1;
        slice_index = 2;
    }
    if(strcmp(plane.c_str(), "yz") == 0) {
        x_bin = ydiv;
        y_bin = zdiv;
        x_bin_index = 1;
        y_bin_index = 2;
        slice_index = 0;
    }
    if(strcmp(plane.c_str(), "zx") == 0) {
        x_bin = zdiv;
        y_bin = xdiv;
        x_bin_index = 2;
        y_bin_index = 0;
        slice_index = 1;
    }

    TH2D* efield_map = new TH2D("Electric Field", "Electric Field", x_bin, 0, x_bin, y_bin, 0, y_bin);
    TCanvas* c1 = new TCanvas();

    double dummy;
    double vector[6];
    int line = 1;
    std::string file_line;
    while(getline(input_file, file_line)) {
        int p = 0;
        if(line < 6)
            continue;
        std::stringstream mystream(file_line);
        while(mystream >> dummy) {
            vector[p] = dummy;
            p++;
        }
        if(vector[slice_index] == slice_cut)
            efield_map->Fill(
                vector[x_bin_index], vector[y_bin_index], sqrt(pow(vector[3], 2) + pow(vector[4], 2) + pow(vector[5], 2)));
        line++;
    }
    c1->cd();
    efield_map->Draw("colz");
    c1->SaveAs("efield_map.png");
    return 1;
}
