/// @file
/// @brief Implementation of the TGeo geometry builder
///
/// Builds the detector geometry according to user defined parameters
///
/// To do :
///  - Refer to the detector desc with their names instead of integers
///
/// @date     March 30 2017
/// @version  0.13

#ifndef ALLPIX_DETECTOR_CONSTRUCTION_TGEO_H
#define ALLPIX_DETECTOR_CONSTRUCTION_TGEO_H

#include <map>
#include <memory>

#include <Math/Vector3D.h>
#include <TGeoManager.h>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/geometry/HybridPixelDetectorModel.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

/* Names of detector parts
 *  These are extremely important and should be placed in a visible way,
 *  as they will be used to retrieve the objects from the gGeoManager.
 */
const TString WrapperName = "Wrapper";
const TString supportName = "support";
const TString WaferName = "Wafer"; // Box in AllPix1
const TString CoverName = "Coverlayer";
const TString SliceName = "Slice";
const TString PixelName = "Pixel";
const TString ChipName = "Chip";
const TString BumpName = "Bump";
const TString GuardRingsName = "GuardRings";

// FIXME To be placed in a more adequate place
TGeoTranslation ToTGeoTranslation(const ROOT::Math::XYZPoint& pos);
TString Print(TGeoTranslation* trl);

namespace allpix {
    /**
     * @brief Module to construct the TGeo from the internal geometry
     */
    class GeometryBuilderTGeoModule : public Module {

    public:
        /**
         * @brief Constructs geometry construction module
         * @param geo_manager Pointer to the geometry manager, containing the detectors
         * @param config Configuration object of the geometry builder module
         */
        GeometryBuilderTGeoModule(Configuration config, Messenger*, GeometryManager*);

        /**
         * @brief Initializes and constructs the TGeo geometry
         */
        void init() override;

    private:
        Configuration m_config;

        /**
         * @brief Construct the TGeo geometry
         */
        void Construct();
        /**
         * @brief Build all detector devices
         */
        void BuildPixelDevices();
        /**
         * @brief Build all the materials
         */
        void BuildMaterialsAndMedia();
        /**
         * @brief Build optional appliances
         */
        void BuildAppliances();
        /**
         * @brief Build optional test structures
         */
        void BuildTestStructure();

        // Global internal variables
        GeometryManager* m_geoDscMng;
        TGeoMedium* m_fillingWorldMaterial;

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
