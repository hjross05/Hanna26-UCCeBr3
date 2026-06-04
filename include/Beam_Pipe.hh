#ifndef Beam_Pipe_H
#define Beam_Pipe_H 1

#include "G4Material.hh"
#include "Materials.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4ThreeVector.hh"
#include "G4PVPlacement.hh"
#include "G4Tubs.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"

#include "G4RotationMatrix.hh"
#include "G4Transform3D.hh"
#include "Randomize.hh"
#include "globals.hh"
#include <iostream>
#include <iomanip>

using namespace std;

class Beam_Pipe
{
public:

  G4LogicalVolume *expHall_log;
  Materials* materials;
  
  Beam_Pipe(G4LogicalVolume*, Materials*);
  ~Beam_Pipe();

  void Construct();

  G4double Radius;
  G4double Thickness;
  G4double hz;

  G4Material* Al;

  G4double startAngle; 
  G4double spanningAngle; 

  G4ThreeVector Pos;
  G4RotationMatrix Rot;
  
  G4LogicalVolume*  pipe_log;
  G4VPhysicalVolume* pipe_phys;

};

#endif

