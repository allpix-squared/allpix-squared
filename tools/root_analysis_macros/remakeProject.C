#include <TFile.h>

/**
 * Recreate project source files from data stored in data file
 */
void remakeProject(TFile* file, std::string file_name) {
    file->MakeProject(file_name.c_str(), "*", "RECREATE+");
}
