/**
 * @author John Idarraga <idarraga@cern.ch>
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "ReadGeoDescription.hpp"
#include "ReadGeoDescription_defs.h"

#include "../../core/utils/exceptions.h"

#include <Riostream.h>
#include <TDOMParser.h>
#include <TXMLNode.h>
#include <TXMLAttr.h>
#include <TList.h>
#include <TString.h>

#include "CLHEP/Units/SystemOfUnits.h"

#include <set>

using namespace allpix;
using namespace std;

ReadGeoDescription::ReadGeoDescription(string xmlFile){
    // Units map
    // Initializing units from strings to
    // map units from the xml file.
    m_unitsMap["nm"] = CLHEP::nm;
    m_unitsMap["um"] = CLHEP::um;
    m_unitsMap["mm"] = CLHEP::mm;
    m_unitsMap["cm"] = CLHEP::cm;
    m_unitsMap["m"] = CLHEP::m;
    
    std::ifstream file(xmlFile);
    if(!file.good()) throw ModuleException("Geometry description file not found");
    
    // list of expected tags
    TDOMParser domParser;
    m_currentType = "";
    
    domParser.SetValidate(false); // do not validate with DTD for now
    domParser.ParseFile(xmlFile.c_str());
    
    TXMLNode *node = domParser.GetXMLDocument()->GetRootNode();
    
    // parse
    ParseContext(node);
}

void ReadGeoDescription::ParseContext(TXMLNode *node)
{
    
    string tempContent;
    string tempAtt1;
    
    for ( ; node ; node = node->GetNextNode()) {
        
        if (node->GetNodeType() == TXMLNode::kXMLElementNode) { // Element Node
            
            m_currentNodeName = string(node->GetNodeName());
                        
            // Catch properties first if any
            // This should be the right attribute each time
            // guaranteed by the dtd file
            if (node->HasAttributes()) {
                
                TList * attrList = node->GetAttributes();
                TIter next(attrList);
                TXMLAttr *attr;
                
                while ((attr =(TXMLAttr*) next())) {
                    
                    // verifying attributes names
                    tempContent = string(attr->GetName());       // att name
                    
                    if(tempContent == __pixeldet_node_ATT_id_S && m_currentNodeName == __pixeldet_node_S) { // check if this is the right attribute "id"                                            
                        tempAtt1 = string(attr->GetValue());      // fetch the value
                                                
                        if(StringIsRelevant(tempAtt1)){
                            // get type of current detector
                            m_currentType = tempAtt1;
                            
                            if(m_detsGeo.find(m_currentType) != m_detsGeo.end()){
                                throw ModuleException("Geometry descriptions contains the same type twice");
                            }
                            
                            // create a new one
                            m_detsGeo[m_currentType] = std::make_shared<PixelDetectorModel>();
                            m_detsGeo[m_currentType]->setType(m_currentType);
                            
                        }
                    }else if(tempContent == __pixeldet_global_ATT_units_S){
                        tempAtt1 = string(attr->GetValue());      // fetch the value
                        if(StringIsRelevant(tempAtt1)){
                            // use this units when processing the contents
                            m_currentAtt = tempAtt1;
                        }
                    }
                    
                }
                
            }
            
            
        }
        if (node->GetNodeType() == TXMLNode::kXMLTextNode) { // Text node
            
            // get the other nodes into the GeoDsc for the current detector
            tempContent = string(node->GetContent());
            
            if(StringIsRelevant(tempContent)){
                
                if(m_currentNodeName == __npix_x_S) {
                    
                    int val = atoi(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetNPixelsX(val);
                    
                }else if(m_currentNodeName == __npix_y_S){
                    
                    int val = atoi(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetNPixelsY(val);
                    
                }else if(m_currentNodeName == __npix_z_S){
                    
                    int val = atoi(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetNPixelsZ(val);
                    
                }else if(m_currentNodeName == __chip_hx_S){
                    
                    float val = atof(tempContent.c_str());
                    
                    m_detsGeo[m_currentType]->SetChipHX(val*m_unitsMap[m_currentAtt]);
                    
                }else if(m_currentNodeName == __chip_hy_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetChipHY(val*m_unitsMap[m_currentAtt]);
                    
                }else if(m_currentNodeName == __chip_hz_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetChipHZ(val*m_unitsMap[m_currentAtt]);
                    
                }else if(m_currentNodeName == __chip_posx_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetChipPosX(val*m_unitsMap[m_currentAtt]);
                    
                }else if(m_currentNodeName == __chip_posy_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetChipPosY(val*m_unitsMap[m_currentAtt]);
                    
                }else if(m_currentNodeName == __chip_posz_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetChipPosZ(val*m_unitsMap[m_currentAtt]);
                    
                }else if(m_currentNodeName == __chip_offsetx_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetChipOffsetX(val*m_unitsMap[m_currentAtt]);
                    
                }else if(m_currentNodeName == __chip_offsety_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetChipOffsetY(val*m_unitsMap[m_currentAtt]);
                    
                }else if(m_currentNodeName == __chip_offsetz_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetChipOffsetZ(val*m_unitsMap[m_currentAtt]);
                    
                }else if(m_currentNodeName == __pixsize_x_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetPixSizeX(val*m_unitsMap[m_currentAtt]);
                    
                }else if(m_currentNodeName == __pixsize_y_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetPixSizeY(val*m_unitsMap[m_currentAtt]);
                    
                }else if(m_currentNodeName == __pixsize_z_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetPixSizeZ(val*m_unitsMap[m_currentAtt]);
                    
                }else if(m_currentNodeName == __sensor_hx_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetSensorHX(val*m_unitsMap[m_currentAtt]);
                    
                }else if(m_currentNodeName == __sensor_hy_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetSensorHY(val*m_unitsMap[m_currentAtt]);
                    
                }else if(m_currentNodeName == __sensor_hz_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetSensorHZ(val*m_unitsMap[m_currentAtt]);

                }else if(m_currentNodeName == __sensor_posx_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetSensorPosX(val*m_unitsMap[m_currentAtt]);
                    
                }else if(m_currentNodeName == __sensor_posy_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetSensorPosY(val*m_unitsMap[m_currentAtt]);
                    
                }else if(m_currentNodeName == __sensor_posz_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetSensorPosZ(val*m_unitsMap[m_currentAtt]);
                    
                }else if(m_currentNodeName == __pcb_hx_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetPCBHX(val*m_unitsMap[m_currentAtt]);
                    
                }else if(m_currentNodeName == __pcb_hy_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetPCBHY(val*m_unitsMap[m_currentAtt]);
                    
                }else if(m_currentNodeName == __pcb_hz_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetPCBHZ(val*m_unitsMap[m_currentAtt]);
                    
                }else if(m_currentNodeName == __sensor_gr_excess_htop_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetSensorExcessHTop(val*m_unitsMap[m_currentAtt]);
                    
                }else if(m_currentNodeName == __sensor_gr_excess_hbottom_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetSensorExcessHBottom(val*m_unitsMap[m_currentAtt]);
                    
                }else if(m_currentNodeName == __sensor_gr_excess_hright_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetSensorExcessHRight(val*m_unitsMap[m_currentAtt]);
                    
                }else if(m_currentNodeName == __sensor_gr_excess_hleft_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetSensorExcessHLeft(val*m_unitsMap[m_currentAtt]);
                    
                }/*else if(m_currentNodeName == __digitizer_S){
                    
                    G4String valS(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetSensorDigitizer(valS);
                }*/
                
                else if(m_currentNodeName == __sensor_Resistivity){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetResistivity(val);
                }				
                
                
                /* else if(m_currentNodeName == __MIP_Tot_S){
                    
                    float val = atoi(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetMIPTot(val);
                }
                
                else if(m_currentNodeName == __MIP_Charge_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetMIPCharge(val);
                    
                }
                
                else if(m_currentNodeName == __Counter_Depth_S){
                    
                    float val = atoi(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetCounterDepth(val);
                    
                }				
                
                else if(m_currentNodeName == __Clock_Unit_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetClockUnit(val);
                    
                }					
                
                else if(m_currentNodeName == __Chip_Noise_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetChipNoise(val);
                    
                }
                
                else if(m_currentNodeName == __Chip_Threshold_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetThreshold(val);
                    
                }
                
                else if(m_currentNodeName == __Cross_Talk_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetCrossTalk(val);
                    
                }
                
                
                else if(m_currentNodeName == __Saturation_Energy_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetSaturationEnergy(val);
                    
                }*/
                
                else if(m_currentNodeName == __Bump_Radius_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetBumpRadius(val*m_unitsMap[m_currentAtt]);
                    
                }
                
                else if(m_currentNodeName == __Bump_Height_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetBumpHeight(val*m_unitsMap[m_currentAtt]);
                    
                }
                
                else if(m_currentNodeName == __Bump_OffsetX_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetBumpOffsetX(val*m_unitsMap[m_currentAtt]);
                    
                }
                
                else if(m_currentNodeName == __Bump_OffsetY_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetBumpOffsetY(val*m_unitsMap[m_currentAtt]);
                    
                }
                
                else if(m_currentNodeName == __Bump_Dr_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetBumpDr(val*m_unitsMap[m_currentAtt]);
                    
                }
                                    
                /*}
                else if(m_currentNodeName == __coverlayer_hz_S){
                    
                    float val = atof(tempContent.c_str());
                    m_detsGeo[m_currentType]->SetCoverlayerHZ(val*m_unitsMap[m_currentAtt]);
                    
                }else if(m_currentNodeName == __coverlayer_mat_S){
                    
                    G4String valS(tempContent);
                    m_detsGeo[m_currentType]->SetCoverlayerMat(valS);
                */
            }
        }
        
        ParseContext(node->GetChildren());
    }
}

std::map<std::string, std::shared_ptr<PixelDetectorModel > > &ReadGeoDescription::GetDetectorsMap(){
    return m_detsGeo;
}

bool ReadGeoDescription::StringIsRelevant(string s){
    
    TString t(s);
    if(t.Contains('\n') || t.Contains('\t'))
        return false;
    
    return true;
}
