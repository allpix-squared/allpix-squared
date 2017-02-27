/**
 * @author John Idarraga <idarraga@cern.ch>
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_GEO_DSC_H
#define ALLPIX_MODULE_GEOMETRY_CONSTRUCTION_GEO_DSC_H

//#include "globals.hh"

#include <iostream>
#include <fstream>
#include <string>
#include <tuple>
#include <map>
#include <cmath>

#include "G4ThreeVector.hh"
#include "CLHEP/Units/SystemOfUnits.h"

namespace allpix {

    class AllPixGeoDsc {
    public:

        AllPixGeoDsc() {}
        ~AllPixGeoDsc() {}

        //  Number of pixels
        G4int GetNPixelsX(){return m_npix_x;};
        G4int GetNPixelsY(){return m_npix_y;};
        G4int GetNPixelsZ(){return m_npix_z;};
        G4int GetNPixelsTotXY(){return GetNPixelsX()*GetNPixelsY();}; // Planar layout //
        G4double GetResistivity(){return m_resistivity;};

        //G4int GetHalfNPixelsX(){return GetNPixelsX()/2;}; // half number of pixels //
        //G4int GetHalfNPixelsY(){return GetNPixelsY()/2;};
        //G4int GetHalfNPixelsZ(){return GetNPixelsZ()/2;};

        //  Chip dimensions
        G4double GetChipX(){return 2.*GetHalfChipX();};
        G4double GetChipY(){return 2.*GetHalfChipY();};
        G4double GetChipZ(){return 2.*GetHalfChipZ();};

        G4double GetHalfChipX(){return m_chip_hx;}; // half Chip size //
        G4double GetHalfChipY(){return m_chip_hy;};
        G4double GetHalfChipZ(){return m_chip_hz;};

        //  Pixel dimensions
        G4double GetPixelX(){return 2.*GetHalfPixelX();};
        G4double GetPixelY(){return 2.*GetHalfPixelY();};
        G4double GetPixelZ(){return 2.*GetHalfPixelZ();};

        G4double GetHalfPixelX(){return m_pixsize_x;}; // half pixel size //
        G4double GetHalfPixelY(){return m_pixsize_y;};
        G4double GetHalfPixelZ(){return m_pixsize_z;};

        // Sensor --> It will be positioned with respect to the wrapper !! //
        G4double GetHalfSensorX(){return m_sensor_hx;};
        G4double GetHalfSensorY(){return m_sensor_hy;};
        G4double GetHalfSensorZ(){return m_sensor_hz;};

        G4double GetHalfCoverlayerZ(){return m_coverlayer_hz;};

        G4double GetSensorX(){return 2.*GetHalfSensorX();};
        G4double GetSensorY(){return 2.*GetHalfSensorY();};
        G4double GetSensorZ(){return 2.*GetHalfSensorZ();};

        G4double GetCoverlayerHZ(){return 2.*GetHalfCoverlayerZ();};
        G4String GetCoverlayerMat(){return m_coverlayer_mat;};
        G4bool IsCoverlayerON(){return m_coverlayer_ON;};

        G4double GetSensorXOffset(){return m_sensor_posx;};
        G4double GetSensorYOffset(){return m_sensor_posy;};
        G4double GetSensorZOffset(){return GetHalfPCBZ();}; // See relation with GetHalfWrapperDZ()

        G4double GetChipXOffset(){return m_chip_offsetx;};
        G4double GetChipYOffset(){return m_chip_offsety;};
        G4double GetChipZOffset(){return m_chip_offsetz;}; // See relation with GetHalfWrapperDZ()

        G4double GetBumpRadius(){return m_bump_radius;};
        G4double GetBumpHeight(){return m_bump_height;};
        G4double GetBumpHalfHeight(){return m_bump_height/2.0;};

        G4double GetBumpOffsetX(){return m_bump_offsetx;};
        G4double GetBumpOffsetY(){return m_bump_offsety;};
        G4double GetBumpDr(){return m_bump_dr;};

        G4double GetSensorExcessHTop(){return m_sensor_gr_excess_htop;};
        G4double GetSensorExcessHBottom(){return m_sensor_gr_excess_hbottom;};
        G4double GetSensorExcessHRight(){return m_sensor_gr_excess_hright;};
        G4double GetSensorExcessHLeft(){return m_sensor_gr_excess_hleft;};

        // PCB --> It will be positioned with respect to the wrapper !!
        G4double GetHalfPCBX(){return m_pcb_hx;};
        G4double GetHalfPCBY(){return m_pcb_hy;};
        G4double GetHalfPCBZ(){return m_pcb_hz;};
        G4double GetPCBX(){return 2.*GetPCBX();};
        G4double GetPCBY(){return 2.*GetPCBX();};
        G4double GetPCBZ(){return 2.*GetPCBX();};

        // Wrapper
        G4double GetHalfWrapperDX(){return GetHalfPCBX();};
        G4double GetHalfWrapperDY(){return GetHalfPCBY();};
        G4double GetHalfWrapperDZ(){

            G4double whdz = GetHalfPCBZ() +
                    GetHalfChipZ() +
                    GetBumpHalfHeight() +
                    GetHalfSensorZ();

            if ( m_coverlayer_ON ) whdz += GetHalfCoverlayerZ();

            return whdz;
        };

        // World
        /*G4double GetHalfWorldDX(){return 1000.*mm;};//GetNPixelsX()*GetPixelX();};
        G4double GetHalfWorldDY(){return 1000.*mm;};//GetNPixelsY()*GetPixelX();};
        G4double GetHalfWorldDZ(){return 2000.*mm;};//GetPixelZ();};

        G4double GetWorldDX(){return 2.*GetHalfWorldDX();};
        G4double GetWorldDY(){return 2.*GetHalfWorldDY();};
        G4double GetWorldDZ(){return 2.*GetHalfWorldDZ();};

        G4int GetMIPTot(){return m_MIP_Tot;}
        G4double GetMIPCharge(){return m_MIP_Charge;}
        G4int GetCounterDepth(){return m_Counter_Depth;}
        G4double GetClockUnit(){return m_Clock_Unit;}
        G4double GetChipNoise(){return m_Chip_Noise;}
        G4double GetThreshold(){return m_Chip_Threshold;}
        G4double GetCrossTalk(){return m_Cross_Talk;}
        G4double GetSaturationEnergy(){return m_Saturation_Energy;}
        G4double GetTemperature(){return m_Temperature;};
        G4double GetFlux(){return m_Flux;};
        G4ThreeVector GetMagField(){return m_MagField;};

        G4ThreeVector GetEFieldFromMap(G4ThreeVector);

        G4bool GetEFieldBoolean(){return m_efieldfromfile;};*/

        ///////////////////////////////////////
        // Set
        void SetID(G4int val){
            m_ID = val;
        }
        void SetNPixelsX(G4int val){
            m_npix_x = val;
        };
        void SetNPixelsY(G4int val){
            m_npix_y = val;
        };
        void SetNPixelsZ(G4int val){
            m_npix_z = val;
        };
        void SetPixSizeX(G4double val){
            m_pixsize_x = val;
        }
        void SetPixSizeY(G4double val){
            m_pixsize_y = val;
        }
        void SetPixSizeZ(G4double val){
            m_pixsize_z = val;
        }

        ///////////////////////////////////////////////////
        // Chip
        void SetChipHX(G4double val){
            m_chip_hx = val;
        };
        void SetChipHY(G4double val){
            m_chip_hy = val;
        }
        void SetChipHZ(G4double val){
            m_chip_hz = val;
        };
        void SetChipPosX(G4double val){
            m_chip_posx = val;
        };
        void SetChipPosY(G4double val){
            m_chip_posy = val;
        }
        void SetChipPosZ(G4double val){
            m_chip_posz = val;
        };
        void SetChipOffsetX(G4double val){
            m_chip_offsetx = val;
        };
        void SetChipOffsetY(G4double val){
            m_chip_offsety = val;
        }
        void SetChipOffsetZ(G4double val){
            m_chip_offsetz = val;
        };


        ///////////////////////////////////////////////////
        // Bumps

        void SetBumpRadius(G4double val){
            m_bump_radius=val;
        };

        void SetBumpHeight(G4double val){
            m_bump_height=val;
        };

        void SetBumpOffsetX(G4double val){
            m_bump_offsetx=val;
        };

        void SetBumpOffsetY(G4double val){
            m_bump_offsety=val;
        };

        void SetBumpDr(G4double val){
            m_bump_dr=val;
        };




        ///////////////////////////////////////////////////
        // Sensor
        void SetSensorHX(G4double val){
            m_sensor_hx = val;
        }
        void SetSensorHY(G4double val){
            m_sensor_hy = val;
        }
        void SetSensorHZ(G4double val){
            m_sensor_hz = val;
        }

        void SetCoverlayerHZ(G4double val){
            m_coverlayer_hz = val;
            m_coverlayer_ON = true;
        }
        void SetCoverlayerMat(G4String mat){
            m_coverlayer_mat = mat;
        }

        void SetSensorPosX(G4double val){
            m_sensor_posx = val;
        }
        void SetSensorPosY(G4double val){
            m_sensor_posy = val;
        }
        void SetSensorPosZ(G4double val){
            m_sensor_posz = val;
        }

        void SetSensorExcessHTop(G4double val){
            m_sensor_gr_excess_htop = val;
        }
        void SetSensorExcessHBottom(G4double val){
            m_sensor_gr_excess_hbottom = val;
        }
        void SetSensorExcessHRight(G4double val){
            m_sensor_gr_excess_hright = val;
        }
        void SetSensorExcessHLeft(G4double val){
            m_sensor_gr_excess_hleft = val;
        }

        ///////////////////////////////////////////////////
        // Digitizer
        void SetSensorDigitizer(G4String valS){
            m_digitizer = valS;
        }

        ///////////////////////////////////////////////////
        // PCB
        void SetPCBHX(G4double val){
            m_pcb_hx = val;
        }
        void SetPCBHY(G4double val){
            m_pcb_hy = val;
        }
        void SetPCBHZ(G4double val){
            m_pcb_hz = val;
        }
        void SetResistivity(G4double val){
            m_resistivity = val;
        }
        void SetMIPTot(G4int val){
            m_MIP_Tot = val;
        }
        void SetMIPCharge(G4double val){
            m_MIP_Charge = val;
        }
        void SetCounterDepth(G4int val){
            m_Counter_Depth = val;
        }

        void SetClockUnit(G4double val){
            m_Clock_Unit = val;
        }

        void SetChipNoise(G4double val){
            m_Chip_Noise = val;
        }

        void SetThreshold(G4double val){
            m_Chip_Threshold = val;
        }

        void SetCrossTalk(G4double val){
            m_Cross_Talk = val;
        }

        void SetSaturationEnergy(G4double val){
            m_Saturation_Energy = val;
        }

        void SetTemperature(G4double val){
            m_Temperature = val;
        }

        void SetFlux(G4double val){
            m_Flux = val;
        }

        void SetMagField(G4ThreeVector vals){
            m_MagField = vals;
        }

        void SetEFieldMap(G4String valS);


        ///////////////////////////////////////////////////
        // operators
        //void operator=(AllPixGeoDsc &);

        ///////////////////////////////////////////////////
        // names
        void SetHitsCollectionName(G4String si){ m_hitsCollectionName = si; };
        void SetDigitCollectionName(G4String si){ m_digitCollectionName = si; };

        G4String GetHitsCollectionName(){ return m_hitsCollectionName; };
        G4String GetDigitCollectionName(){ return m_digitCollectionName; };

        G4String GetSensorDigitizer(){return m_digitizer;};

        ///////////////////////////////////////////////////
        // extras
        void Dump();


    private:

        G4int m_ID;

        G4int m_npix_x;
        G4int m_npix_y;
        G4int m_npix_z;

        G4double m_pixsize_x;
        G4double m_pixsize_y;
        G4double m_pixsize_z;

        G4double m_sensor_hx;
        G4double m_sensor_hy;
        G4double m_sensor_hz;

        G4double m_coverlayer_hz;
        G4String m_coverlayer_mat;
        G4bool m_coverlayer_ON;

        G4double m_sensor_posx;
        G4double m_sensor_posy;
        G4double m_sensor_posz;

        G4double m_sensor_gr_excess_htop;
        G4double m_sensor_gr_excess_hbottom;
        G4double m_sensor_gr_excess_hright;
        G4double m_sensor_gr_excess_hleft;

        G4double m_chip_hx;
        G4double m_chip_hy;
        G4double m_chip_hz;

        G4double m_chip_offsetx;
        G4double m_chip_offsety;
        G4double m_chip_offsetz;

        G4double m_chip_posx;
        G4double m_chip_posy;
        G4double m_chip_posz;

        G4double m_pcb_hx;
        G4double m_pcb_hy;
        G4double m_pcb_hz;


        G4double m_bump_radius;
        G4double m_bump_height;
        G4double m_bump_offsetx;
        G4double m_bump_offsety;
        G4double m_bump_dr;

        // hits collection, digitizer collection and digitizer name
        G4String m_digitizer;
        G4String m_hitsCollectionName;
        G4String m_digitCollectionName;

        G4double m_WaferXpos;
        G4double m_WaferYpos;

        G4double m_resistivity;

        G4int m_MIP_Tot;
        G4double m_MIP_Charge;
        G4int m_Counter_Depth;
        G4double m_Clock_Unit;
        G4double m_Chip_Noise;
        G4double m_Chip_Threshold;
        G4double m_Cross_Talk;
        G4double m_Saturation_Energy;
        G4double m_Temperature;
        G4double m_Flux;
        G4ThreeVector m_MagField;

        G4String m_EFieldFile;

        std::vector<std::vector<std::vector<G4ThreeVector>>> m_efieldmap;
        G4int m_efieldmap_nx, m_efieldmap_ny, m_efieldmap_nz;

        G4bool m_efieldfromfile;

    };

}

#endif
