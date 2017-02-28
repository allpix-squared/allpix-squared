/**
 *  Author John Idarraga <idarraga@cern.ch>
 */
//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
//
// $Id: ExN02TrackerSD.hh,v 1.7 2006/06/29 17:47:56 gunter Exp $
// GEANT4 tag $Name: geant4-09-02 $
//
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#ifndef ALLPIX_SIMPLE_DEPOSITION_MODULE_SENSITIVE_DETECTOR_H
#define ALLPIX_SIMPLE_DEPOSITION_MODULE_SENSITIVE_DETECTOR_H

#include "G4VSensitiveDetector.hh"
#include "G4WrapperProcess.hh"

#include <set>

#include "../../core/geometry/Detector.hpp"

class G4Step;
class G4HCofThisEvent;
class G4Event;

namespace allpix{
    class PixelDetectorModel;
    
    class SensitiveDetectorG4 : public G4VSensitiveDetector {
    public:
        SensitiveDetectorG4(std::shared_ptr<Detector>);
        ~SensitiveDetectorG4();
        
        void Initialize(G4HCofThisEvent*);
        G4bool ProcessHits(G4Step*, G4TouchableHistory*);
        void EndOfEvent(G4HCofThisEvent*);
    
    private:
        std::vector<G4Step*> steps_;
        
        // the linked detector
        std::shared_ptr<Detector> detector_; 
        
        bool m_firstStrikePrimary;
        G4double m_kinEPrimary;
        G4double m_totalEdep;
    };
}

#endif /* ALLPIX_SIMPLE_DEPOSITION_MODULE_SENSITIVE_DETECTOR_H */

