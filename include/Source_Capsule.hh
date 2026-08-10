#ifndef Source_Capsule_H
#define Source_Capsule_H 1

#include "G4Material.hh"
#include "Materials.hh"
#include "G4Tubs.hh"
#include "G4Polycone.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4ThreeVector.hh"
#include "G4PVPlacement.hh"

#include "G4VisAttributes.hh"
#include "G4Colour.hh"

#include "G4RotationMatrix.hh"
#include "G4Transform3D.hh"
#include "Randomize.hh"
#include "globals.hh"
#include <iostream>
#include <iomanip>

using namespace std;

class Source_Capsule
{
public:

  G4LogicalVolume *expHall_log;
  Materials* materials;
  
  Source_Capsule(G4LogicalVolume*, Materials*);
  ~Source_Capsule();

  G4VPhysicalVolume *Construct();
  void setType(G4String t){capsuleType = t;};
  void setX(G4double x){Pos.setX(x);};
  void setY(G4double y){Pos.setY(y);};
  void setZ(G4double z){Pos.setZ(z);};

  void rotateX(G4double ax){Rot.rotateX(ax);};
  void rotateY(G4double ay){Rot.rotateY(ay);};
  void rotateZ(G4double az){Rot.rotateZ(az);};

  G4double getDepth(){return Depth;};
  G4double getRadius(){return Radius;};
  
  void PlaceCapsule();

  private:

  G4String capsuleType;
  
  // Logical volume

  G4LogicalVolume* capsule_log;

  // Physical volume
 
  G4VPhysicalVolume* capsule_phys;

  // Materials
  //  G4Material* polyethylene;
  G4Material* plexiglass;
  G4Material* aluminum;

  // dimensions
  G4double Depth;
  G4double Radius;
  G4double Length;
  G4double Width;
  G4double innerRadius;
  G4double Thickness;
  G4double startAngle;
  G4double spanningAngle;

  // position
  G4RotationMatrix Rot;
  G4ThreeVector Pos;
  
  //  G4Tubs* capsule;

};

#endif

