/**
 * This is just AllPix1's AllPixGeoDsc class with G4 dependencies removed.
 * Assuning distance units are mm
 */

#ifndef GeoDsc_h
#define GeoDsc_h 1

#include <iostream>
#include <fstream>
#include <string>
#include <tuple>
#include <sys/stat.h>
#include <unistd.h>
#include <map>
#include <math.h>
#include <vector>

// ROOT
#include "TString.h"
#include "TVector3.h"

using namespace std;

class GeoDsc {

 public:

  GeoDsc();
  ~GeoDsc();

  int GetID(){return m_ID;};
  //  Number of pixels
  int GetNPixelsX(){return m_npix_x;};
  int GetNPixelsY(){return m_npix_y;};
  int GetNPixelsZ(){return m_npix_z;};
  int GetNPixelsTotXY(){return GetNPixelsX()*GetNPixelsY();}; // Planar layout //
  double GetResistivity(){return m_resistivity;};

  //  Chip dimensions
  double GetChipX(){return 2.*GetHalfChipX();};
  double GetChipY(){return 2.*GetHalfChipY();};
  double GetChipZ(){return 2.*GetHalfChipZ();};

  double GetHalfChipX(){return m_chip_hx;}; // half Chip size //
  double GetHalfChipY(){return m_chip_hy;};
  double GetHalfChipZ(){return m_chip_hz;};

  //  Pixel dimensions
  double GetPixelX(){return 2.*GetHalfPixelX();};
  double GetPixelY(){return 2.*GetHalfPixelY();};
  double GetPixelZ(){return 2.*GetHalfPixelZ();};

  double GetHalfPixelX(){return m_pixsize_x;}; // half pixel size //
  double GetHalfPixelY(){return m_pixsize_y;};
  double GetHalfPixelZ(){return m_pixsize_z;};

  // Sensor --> It will be positioned with respect to the wrapper !! //
  double GetHalfSensorX(){return m_sensor_hx;};
  double GetHalfSensorY(){return m_sensor_hy;};
  double GetHalfSensorZ(){return m_sensor_hz;};

  double GetHalfCoverlayerZ(){return m_coverlayer_hz;};

  double GetSensorX(){return 2.*GetHalfSensorX();};
  double GetSensorY(){return 2.*GetHalfSensorY();};
  double GetSensorZ(){return 2.*GetHalfSensorZ();};

  double GetCoverlayerHZ(){return 2.*GetHalfCoverlayerZ();};
  TString GetCoverlayerMat(){return m_coverlayer_mat;};
  bool IsCoverlayerON(){return m_coverlayer_ON;};

  double GetSensorXOffset(){return m_sensor_posx;};
  double GetSensorYOffset(){return m_sensor_posy;};
  double GetSensorZOffset(){return GetHalfPCBZ();}; // See relation with GetHalfWrapperDZ()

  double GetChipXOffset(){return m_chip_offsetx;};
  double GetChipYOffset(){return m_chip_offsety;};
  double GetChipZOffset(){return m_chip_offsetz;}; // See relation with GetHalfWrapperDZ()

  double GetBumpRadius(){return m_bump_radius;};
  double GetBumpHeight(){return m_bump_height;};
  double GetBumpHalfHeight(){return m_bump_height/2.0;};

  double GetBumpOffsetX(){return m_bump_offsetx;};
  double GetBumpOffsetY(){return m_bump_offsety;};
  double GetBumpDr(){return m_bump_dr;};

  double GetSensorExcessHTop(){return m_sensor_gr_excess_htop;};
  double GetSensorExcessHBottom(){return m_sensor_gr_excess_hbottom;};
  double GetSensorExcessHRight(){return m_sensor_gr_excess_hright;};
  double GetSensorExcessHLeft(){return m_sensor_gr_excess_hleft;};

  // PCB --> It will be positioned with respect to the wrapper !!
  double GetHalfPCBX(){return m_pcb_hx;};
  double GetHalfPCBY(){return m_pcb_hy;};
  double GetHalfPCBZ(){return m_pcb_hz;};
  double GetPCBX(){return 2.*GetPCBX();};
  double GetPCBY(){return 2.*GetPCBX();};
  double GetPCBZ(){return 2.*GetPCBX();};

  // Wrapper
  double GetHalfWrapperDX(){return GetHalfPCBX();};
  double GetHalfWrapperDY(){return GetHalfPCBY();};
  double GetHalfWrapperDZ(){
    
    double whdz = GetHalfPCBZ() +
      GetHalfChipZ() +
      GetBumpHalfHeight() +
      GetHalfSensorZ();
    
    if ( m_coverlayer_ON ) whdz += GetHalfCoverlayerZ();
    
    return whdz;
  };

  // World
  double GetHalfWorldDX(){return 1000.;};//GetNPixelsX()*GetPixelX();};
  double GetHalfWorldDY(){return 1000.;};//GetNPixelsY()*GetPixelX();};
  double GetHalfWorldDZ(){return 2000.;};//GetPixelZ();};

  double GetWorldDX(){return 2.*GetHalfWorldDX();};
  double GetWorldDY(){return 2.*GetHalfWorldDY();};
  double GetWorldDZ(){return 2.*GetHalfWorldDZ();};

  int GetMIPTot(){return m_MIP_Tot;}
  double GetMIPCharge(){return m_MIP_Charge;}
  int GetCounterDepth(){return m_Counter_Depth;}
  double GetClockUnit(){return m_Clock_Unit;}
  double GetChipNoise(){return m_Chip_Noise;}
  double GetThreshold(){return m_Chip_Threshold;}
  double GetCrossTalk(){return m_Cross_Talk;}
  double GetSaturationEnergy(){return m_Saturation_Energy;}
  double GetTemperature(){return m_Temperature;};
  double GetFlux(){return m_Flux;};
  TVector3 GetMagField(){return m_MagField;};

  TVector3 GetEFieldFromMap(TVector3);

  bool GetEFieldBoolean(){return m_efieldfromfile;};



  ///////////////////////////////////////
  // Set
  void SetID(int val){
    m_ID = val;
  }
  void SetNPixelsX(int val){
    m_npix_x = val;
  };
  void SetNPixelsY(int val){
    m_npix_y = val;
  };
  void SetNPixelsZ(int val){
    m_npix_z = val;
  };
  void SetPixSizeX(double val){
    m_pixsize_x = val;
  }
  void SetPixSizeY(double val){
    m_pixsize_y = val;
  }
  void SetPixSizeZ(double val){
    m_pixsize_z = val;
  }

  ///////////////////////////////////////////////////
  // Chip
  void SetChipHX(double val){
    m_chip_hx = val;
  };
  void SetChipHY(double val){
    m_chip_hy = val;
  }
  void SetChipHZ(double val){
    m_chip_hz = val;
  };
  void SetChipPosX(double val){
    m_chip_posx = val;
  };
  void SetChipPosY(double val){
    m_chip_posy = val;
  }
  void SetChipPosZ(double val){
    m_chip_posz = val;
  };
  void SetChipOffsetX(double val){
    m_chip_offsetx = val;
  };
  void SetChipOffsetY(double val){
    m_chip_offsety = val;
  }
  void SetChipOffsetZ(double val){
    m_chip_offsetz = val;
  };


  ///////////////////////////////////////////////////
  // Bumps

  void SetBumpRadius(double val){
    m_bump_radius=val;
  };

  void SetBumpHeight(double val){
    m_bump_height=val;
  };

  void SetBumpOffsetX(double val){
    m_bump_offsetx=val;
  };

  void SetBumpOffsetY(double val){
    m_bump_offsety=val;
  };

  void SetBumpDr(double val){
    m_bump_dr=val;
  };




  ///////////////////////////////////////////////////
  // Sensor
  void SetSensorHX(double val){
    m_sensor_hx = val;
  }
  void SetSensorHY(double val){
    m_sensor_hy = val;
  }
  void SetSensorHZ(double val){
    m_sensor_hz = val;
  }

  void SetCoverlayerHZ(double val){
    m_coverlayer_hz = val;
    m_coverlayer_ON = true;
  }
  void SetCoverlayerMat(TString mat){
    m_coverlayer_mat = mat;
  }

  void SetSensorPosX(double val){
    m_sensor_posx = val;
  }
  void SetSensorPosY(double val){
    m_sensor_posy = val;
  }
  void SetSensorPosZ(double val){
    m_sensor_posz = val;
  }

  void SetSensorExcessHTop(double val){
    m_sensor_gr_excess_htop = val;
  }
  void SetSensorExcessHBottom(double val){
    m_sensor_gr_excess_hbottom = val;
  }
  void SetSensorExcessHRight(double val){
    m_sensor_gr_excess_hright = val;
  }
  void SetSensorExcessHLeft(double val){
    m_sensor_gr_excess_hleft = val;
  }

  ///////////////////////////////////////////////////
  // Digitizer
  void SetSensorDigitizer(TString valS){
    m_digitizer = valS;
  }

  ///////////////////////////////////////////////////
  // PCB
  void SetPCBHX(double val){
    m_pcb_hx = val;
  }
  void SetPCBHY(double val){
    m_pcb_hy = val;
  }
  void SetPCBHZ(double val){
    m_pcb_hz = val;
  }
  void SetResistivity(double val){
    m_resistivity = val;
  }
  void SetMIPTot(int val){
    m_MIP_Tot = val;
  }
  void SetMIPCharge(double val){
    m_MIP_Charge = val;
  }
  void SetCounterDepth(int val){
    m_Counter_Depth = val;
  }

  void SetClockUnit(double val){
    m_Clock_Unit = val;
  }

  void SetChipNoise(double val){
    m_Chip_Noise = val;
  }

  void SetThreshold(double val){
    m_Chip_Threshold = val;
  }

  void SetCrossTalk(double val){
    m_Cross_Talk = val;
  }

  void SetSaturationEnergy(double val){
    m_Saturation_Energy = val;
  }

  void SetTemperature(double val){
    m_Temperature = val;
  }

  void SetFlux(double val){
    m_Flux = val;
  }

  void SetMagField(TVector3 vals){
    m_MagField = vals;
  }

  void SetEFieldMap(TString valS);


  ///////////////////////////////////////////////////
  // operators
  //void operator=(GeoDsc &);

  ///////////////////////////////////////////////////
  // names
  void SetHitsCollectionName(TString si){ m_hitsCollectionName = si; };
  void SetDigitCollectionName(TString si){ m_digitCollectionName = si; };

  TString GetHitsCollectionName(){ return m_hitsCollectionName; };
  TString GetDigitCollectionName(){ return m_digitCollectionName; };

  TString GetSensorDigitizer(){return m_digitizer;};

  ///////////////////////////////////////////////////
  // extras
  void Dump();


 private:

  int m_ID;

  int m_npix_x;
  int m_npix_y;
  int m_npix_z;

  double m_pixsize_x;
  double m_pixsize_y;
  double m_pixsize_z;

  double m_sensor_hx;
  double m_sensor_hy;
  double m_sensor_hz;

  double m_coverlayer_hz;
  TString m_coverlayer_mat;
  bool m_coverlayer_ON;

  double m_sensor_posx;
  double m_sensor_posy;
  double m_sensor_posz;

  double m_sensor_gr_excess_htop;
  double m_sensor_gr_excess_hbottom;
  double m_sensor_gr_excess_hright;
  double m_sensor_gr_excess_hleft;

  double m_chip_hx;
  double m_chip_hy;
  double m_chip_hz;

  double m_chip_offsetx;
  double m_chip_offsety;
  double m_chip_offsetz;

  double m_chip_posx;
  double m_chip_posy;
  double m_chip_posz;

  double m_pcb_hx;
  double m_pcb_hy;
  double m_pcb_hz;


  double m_bump_radius;
  double m_bump_height;
  double m_bump_offsetx;
  double m_bump_offsety;
  double m_bump_dr;

  // hits collection, digitizer collection and digitizer name
  TString m_digitizer;
  TString m_hitsCollectionName;
  TString m_digitCollectionName;

  double m_WaferXpos;
  double m_WaferYpos;

  double m_resistivity;

  int m_MIP_Tot;
  double m_MIP_Charge;
  int m_Counter_Depth;
  double m_Clock_Unit;
  double m_Chip_Noise;
  double m_Chip_Threshold;
  double m_Cross_Talk;
  double m_Saturation_Energy;
  double m_Temperature;
  double m_Flux;
  TVector3 m_MagField;

  TString m_EFieldFile;

  vector<vector<vector<TVector3>>> m_efieldmap;
  int m_efieldmap_nx, m_efieldmap_ny, m_efieldmap_nz;

  bool m_efieldfromfile;

};


#endif
