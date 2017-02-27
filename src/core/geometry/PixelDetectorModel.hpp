/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_PIXEL_DETECTOR_H
#define ALLPIX_PIXEL_DETECTOR_H

#include <string>
#include <tuple>

#include "DetectorModel.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <tuple>
#include <map>
#include <cmath>

//FIXME: this needs a restructure...

namespace allpix {

    class PixelDetectorModel : public DetectorModel {
    public:        
        //Constructor and destructor
        PixelDetectorModel() {}
        ~PixelDetectorModel() {}

        //  Number of pixels
        inline int GetNPixelsX(){return m_npix_x;};
        inline int GetNPixelsY(){return m_npix_y;};
        inline int GetNPixelsZ(){return m_npix_z;};
        inline int GetNPixelsTotXY(){return GetNPixelsX()*GetNPixelsY();}; // Planar layout //
        inline double GetResistivity(){return m_resistivity;};

        //inline int GetHalfNPixelsX(){return GetNPixelsX()/2;}; // half number of pixels //
        //inline int GetHalfNPixelsY(){return GetNPixelsY()/2;};
        //inline int GetHalfNPixelsZ(){return GetNPixelsZ()/2;};

        //  Chip dimensions
        inline double GetChipX(){return 2.*GetHalfChipX();};
        inline double GetChipY(){return 2.*GetHalfChipY();};
        inline double GetChipZ(){return 2.*GetHalfChipZ();};

        inline double GetHalfChipX(){return m_chip_hx;}; // half Chip size //
        inline double GetHalfChipY(){return m_chip_hy;};
        inline double GetHalfChipZ(){return m_chip_hz;};

        //  Pixel dimensions
        inline double GetPixelX(){return 2.*GetHalfPixelX();};
        inline double GetPixelY(){return 2.*GetHalfPixelY();};
        inline double GetPixelZ(){return 2.*GetHalfPixelZ();};

        inline double GetHalfPixelX(){return m_pixsize_x;}; // half pixel size //
        inline double GetHalfPixelY(){return m_pixsize_y;};
        inline double GetHalfPixelZ(){return m_pixsize_z;};

        // Sensor --> It will be positioned with respect to the wrapper !! //
        inline double GetHalfSensorX(){return m_sensor_hx;};
        inline double GetHalfSensorY(){return m_sensor_hy;};
        inline double GetHalfSensorZ(){return m_sensor_hz;};

        inline double GetHalfCoverlayerZ(){return m_coverlayer_hz;};

        inline double GetSensorX(){return 2.*GetHalfSensorX();};
        inline double GetSensorY(){return 2.*GetHalfSensorY();};
        inline double GetSensorZ(){return 2.*GetHalfSensorZ();};

        inline double GetCoverlayerHZ(){return 2.*GetHalfCoverlayerZ();};

        inline double GetSensorXOffset(){return m_sensor_posx;};
        inline double GetSensorYOffset(){return m_sensor_posy;};
        inline double GetSensorZOffset(){return GetHalfPCBZ();}; // See relation with GetHalfWrapperDZ()

        inline double GetChipXOffset(){return m_chip_offsetx;};
        inline double GetChipYOffset(){return m_chip_offsety;};
        inline double GetChipZOffset(){return m_chip_offsetz;}; // See relation with GetHalfWrapperDZ()

        inline double GetBumpRadius(){return m_bump_radius;};
        inline double GetBumpHeight(){return m_bump_height;};
        inline double GetBumpHalfHeight(){return m_bump_height/2.0;};

        inline double GetBumpOffsetX(){return m_bump_offsetx;};
        inline double GetBumpOffsetY(){return m_bump_offsety;};
        inline double GetBumpDr(){return m_bump_dr;};

        inline double GetSensorExcessHTop(){return m_sensor_gr_excess_htop;};
        inline double GetSensorExcessHBottom(){return m_sensor_gr_excess_hbottom;};
        inline double GetSensorExcessHRight(){return m_sensor_gr_excess_hright;};
        inline double GetSensorExcessHLeft(){return m_sensor_gr_excess_hleft;};

        // PCB --> It will be positioned with respect to the wrapper !!
        inline double GetHalfPCBX(){return m_pcb_hx;};
        inline double GetHalfPCBY(){return m_pcb_hy;};
        inline double GetHalfPCBZ(){return m_pcb_hz;};
        inline double GetPCBX(){return 2.*GetPCBX();};
        inline double GetPCBY(){return 2.*GetPCBX();};
        inline double GetPCBZ(){return 2.*GetPCBX();};

        // Wrapper
        inline double GetHalfWrapperDX(){return GetHalfPCBX();};
        inline double GetHalfWrapperDY(){return GetHalfPCBY();};
        inline double GetHalfWrapperDZ(){

            double whdz = GetHalfPCBZ() +
                    GetHalfChipZ() +
                    GetBumpHalfHeight() +
                    GetHalfSensorZ();

            //if ( m_coverlayer_ON ) whdz += GetHalfCoverlayerZ();

            return whdz;
        };
        
        void SetResistivity(double val){
            m_resistivity = val;
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
        /*void SetCoverlayerHZ(double val){
            m_coverlayer_hz = val;
            m_coverlayer_ON = true;
        }*/
        /*void SetCoverlayerMat(G4String mat){
            m_coverlayer_mat = mat;
        }*/

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
    private:

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

        double m_resistivity;
    };

}

#endif /* ALLPIX_PIXEL_DETECTOR_H */
