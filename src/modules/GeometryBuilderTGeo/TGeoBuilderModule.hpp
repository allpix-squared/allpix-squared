/// \file TGeoBuilderModule
/// \brief Implementation of the Geometry builder module using TGeo.
///
/// Builds the detector geometry according to user defined parameters.
///
/// To do :
///  - Refer to the detector desc with their names instead of integers.
///  -
///  -
///
///
/// \date     March 30 2017
/// \version  0.13
/// \author N. Gauvin; CERN

#ifndef ALLPIX_DETECTOR_CONSTRUCTION_TGEO_H
#define ALLPIX_DETECTOR_CONSTRUCTION_TGEO_H

#include <map>
#include <memory>

// ROOT
#include <Math/Vector3D.h>
#include <TGeoManager.h>

// AllPix2
#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/geometry/PixelDetectorModel.hpp"
#include "core/messenger/Messenger.hpp"
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

// ### to be placed in a more adequate place
TGeoTranslation ToTGeoTranslation(const ROOT::Math::XYZPoint& pos);
TString Print(TGeoTranslation* trl);

namespace allpix {

    class TGeoBuilderModule : public Module {

    public:
        TGeoBuilderModule(Configuration config, Messenger*, GeometryManager*);
        ~TGeoBuilderModule() override;

        TGeoBuilderModule(const TGeoBuilderModule&) = delete;
        TGeoBuilderModule& operator=(const TGeoBuilderModule&) = delete;

        // Init method is used to construct the geometry
        void init() override;

    private:
        // Internal methods
        void Construct();
        void BuildPixelDevices();
        void BuildMaterialsAndMedia();
        void BuildAppliances();
        void BuildTestStructure();

        // configuration for this module
        Configuration m_config;

        // Global variables
        GeometryManager* m_geoDscMng;
        TGeoMedium* m_fillingWorldMaterial; /// Medium to fill the World.

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
        std::map<std::string, ROOT::Math::XYZVector> m_vectorWrapperEnhancement;
        std::map<std::string, TGeoTranslation> m_posVectorAppliances; //
    };
} // namespace allpix

#endif /* ALLPIX_DETECTOR_CONSTRUCTION_TGEO_H */
