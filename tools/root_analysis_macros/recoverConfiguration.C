#include <fstream>
#include <iostream>
#include <sstream>

#include <Math/Rotation3D.h>
#include <TClass.h>
#include <TDirectoryFile.h>
#include <TFile.h>
#include <TKey.h>
#include <TROOT.h>

std::stringstream listkeys(TDirectoryFile* dir) {
    std::stringstream str;
    bool name_set = false;

    std::string dirname = std::string(dir->GetName());
    // Don't add a top-level section:
    if(dirname != "config" && dirname != "detectors") {
        // Split directory name into module, detector and input name:
        std::string::size_type n = dirname.find(":");
        if(n != std::string::npos) {
            str << "[" << dirname.substr(0, n) << "]" << std::endl;
            std::string::size_type m = dirname.substr(n + 1).find("_");
            str << "name = \"" << dirname.substr(n + 1).substr(0, m) << "\"" << std::endl;
            name_set = true;
        } else {
            str << "[" << dir->GetName() << "]" << std::endl;
        }
    }

    // Loop over directory content:
    TIter next(dir->GetListOfKeys());
    TKey* entry;
    while((entry = (TKey*)next())) {
        TClass* cl = gROOT->GetClass(entry->GetClassName());
        if(cl->InheritsFrom("TDirectoryFile")) {
            // If entry is a directory, recurse:
            str << listkeys((TDirectoryFile*)entry->ReadObj()).str();
        } else if(cl->InheritsFrom("string")) {
            std::string key = entry->GetName();
            std::string value = (*(string*)entry->ReadObj());
            // If the value is empty, omit the key:
            if(value == "" || value == "\"\"") {
                continue;
            }
            // If the name has already been specified, omit:
            if(key == "name" && name_set) {
                continue;
            }

            // Otherwise add the configuration key/value pair:
            str << key << " = " << value << std::endl;
        } else if(cl->InheritsFrom("ROOT::Math::Rotation3D")) {
            // FIXME transform rotation back to the three ZYX angles
            // str << entry->GetName() << " = " << (*(ROOT::Math::Rotation3D*)entry->ReadObj()) << std::endl;
        } else if(cl->InheritsFrom("ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::"
                                   "DefaultCoordinateSystemTag>")) {
            ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>, ROOT::Math::DefaultCoordinateSystemTag> position =
                (*(ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>, ROOT::Math::DefaultCoordinateSystemTag>*)
                      entry->ReadObj());
            str << entry->GetName() << " = " << position.x() << "mm " << position.y() << "mm " << position.z() << "mm"
                << std::endl;
        } else {
            std::cout << "Could not deduce parameter type of \"" << entry->GetName() << "\"";
        }
    }

    str << std::endl;
    return str;
}

void recoverConfiguration(TString data_file, TString config_file_name, TString detector_file_name) {

    TFile* f1 = TFile::Open(data_file);
    if(!f1) {
        std::cout << "Cannot open data file \"" << data_file << "\"" << std::endl;
        return 1;
    }

    TDirectoryFile* config = nullptr;
    f1->GetObject("config", config);
    if(config != nullptr) {
        std::cout << "Found TDirectory containing the configuration keys." << std::endl;
        stringstream str = listkeys(config);
        std::ofstream config_file(config_file_name, std::ios_base::out | std::ios_base::trunc);
        if(config_file.good()) {
            config_file << str.str();
            std::cout << "Wrote configuration to: \"" << config_file_name << "\"" << std::endl;
        }
    } else {
        cout << "Could not find TDirectory with configuration." << std::endl;
    }

    TDirectoryFile* detectors = nullptr;
    f1->GetObject("detectors", detectors);
    if(detectors != nullptr) {
        cout << "Found TDirectory containing the detector configuration." << std::endl;
        stringstream str = listkeys(detectors);
        std::ofstream det_file(detector_file_name, std::ios_base::out | std::ios_base::trunc);
        if(det_file.good()) {
            det_file << str.str();
            std::cout << "Wrote detector setup to: \"" << detector_file_name << "\"" << std::endl;
        }
    } else {
        cout << "Could not find TDirectory with detector configuration." << std::endl;
    }

    return;
}
