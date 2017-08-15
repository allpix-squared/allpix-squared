#include <TFile.h>

/**
 * Recreate project source files from data stored in data file
 */
void remakeProject(TFile* file, std::string dir_name) {
    file->MakeProject(dir_name.c_str(), "*", "RECREATE+");
}
