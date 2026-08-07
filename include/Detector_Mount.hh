#ifndef Detector_Mount_H
#define Detector_Mount_H 1

#include "G4Material.hh"
#include "Materials.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4SubtractionSolid.hh"
#include "G4AssemblyVolume.hh"
#include "G4ExtrudedSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4ThreeVector.hh"
#include "G4PVPlacement.hh"
#include "G4TwoVector.hh"
#include "G4NistManager.hh"

#include "G4VisAttributes.hh"
#include "G4Colour.hh"

#include "G4RotationMatrix.hh"
#include "G4Transform3D.hh"
#include "Randomize.hh"
#include "globals.hh"
#include <iostream>
#include <iomanip>

using namespace std;

class Detector_Mount
{
public:

  G4LogicalVolume *expHall_log;
  Materials* materials;
  G4Material* Steel = G4NistManager::Instance()->FindOrBuildMaterial("G4_STAINLESS-STEEL");


  Detector_Mount(G4LogicalVolume*, Materials*);
  ~Detector_Mount();

  void Construct();
  //void SetBeamPipe(G4bool val) {isBeamPipe = val;}
  void setGeoFile(const G4String& name) { geoFileName = name; }
 


  //G4String getGeoFile() const { return geoFileName; }

  G4AssemblyVolume* GetLassembly() { return Lassembly; }
  G4AssemblyVolume* GetBPassembly() { return BPassembly; }
  G4AssemblyVolume* GetAssembly() { return isBeamPipe ? BPassembly : Lassembly; }
  
  void SetAssemblyPos(const G4ThreeVector& pos) { assemblyPos = pos; }
  void SetAssemblyRot(const G4RotationMatrix& rot) { assemblyRot = rot; }
  void setX(G4double x){assemblyPos.setX(x);};
  void setY(G4double y){assemblyPos.setY(y);};
  void setZ(G4double z){assemblyPos.setZ(z);};
  void rotateX(G4double ax){assemblyRot.rotateX(ax);};
  void rotateY(G4double ay){assemblyRot.rotateY(ay);};
  void rotateZ(G4double az){assemblyRot.rotateZ(az);};
  G4bool isConstructed(){return constructed;};
  //G4bool IsBeamPipe(){return isBeamPipe;};
  
  void PlaceMount();

  private:

  G4ExtrudedSolid* rawPlate;
  G4ExtrudedSolid* BeamPlate;

  // Logical volume

  G4LogicalVolume* LMount_log;
  G4LogicalVolume* BPMount_log;
  G4LogicalVolume* LargeCradle_log;
  G4LogicalVolume* BeamCradle_log;

  // Materials
  G4Material* EpoxyResin;
  
  // dimensions
  G4double LCradleBox_Length;
  G4double LCradleBox_Width;
  G4double LCradleBox_Depth;
  G4double LCradleHole_Radius;
  

  G4double BPCradleBox_Length;
  G4double BPCradleBox_Width;
  G4double BPCradleBox_Depth;
  G4double BPCradleHole_Radius;


  // position
  G4RotationMatrix CradleHole_Rot;
  G4RotationMatrix Cradle0_Rot;
  G4RotationMatrix Cradle1_Rot;
  G4RotationMatrix BPMount_Rot;
  G4ThreeVector UpperCradle_Shift;
  G4ThreeVector BPUpperCradle_Shift;
  G4ThreeVector LowerCradle_Shift;
  G4ThreeVector LLeft_Shift;
  G4ThreeVector LRight_Shift;
  G4ThreeVector BPLeft_Shift;
  G4ThreeVector BPRight_Shift;
  G4ThreeVector Pos;
  G4RotationMatrix Rot;
  G4RotationMatrix BBPCradle_Rot;
  G4RotationMatrix TBPCradle_Rot;
  G4ThreeVector cradleShift;
  G4ThreeVector CradleHole_Shift;
  G4ThreeVector BPCradleHole_Shift;
  G4ThreeVector assemblyPos;
  G4RotationMatrix assemblyRot;
  
  G4SubtractionSolid* LCradle;
  G4SubtractionSolid* BPCradle;

  G4AssemblyVolume* Lassembly;
  G4AssemblyVolume* BPassembly;

  G4bool constructed;
  G4bool isBeamPipe;

  G4String geoFileName;
  std::ifstream geoFile;
};

#endif

