#ifndef Target_Chamber_H
#define Target_Chamber_H 1

#include "G4Material.hh"
#include "Materials.hh"
#include "G4Tubs.hh"
#include "G4Sphere.hh"
#include "CADMesh.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4ThreeVector.hh"
#include "G4PVPlacement.hh"

#include "G4VisAttributes.hh"
#include "G4Colour.hh"

#include "G4RotationMatrix.hh"
#include "G4Transform3D.hh"
#include "globals.hh"

using namespace std;

class Target_Chamber
{
public:

  G4LogicalVolume *expHall_log;
  Materials* materials;

  Target_Chamber(G4LogicalVolume*, Materials*);
  ~Target_Chamber();

  void Construct();
  
  G4double Radius;      // Outer
  G4double Thickness;
  
  G4Material* Al;

  G4ThreeVector Pos;
  G4RotationMatrix Rot;
  
  G4LogicalVolume*  chamber_log;
  G4VPhysicalVolume* chamber_phys;
  
};

#endif
