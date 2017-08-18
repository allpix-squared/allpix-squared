#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include "TCanvas.h"
#include "TFile.h"
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
    int slice_index = 0;
    bool flag_cut = false;
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
        } else if(strcmp(argv[i], "-c") == 0 && (i + 1 < argc)) {
            slice_cut = std::atoi(argv[++i]);
            flag_cut = true;
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
        std::cout << "\t -p <plane>             plane to be ploted. xy, yz or zx (default is yz)" << std::endl;
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
        if(!flag_cut) {
            slice_cut = zdiv / 2;
        }
        x_bin_index = 0;
        y_bin_index = 1;
        slice_index = 2;
    }
    if(strcmp(plane.c_str(), "yz") == 0) {
        x_bin = ydiv;
        y_bin = zdiv;
        if(!flag_cut) {
            slice_cut = xdiv / 2;
        }
        x_bin_index = 1;
        y_bin_index = 2;
        slice_index = 0;
    }
    if(strcmp(plane.c_str(), "zx") == 0) {
        x_bin = zdiv;
        y_bin = xdiv;
        if(!flag_cut) {
            slice_cut = ydiv / 2;
        }
        x_bin_index = 2;
        y_bin_index = 0;
        slice_index = 1;
    }

    // Create and fill histogram
    auto efield_map = new TH2D("Electric Field", "Electric Field", x_bin, 0, x_bin, y_bin, 0, y_bin);
    auto exfield_map = new TH2D("Electric Field X", "Electric Field X", x_bin, 0, x_bin, y_bin, 0, y_bin);
    auto eyfield_map = new TH2D("Electric Field Y", "Electric Field Y", x_bin, 0, x_bin, y_bin, 0, y_bin);
    auto ezfield_map = new TH2D("Electric Field Z", "Electric Field Z", x_bin, 0, x_bin, y_bin, 0, y_bin);
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
            efield_map->Fill(
                vector[x_bin_index], vector[y_bin_index], sqrt(pow(vector[3], 2) + pow(vector[4], 2) + pow(vector[5], 2)));
            exfield_map->Fill(vector[x_bin_index], vector[y_bin_index], vector[3]);
            eyfield_map->Fill(vector[x_bin_index], vector[y_bin_index], vector[4]);
            ezfield_map->Fill(vector[x_bin_index], vector[y_bin_index], vector[5]);
        }
        line++;
    }
    auto* tf = new TFile("efield_plots.root", "RECREATE");
    efield_map->Write("Norm");
    exfield_map->Write("Ex");
    eyfield_map->Write("Ey");
    ezfield_map->Write("Ez");
    c1->cd();
    efield_map->Draw("colz");
    c1->SaveAs(output_file_name.c_str());
    tf->Close();
    return 0;
}
