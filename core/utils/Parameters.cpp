// Global includes
#include <fstream>
#include <sstream>
#include <unistd.h>

// Local includes
#include "Parameters.hpp"

Parameters::Parameters(){
  
  histogramFile = "outputHistograms.root";
  conditionsFile = "cond.dat";
  dutMaskFile = "defaultMask.dat";
	inputDirectory = "";
  nEvents = 0;
  align = false;
  produceMask = false;
  currentTime = 0.; // seconds
  eventLength = 0.000; //seconds (0.1 ms)
}

void Parameters::help()
{
  cout << "********************************************************************" << endl;
  cout << "Typical 'tbAnalysis' executions are:" << endl;
  cout << " => bin/tbAnalysis -d directory" << endl;
  cout << endl;
}

// Sort function for detectors from low to high z
map<string,DetectorParameters*> globalDetector;
bool sortByZ(string detector1, string detector2){
  return (globalDetector[detector1]->displacementZ() < globalDetector[detector2]->displacementZ());
}

// Read command line options and set appropriate variables
void Parameters::readCommandLineOptions(int argc, char *argv[]){
  
  // If there are no input parameters then display the help function
  if(argc == 1){
    help();
    return;
  }
  
  cout<<endl;
  cout<<"===================| Reading Parameters  |===================="<<endl<<endl;
  int option; char temp[256];
  while ( (option = getopt(argc, argv, "gema:d:c:n:h:t:o:f:t:p:s:")) != -1) {
    switch (option) {
      case 'a':
        align = true;
        sscanf( optarg, "%d", &alignmentMethod);
        cout<<"Alignment flagged to be run. Running method "<<alignmentMethod<<endl;
        break;
      case 'e':
        eventDisplay = true;
        cout<<"Event display flagged to be run"<<endl;
        break;
      case 'g':
        gui = true;
        cout<<"GUI flagged to be run"<<endl;
        break;
      case 'm':
        produceMask = true;
        cout<<"Will update masked pixel files"<<endl;
        break;
      case 'd':
        sscanf( optarg, "%s", temp);
        inputDirectory = (string)temp;
        cout<<"Taking data from: "<<inputDirectory<<endl;
        break;
      case 'c':
        sscanf( optarg, "%s", temp);
        conditionsFile = (string)temp;
        cout<<"Picking up conditions file: "<<conditionsFile<<endl;
        break;
      case 'h':
        sscanf( optarg, "%s", temp);
        histogramFile = (string)temp;
        cout<<"Writing histograms to: "<<histogramFile<<endl;
        break;
      case 'n':
        sscanf( optarg, "%d", &nEvents);
        cout<<"Running over "<<nEvents<<" events"<<endl;
        break;
      case 'o':
        sscanf( optarg, "%lf", &currentTime);
        cout<<"Starting at time: "<<currentTime<<" s"<<endl;
        break;
      case 't':
        sscanf( optarg, "%s", temp);
        inputTupleFile = (string)temp;
        cout<<"Reading tuples from: "<<inputTupleFile<<endl;
        break;
      case 'f':
        sscanf( optarg, "%s", temp);
        outputTupleFile = (string)temp;
        cout<<"Writing output tuples to: "<<outputTupleFile<<endl;
        break;
      case 'p':
        sscanf( optarg, "%lf", &eventLength);
        cout<<"Running with an event length of: "<<eventLength<<" s"<<endl;
        break;
      case 's':
        sscanf( optarg, "%s", temp);
        dutMaskFile = (string)temp;
        cout<<"Taking dut mask from: "<<dutMaskFile<<endl;

    }
  }
  cout<<endl;
}

bool Parameters::writeConditions(){
  
  // Open the conditions file to write detector information
  ofstream conditions;
  conditions.open("alignmentOutput.dat");

  // Write the file header
  conditions << std::left << setw(12) << "DetectorID" << setw(14) << "DetectorType" << setw(10) << "nPixelsX" << setw(10) << "nPixelsY" << setw(8) << "PitchX" << setw(8) << "PitchY" << setw(11) << "X" << setw(11) << "Y" << setw(11) << "Z" << setw(11) << "Rx" << setw(11) << "Ry" << setw(11) << "Rz" << setw(14) << "tOffset"<<endl;
  
  // Loop over all detectors
  for(int det = 0; det<this->nDetectors; det++){
    string detectorID = this->detectors[det];
    DetectorParameters* detectorParameters = this->detector[detectorID];
    // Write information to file
    conditions << std::left << setw(12) << detectorID << setw(14) << detectorParameters->type() << setw(10) << detectorParameters->nPixelsX() << setw(10) << detectorParameters->nPixelsY() << setw(8) << 1000.*detectorParameters->pitchX() << setw(8) << 1000.*detectorParameters->pitchY() << setw(11) << std::fixed << detectorParameters->displacementX() << setw(11) << detectorParameters->displacementY() << setw(11) << detectorParameters->displacementZ() << setw(11) << detectorParameters->rotationX() << setw(11) << detectorParameters->rotationY() << setw(11) << detectorParameters->rotationZ() << setw(14) << std::setprecision(10) << detectorParameters->timingOffset() << resetiosflags( ios::fixed ) << std::setprecision(6) << endl;

  }
  
  // Close the file
  conditions.close();
  
  return true;
}

bool Parameters::readConditions(){
 
  // Open the conditions file to read detector information
  ifstream conditions;
  conditions.open(conditionsFile.c_str());
  string line;
  
  cout<<"----------------------------------------------------------------------------------------------------------------------------------------------------------------"<<endl;
  // Loop over file
  while(getline(conditions,line)){
    
    // Ignore header
    if( line.find("DetectorID") != string::npos ){
      cout<<"Device parameters: "<<line<<endl;
      continue;
    }
    
    // Make default values for detector parameters
    string detectorID(""), detectorType(""); double nPixelsX(0), nPixelsY(0); double pitchX(0), pitchY(0), x(0), y(0), z(0), Rx(0), Ry(0), Rz(0);
    double timingOffset(0.);
    
    // Grab the line and parse the data into the relevant variables
    istringstream detectorInfo(line);
    detectorInfo >> detectorID >> detectorType >> nPixelsX >> nPixelsY >> pitchX >> pitchY >> x >> y >> z >> Rx >> Ry >> Rz >> timingOffset;
    if(detectorID == "") continue;

    // Save the detector parameters in memory
    DetectorParameters* detectorSummary = new DetectorParameters(detectorType,nPixelsX,nPixelsY,pitchX,pitchY,x,y,z,Rx,Ry,Rz,timingOffset);
    detector[detectorID] = detectorSummary;
    
    // Temp: register detector
    registerDetector(detectorID);
    
    cout<<"Device parameters: "<<line<<endl;
    
  }
  cout<<"----------------------------------------------------------------------------------------------------------------------------------------------------------------"<<endl;
  
  // Now check that all devices which are registered have parameters as well
  bool unregisteredDetector=false;
  // Loop over all registered detectors
  for(int det=0;det<nDetectors;det++){
    if(detector.count(detectors[det]) == 0){
      cout<<"Detector "<<detectors[det]<<" has no conditions loaded"<<endl;
      unregisteredDetector = true;
    }
  }
  if(unregisteredDetector) return false;
  
  // Finally, sort the list of detectors by z position (from lowest to highest)
  globalDetector = detector;
  std::sort(detectors.begin(),detectors.end(),sortByZ);

  return true;
  
}

void Parameters::readDutMask() {
  // If no masked file set, do nothing
  if(dutMaskFile == "defaultMask.dat") return;
  detector[this->DUT]->setMaskFile(dutMaskFile);
  // Open the file with masked pixels
  std::cout<<"Reading DUT mask file from "<<dutMaskFile<<std::endl;
  std::fstream inputMaskFile(dutMaskFile.c_str(), std::ios::in);
  int row,col; std::string id;
  std::string line;
  // loop over all lines and apply masks
  while(getline(inputMaskFile,line)){
    inputMaskFile >> id >> row >> col;
    if(id == "c") maskDutColumn(col); // Flag to mask a column
    if(id == "r") maskDutRow(row); // Flag to mask a row
    if(id == "p") detector[this->DUT]->maskChannel(col,row); // Flag to mask a pixel
  }
  return;
}

// The masking of pixels on the dut uses a map with unique
// id for each pixel given by column + row*numberColumns
void Parameters::maskDutColumn(int column){
  int nRows = detector[this->DUT]->nPixelsY();
  for(int row=0;row<nRows;row++) detector[this->DUT]->maskChannel(column,row);
}
void Parameters::maskDutRow(int row){
  int nColumns = detector[this->DUT]->nPixelsX();
  for(int column=0;column<nColumns;column++) detector[this->DUT]->maskChannel(column,row);
}










