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
/// \file hadronic/Hadr03/src/RunAction.cc
/// \brief Implementation of the RunAction class
//
// $Id: RunAction.cc 70756 2013-06-05 12:20:06Z ihrivnac $
// 
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include "RunAction.hh"

#include "DetectorConstruction.hh"
#include "PrimaryGeneratorAction.hh"
#include "HistoManager.hh"
#include "Run.hh"

#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"

#include "Randomize.hh"
#include <iomanip>

#include <ctime>
#include <stdlib.h>
#include "CUDA_G4NeutronHPVector.h"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

RunAction::RunAction(DetectorConstruction* det, PrimaryGeneratorAction* prim)
  : G4UserRunAction(),
    fDetector(det), fPrimary(prim), fRun(0), fHistoManager(0)
{
 // Book predefined histograms
 fHistoManager = new HistoManager(); 
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

RunAction::~RunAction()
{
 delete fHistoManager;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4Run* RunAction::GenerateRun()
{ 
  fRun = new Run(fDetector); 
  return fRun;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void RunAction::BeginOfRunAction(const G4Run*)
{    
  // save Rndm status
  G4RunManager::GetRunManager()->SetRandomNumberStore(false);
  G4Random::showEngineStatus();
       
  //histograms
  //
  G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();
  if ( analysisManager->IsActive() ) {
    analysisManager->OpenFile();
  }  
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void RunAction::EndOfRunAction(const G4Run* aRun)
{
  
  // ================== CUDA CODE ==================== //
  #if USE_GPU
    #define arraySize 15
    int *arr1 = new int[arraySize];
    int *arr2 = new int[arraySize];
    for (int i = 0; i < arraySize; i++) {
      arr1[i] = (int)rand() % 100;
      arr2[i] = (int)rand() % 100;
    }
    int *sum = new int[arraySize];
    
    G4cout << "\n***************************** CUDA ***********************************" << G4endl;
    std::clock_t start = std::clock();
    CUDA_sumArrays(arr1, arr2, sum, arraySize);
    std::clock_t end = std::clock();

    G4cout << "Sum of [";
    for (int i = 0; i < arraySize; i++) {
      if (i == arraySize-1)
        G4cout << arr1[i] << "]";
      else
        G4cout << arr1[i] << ", ";
    }
    G4cout << "\nand of [";
    for (int i = 0; i < arraySize; i++) {
      if (i == arraySize-1)
        G4cout << arr2[i] << "]";
      else
        G4cout << arr2[i] << ", ";
    }
    G4cout << "\n Array [";
    for (int i = 0; i < arraySize; i++) {
      if (i == arraySize-1)
        G4cout << sum[i] << "]";
      else
        G4cout << sum[i] << ", ";
    }
    G4cout << "\nComputation done in " << double(end-start)/CLOCKS_PER_SEC << " s" << G4endl;
    G4cout << "******************************** CUDA ********************************\n" << G4endl;
  #else
    G4cout << "\nCUDA is disabled." << G4endl;
  #endif
  // ================ END CUDA CODE ================== //
  
  G4int nbOfEvents = aRun->GetNumberOfEvent();

  if ( fPrimary && nbOfEvents ) { 

    G4int prec = 5;  
    G4int dfprec = G4cout.precision(prec);
    
    G4Material* material = fDetector->GetMaterial();
    G4double density = material->GetDensity();
   
    G4ParticleDefinition* particle = 
                            fPrimary->GetParticleGun()->GetParticleDefinition();
    G4String Particle = particle->GetParticleName();    
    G4double energy = fPrimary->GetParticleGun()->GetParticleEnergy();
    G4cout << "\n The run is of " << nbOfEvents << " "<< Particle << " of "
           << G4BestUnit(energy,"Energy") << " through " 
           << G4BestUnit(fDetector->GetSize(),"Length") << " of "
           << material->GetName() << " (density: " 
           << G4BestUnit(density,"Volumic Mass") << ")" << G4endl;
         
    //restore default format         
    G4cout.precision(dfprec);
  }         
         
  //compute and print statistics
  //
  if (isMaster) fRun->ComputeStatistics();    
  
  //save histograms      
  G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();
  ////G4double factor = 1./nbOfEvents;
  ////analysisManager->ScaleH1(3,factor);      
  if ( analysisManager->IsActive() ) {
    analysisManager->Write();
    analysisManager->CloseFile();
  }
      
  // show Rndm status
  G4Random::showEngineStatus();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
