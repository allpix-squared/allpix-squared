// SPDX-FileCopyrightText: 2017-2024 CERN and the Allpix Squared authors
// SPDX-License-Identifier: MIT

#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

#include <Math/Rotation3D.h>
#include <Math/RotationZYX.h>
#include <TClass.h>
#include <TDirectoryFile.h>
#include <TFile.h>
#include <TKey.h>
#include <TROOT.h>

std::string detectors_file;

std::stringstream listkeys(TDirectoryFile* dir, bool name_set = false) {
    std::stringstream str;

    // Loop over directory content:
    TIter next(dir->GetListOfKeys());
    TKey* entry;
    while((entry = (TKey*)next())) {
        TClass* cl = gROOT->GetClass(entry->GetClassName());
        std::string key = std::string(entry->GetName());

        if(cl->InheritsFrom("TDirectoryFile")) {
            bool named_module = false;

            // Split directory name into module, detector and input name:
            std::string::size_type n = key.find(":");
            if(n != std::string::npos) {
                str << "[" << key.substr(0, n) << "]" << std::endl;
                std::string::size_type m = key.substr(n + 1).find("_");
                str << "name = \"" << key.substr(n + 1).substr(0, m) << "\"" << std::endl;
                named_module = true;
            } else {
                str << "[" << key << "]" << std::endl;
            }

            // Recurse:
            str << listkeys(reinterpret_cast<TDirectoryFile*>(entry->ReadObj()), named_module).str();
        } else if(cl->InheritsFrom("string")) {
            std::string value = (*reinterpret_cast<std::string*>(entry->ReadObj()));
            // Omit empty "input" and "output" keys:
            if((key == "input" || key == "output") && (value == "" || value == "\"\"")) {
                continue;
            }
            // If the name has already been specified, omit:
            if((key == "name" || key == "type") && name_set) {
                continue;
            }

            // If this is the detectors file, cache its name and add the current directory as model path:
            if(key == "detectors_file") {
                detectors_file = value;
                str << "model_paths = \".\"" << std::endl;
            }

            // Otherwise add the configuration key/value pair:
            str << key << " = " << value << std::endl;
        } else if(cl->InheritsFrom("ROOT::Math::Rotation3D")) {
            // Transform rotation back to the three ZYX angles
            ROOT::Math::Rotation3D rot = (*reinterpret_cast<ROOT::Math::Rotation3D*>(entry->ReadObj())).Inverse();
            ROOT::Math::RotationZYX angles = ROOT::Math::RotationZYX(rot);
            str << "orientation_type = \"xyz\"" << std::endl;
            str << key << " = " << (-angles.Psi()) << "rad " << (-angles.Theta()) << "rad " << (-angles.Phi()) << "rad"
                << std::endl;
        } else if(cl->InheritsFrom("ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::"
                                   "DefaultCoordinateSystemTag>")) {
            ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>, ROOT::Math::DefaultCoordinateSystemTag> position =
                (*reinterpret_cast<
                    ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>, ROOT::Math::DefaultCoordinateSystemTag>*>(
                    entry->ReadObj()));
            str << key << " = " << position.x() << "mm " << position.y() << "mm " << position.z() << "mm" << std::endl;
        } else {
            std::cout << "Could not deduce parameter type of \"" << key << "\": " << entry->GetClassName() << std::endl;
        }
    }

    str << std::endl;
    return str;
}

void recoverConfiguration(std::string data_file, std::string config_file_name) {

    TFile* f1 = TFile::Open(data_file.c_str());
    if(!f1) {
        std::cout << "Cannot open data file \"" << data_file << "\"" << std::endl;
        return;
    }

    TDirectoryFile* config = nullptr;
    f1->GetObject("config", config);
    if(config != nullptr) {
        std::cout << "Found TDirectory containing the configuration keys." << std::endl;
        std::stringstream str = listkeys(config);
        std::ofstream config_file(config_file_name, std::ios_base::out | std::ios_base::trunc);
        if(config_file.good()) {
            config_file << str.str();
            std::cout << "Wrote configuration to: \"" << config_file_name << "\"" << std::endl;
        }
    } else {
        std::cout << "Could not find TDirectory with configuration." << std::endl;
    }

    // Use path of the configuration file:
    std::size_t found = config_file_name.find_last_of("/\\");
    std::string path = (found != std::string::npos ? config_file_name.substr(0, found) : ".");

    TDirectoryFile* detectors = nullptr;
    f1->GetObject("detectors", detectors);
    if(detectors != nullptr) {
        std::cout << "Found TDirectory containing the detector configuration." << std::endl;
        std::stringstream str = listkeys(detectors);

        if(detectors_file.empty()) {
            // Default to detectors file name:
            detectors_file = "detectors.conf";
            std::cout << "Using default name for detectors file - you might need to adjust the parameter." << std::endl;
        } else {
            // Clean string from quotation marks:
            if(detectors_file.front() == '"') {
                detectors_file.erase(0, 1);                      // erase the first character
                detectors_file.erase(detectors_file.size() - 1); // erase the last character
            }
            std::cout << "Using stored detectors file name \"" << detectors_file << "\"" << std::endl;
        }

        detectors_file = path + "/" + detectors_file;
        std::ofstream det_file(detectors_file, std::ios_base::out | std::ios_base::trunc);
        if(det_file.good()) {
            det_file << str.str();
            std::cout << "Wrote detector setup to: \"" << detectors_file << "\"" << std::endl;
        } else {
            std::cout << "Problem writing detector file \"" << detectors_file << "\"" << std::endl;
        }
    } else {
        std::cout << "Could not find TDirectory with detector configuration." << std::endl;
    }

    TDirectoryFile* models = nullptr;
    f1->GetObject("models", models);
    if(models != nullptr) {
        std::cout << "Found TDirectory containing the model configurations." << std::endl;

        TIter next(models->GetListOfKeys());
        TKey* entry;
        while((entry = (TKey*)next())) {
            TClass* cl = gROOT->GetClass(entry->GetClassName());
            std::string key = std::string(entry->GetName());

            if(cl->InheritsFrom("TDirectoryFile")) {
                std::string model_parameters = listkeys(reinterpret_cast<TDirectoryFile*>(entry->ReadObj())).str();
                std::string model_file = path + "/" + key + ".conf";
                std::ofstream mod_file(model_file, std::ios_base::out | std::ios_base::trunc);
                if(mod_file.good()) {
                    // Remove numbering from support layer headers
                    std::regex support_header("\\[support_[0-9]+\\]");
                    mod_file << std::regex_replace(model_parameters, support_header, "[support]");
                    std::cout << "Wrote model \"" << key << "\" to: \"" << model_file << "\"" << std::endl;
                } else {
                    std::cout << "Problem writing model \"" << key << "\" to \"" << model_file << "\"" << std::endl;
                }
            } else {
                std::cout << "Directory entry not a detector model." << std::endl;
            }
        }
    } else {
        std::cout << "Could not find TDirectory with detector models." << std::endl;
    }

    return;
}
