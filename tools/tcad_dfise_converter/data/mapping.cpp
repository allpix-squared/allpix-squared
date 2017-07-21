#include <fstream>
#include <math.h>
#include <sstream>
#include <stdio.h>
#include <vector>
#include "TDirectory.h"
#include "TFile.h"
#include "TH2.h"

using namespace std;

int mapping() {

    gStyle->SetOptStat(0);
    gStyle->SetNumberContours(999);

    int col, row;
    double dummy;
    double vector[6];

    int bin = 100;
    TH2D* th_map = new TH2D("Capacitances", "Capacitances", bin, 0, bin, bin, 0, bin);

    std::ifstream input_file;
    string file_line;
    string file;
    stringstream file_name;
    file_name << "example_pixel_" << bin << "x" << bin << "x" << bin << ".txt";
    file = file_name.str();
    cout << "Reading file: " << file << endl;

    input_file.open(file.c_str());
    if(input_file.is_open() != 0)
        cout << "Reading file OK " << endl;
    else {
        cout << "Bad file" << endl;
        return -1;
    }
    int line = 1;
    while(getline(input_file, file_line)) {
        int p = 0;
        stringstream mystream(file_line);
        while(mystream >> dummy) {
            vector[p] = dummy;
            p++;
        }
        if(vector[0] == (int)bin / 2)
            th_map->Fill(vector[1], vector[2], sqrt(pow(vector[3], 2) + pow(vector[4], 2) + pow(vector[5], 2)));
        line++;
    }
    th_map->Draw("colz");
    return 1;
}
