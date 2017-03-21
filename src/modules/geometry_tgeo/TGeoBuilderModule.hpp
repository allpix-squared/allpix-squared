/// \file TGeoBuilderModule
/// \brief Implementation of the Geometry builder module using TGeo.
///
/// Builds the detector geometry according to user defined parameters.
///
/// To do :
///  - Integrate Configuration
///  - Refer to the detector desc with their names instead of integers.
///  - Change TVector3 with XYZVector
///
///
/// \date     March 20 2017
/// \version  0.10
/// \author N. Gauvin; CERN

#ifndef ALLPIX_DETECTOR_CONSTRUCTION_TGEO_H
#define ALLPIX_DETECTOR_CONSTRUCTION_TGEO_H

#include <map>
#include <memory>
#include <vector>

// ROOT
#include <TGeoManager.h>
#include <TVector3.h>   //### to be removed
#include <Math/Vector3D.h>

// AllPix2
#include "core/config/Configuration.hpp"
#include "core/geometry/PixelDetectorModel.hpp"
#include "core/module/Module.hpp"

/*** Names of detector parts
 * These are extremely important and should be placed in a visible way,
 * as they will be used to retrieve the objects from the gGeoManager.
 ***/
const TString WrapperName = "Wrapper";
const TString PCBName = "PCB";
const TString WaferName = "Wafer"; // Box in AllPix1
const TString CoverName = "Coverlayer";
const TString SliceName = "Slice";
const TString PixelName = "Pixel";
const TString ChipName = "Chip";
const TString BumpName = "Bump";
const TString GuardRingsName = "GuardRings";

namespace allpix {

    class TGeoBuilderModule : public Module {

    public:
      // provide a static const variable of type string (required!)
      static const std::string name;

        // constructor should take a pointer to AllPix, a ModuleIdentifier and a Configuration as input      
        TGeoBuilderModule(AllPix* apx, Configuration config);
        ~TGeoBuilderModule() override;

        TGeoBuilderModule(const TGeoBuilderModule&) = delete;
        TGeoBuilderModule& operator=(const TGeoBuilderModule&) = delete;

        // Module run method
        void run() override;

    private:      
        // Internal methods
        void Construct();
        void BuildPixelDevices();
        void BuildMaterialsAndMedia();
        void BuildAppliances();
        void BuildTestStructure();
        void ReadDetectorDescriptions(); //### Debugging only !

        // Global variables
        TGeoMedium* m_fillingWorldMaterial;                        /// Medium to fill the World.
        std::vector<std::shared_ptr<PixelDetectorModel>> m_geoMap; /// the detector descriptions

        // User defined parameters
        /*
          Medium to fill the World. Available media :
          - Air
          - Vacuum
        */
        TString m_userDefinedWorldMaterial;
        TString m_userDefinedGeoOutputFile;
        bool m_buildAppliancesFlag;
        int m_Appliances_type;
        bool m_buildTestStructureFlag;
        std::map<int, TVector3> m_vectorWrapperEnhancement;
        std::map<int, TGeoTranslation> m_posVector;           // position of medipix(es), key is detector Id
        std::map<int, TGeoRotation> m_rotVector;              // map<int, G4RotationMatrix *>
        std::map<int, TGeoTranslation> m_posVectorAppliances; //

      // configuration for this module
        Configuration m_config;
    };
}

#endif /* ALLPIX_DETECTOR_CONSTRUCTION_TGEO_H */
