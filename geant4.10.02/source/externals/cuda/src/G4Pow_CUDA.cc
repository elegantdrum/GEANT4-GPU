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
// $Id: G4Pow.cc 93311 2015-10-16 10:16:37Z gcosmo $
//
// -------------------------------------------------------------------
//
// GEANT4 Class file
//
//
// File name:     G4Pow
//
// Author:        Vladimir Ivanchenko
//
// Creation date: 23.05.2009
//
// Modifications:
// 08.01.2011 V.Ivanchenko extended maxZ from 256 to 512
// 02.05.2013 V.Ivanchenko added expA and logX methods, 
//            revised A13, logA, powZ, powA to improved accuracy
//
// -------------------------------------------------------------------

#include "G4Pow_CUDA.hh"
// #ifdef G4MULTITHREADED
// #include "G4Threading.hh"
// #endif

G4Pow* G4Pow::fpInstance = 0;

// -------------------------------------------------------------------

G4Pow* G4Pow::GetInstance()
{
  if (fpInstance == 0)
  {
    static G4Pow geant4pow;
    fpInstance = &geant4pow;
  }
  return fpInstance;
}

// -------------------------------------------------------------------

G4Pow::G4Pow()
  : onethird(1.0/3.0), max2(5)
{  
// #ifdef G4MULTITHREADED
//   if(G4Threading::IsWorkerThread())
//   { 
//     G4Exception ("G4Pow::G4Pow()", "InvalidSetup", FatalException, 
//                  "Attempt to instantiate G4Pow in worker thread!");
//   }
// #endif
  const G4int maxZ = 512; 
  const G4int maxZfact = 170; 

  maxA    = -0.6 + maxZ;
  maxA2   = 1.25 + max2*0.2;
  maxAexp = -0.76+ maxZfact*0.5;

  ener.resize(max2+1,1.0);
  logen.resize(max2+1,0.0);
  lz2.resize(max2+1,0.0);
  pz13.resize(maxZ,0.0);
  lz.resize(maxZ,0.0);
  fexp.resize(maxZfact,0.0);
  fact.resize(maxZfact,0.0);
  logfact.resize(maxZ,0.0);

  G4double f = 1.0;
  G4double logf = 0.0;
  fact[0] = 1.0;
  fexp[0] = 1.0;

  for(G4int i=1; i<=max2; ++i)
  {
    ener[i] = powN(500.,i); 
    logen[i]= G4Log(ener[i]); 
    lz2[i]  = G4Log(1.0 + i*0.2);
  }

  for(G4int i=1; i<maxZ; ++i)
  {
    G4double x  = G4double(i);
    pz13[i] = std::pow(x,onethird);
    lz[i]   = G4Log(x);
    if(i < maxZfact)
    { 
      f *= x; 
      fact[i] = f;
      fexp[i] = G4Exp(0.5*i);
    }
    logf += lz[i];
    logfact[i] = logf;
  }
}

// -------------------------------------------------------------------

G4Pow::~G4Pow()
{}

// -------------------------------------------------------------------

G4double G4Pow::powN(G4double x, G4int n) const
{
  if(0.0 == x)        { return 0.0; }
  if(std::abs(n) > 8) { return std::pow(x, G4double(n)); }
  G4double res = 1.0;
  if(n >= 0) { for(G4int i=0; i<n; ++i) { res *= x; } }
  else if(n < 0)
  {
    G4double y = 1.0/x;
    G4int nn = -n;
    for(G4int i=0; i<nn; ++i) { res *= y; }
  }
  return res;
}
