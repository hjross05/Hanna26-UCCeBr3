#ifndef Target_Chamber_H
#define Target_Chamber_H 1

#include "G4Material.hh"
#include "Materials.hh"
#include "G4Tubs.hh"
#include "G4Sphere.hh"
#include "G4Polycone.hh"
#include "G4UnionSolid.hh"
#include "G4SubtractionSolid.hh"
#include "G4ExtrudedSolid.hh"
#include "CADMesh.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4ThreeVector.hh"
#include "G4TwoVector.hh"
#include "G4PVPlacement.hh"
#include "G4RunManager.hh"

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

  void setLadderX(G4double val);
  void setLadderY(G4double val);
  void setLadderZ(G4double val);

  void setHole(G4int val);

  void UpdatePlacement();

  void Construct();

  //Target Chamber
  /*G4double Radius;      // Outer (sphere)
  G4double Thickness;*/ 

  G4double Hemi_Radius;      // Outer
  G4double Hemi_Thickness;

  G4double Mid_Radius;     //Outer
  G4double Mid_Thickness;
  G4double Mid_Length;

  G4double Ring_Radius;     //Outer
  G4double Ring_Thickness;
  G4double Ring_Length;
  
  G4double End_Radius;     //Outer
  G4double End_Thickness;
  G4double End_Length;

  //Beam Pipe
  G4double BPRadius;
  G4double BPThickness;
  G4double BPLength;

  //Gate Valve Elements
  G4double GateValve_Length;
  G4double GateValve_Width;
  G4double GateValve_Depth;

  G4double ValveCap_Length;
  G4double ValveCap_Width;
  G4double ValveCap_Depth;

  /*G4double Barrel_Radius
  G4double Barrel_Length
  
  G4double Join_TRadius //Outer
  G4double Join_BRadius
  G4double Join_Length*/

  //KF50 Flange
  static const G4int fNumZPlanes = 5;
  G4double fZPlane[fNumZPlanes];
  G4double fRInner[fNumZPlanes];
  G4double fROuter[fNumZPlanes];

  static const G4int NumZPlanes = 6;
  G4double ZPlane[NumZPlanes];
  G4double RInner[NumZPlanes];
  G4double ROuter[NumZPlanes];

  static const G4int WNumZPlanes = 3;
  G4double WZPlane[WNumZPlanes];
  G4double WRInner[WNumZPlanes];
  G4double WROuter[WNumZPlanes];

  //Hole
  G4double FDrill_Radius;
  G4double FDrill_Length;
  G4double FDrill_Thickness;

  //KF50 Clamp
  static const G4int NumCPlanes = 6; 
  G4double CPlane[NumCPlanes];
  G4double CInner[NumCPlanes];
  G4double COuter[NumCPlanes];

  G4double CBox_Length; 
  G4double CBox_Width;
  G4double CBox_Depth;

  //KF40 Clamp
  G4double C40Plane[NumCPlanes];
  G4double C40Inner[NumCPlanes];
  G4double C40Outer[NumCPlanes];

  //Support Tube
  G4double Support_Radius;
  G4double Support_Length;

  //Piece between support tube and base
  G4double Flat_Radius;
  G4double Flat_Length;

  //Base
  G4double Base_Radius;
  G4double Base_Length;

  //Wings
  G4double Wing_Radius;
  G4double Wing_Length;
  G4double Wing_Thickness;

  //Target Ladder Holes
  G4double THole_Radius;
  G4double THole_Length;
  
  G4double TPole_Radius;
  G4double TPole_Length;

  //Ladder Tube
  G4double LTube_Radius;
  G4double LTube_Length;
  G4double LTube_Thickness;

  //Guage Supports
  G4double Horizon_Radius;
  G4double Horizon_Length;
  
  G4double Meridian_Radius;
  G4double Meridian_Length;
  G4double Down_Radius;
  G4double Down_Length;

  G4double KF25C_Radius;
  G4double KF25C_Length;
  G4double KF25C_Thickness;

  G4double Window_Radius;
  G4double Window_Length;

 


  G4Material* Al;
  G4Material* Steel = G4NistManager::Instance()->FindOrBuildMaterial("G4_STAINLESS-STEEL");
  G4Material* quartz;
  G4Material* plastic;

  G4double startAngle; 
  G4double spanningAngle; 

  G4ThreeVector Pos; //(sphere)

  G4ThreeVector Hemi_Pos;
  //G4RotationMatrix Hemi_Rot;
  
  G4ThreeVector Mid_Shift;
  //G4RotationMatrix Mid_Rot;
  
  G4ThreeVector Ring_Shift;
 // G4RotationMatrix Ring_Rot;

  G4ThreeVector End_Shift;
  G4RotationMatrix Rot;

  G4ThreeVector BPShift;
  G4RotationMatrix BPRot;

  G4ThreeVector Main_Shift;
  G4ThreeVector Box_Shift;

  G4ThreeVector KF50_Shift;
  G4RotationMatrix KF50_Rot;
  G4RotationMatrix FDrill_Rot;

  G4ThreeVector FDrill_Shift;

  G4ThreeVector Window_Shift;

  G4ThreeVector LKF50_Shift;
  G4RotationMatrix LKF50_Rot;

  G4ThreeVector GateValve_Shift;
  G4ThreeVector ValveCap_Shift;
  G4RotationMatrix GateValve_Rot;
  G4RotationMatrix ValveCap_Rot;
  G4RotationMatrix GDrill_Rot;
  G4ThreeVector GDrill_Shift;
  G4ThreeVector GKF50_Shift;
  G4ThreeVector LTube_Shift;

  G4ThreeVector Clamp_Shift;
  G4RotationMatrix Clamp_Rot;
  G4ThreeVector RCBox_Shift;
  G4ThreeVector LCBox_Shift;
  G4RotationMatrix CBox_Rot;

  G4ThreeVector BPClamp_Shift;
  G4ThreeVector R40CBox_Shift;
  G4ThreeVector L40CBox_Shift;

  G4ThreeVector Support_Shift;

  G4ThreeVector Flat_Shift;

  G4ThreeVector Base_Shift;

  G4ThreeVector LBKF50_Shift;
  G4RotationMatrix LBKF50_Rot;

  G4ThreeVector Wing_Shift;
  G4RotationMatrix LeftWing_Rot;
  G4RotationMatrix RightWing_Rot;

  //Target Ladder
  G4ThreeVector TLadder_Shift;
  G4ThreeVector TPole_Shift;
  G4ThreeVector TLH1_Shift;
  G4ThreeVector TLH2_Shift;
  G4ThreeVector TLH3_Shift;
  G4ThreeVector TLH4_Shift;
  G4ThreeVector TLH5_Shift;

  //Detector Mounts
  G4ThreeVector L90_Shift;
  G4ThreeVector R90_Shift;
  G4ThreeVector L270_Shift;
  G4ThreeVector R270_Shift;
  G4RotationMatrix M270_Rot;
  G4ThreeVector R220_Shift;
  G4ThreeVector L220_Shift;
  G4RotationMatrix M220_Rot;
  G4ThreeVector R142_Shift;
  G4ThreeVector L142_Shift;
  G4RotationMatrix M142_Rot;

  //Cross Gauage
  G4RotationMatrix Horizon_Rot;
  G4ThreeVector Horizon_Shift;
  G4ThreeVector Meridian_Shift;
  G4ThreeVector Down_Shift;

  G4ThreeVector RKF25C_Shift;
  G4ThreeVector LMKF25C_Shift;
  G4ThreeVector TKF25C_Shift;
  G4ThreeVector BUKF25C_Shift;
  G4ThreeVector BLKF25C_Shift;
  G4ThreeVector LDKF25C_Shift;

  //Cradles
  G4RotationMatrix CradleHole_Rot;
  G4RotationMatrix Cradle0_Rot;
  G4ThreeVector Cradle0_Shift;
  G4ThreeVector Cradle1_Shift;
  G4ThreeVector Cradle2_Shift;
  G4ThreeVector Cradle3_Shift;
  G4ThreeVector Cradle4_Shift;
  G4ThreeVector Cradle5_Shift;
  G4ThreeVector Cradle6_Shift;
  G4ThreeVector Cradle7_Shift;
  G4ThreeVector Cradle8_Shift;


  G4LogicalVolume*  Hemi_log;
  G4VPhysicalVolume* Hemi_phys;
  
  G4LogicalVolume*  Mid_log;
  G4VPhysicalVolume* Mid_phys;
  
  G4LogicalVolume*  Ring_log;
  G4VPhysicalVolume* Ring_phys;

  G4LogicalVolume*  End_log;
  G4VPhysicalVolume* End_phys;

  //G4LogicalVolume*  chamber_log;
  //G4VPhysicalVolume* chamber_phys;
  
  G4LogicalVolume*  pipe_log;
  G4VPhysicalVolume* pipe_phys;

  G4LogicalVolume*  Main_log;
  G4VPhysicalVolume* Box_phys;

  G4LogicalVolume*  Bowl_log;
  G4VPhysicalVolume* Bowl_phys;

  G4LogicalVolume* KF50_log;
  G4VPhysicalVolume* KF50_phys;
  G4LogicalVolume* longKF50_log;
  G4VPhysicalVolume* LKF50_phys;
  G4LogicalVolume* shortKF50_log;

  G4LogicalVolume* Window_log;
  G4VPhysicalVolume* Window_phys;

  G4LogicalVolume* Both_log;
  G4VPhysicalVolume* Both_phys;

  G4LogicalVolume* GateValve_log;
  G4VPhysicalVolume* GateValve_phys;

  G4LogicalVolume* Clamp_log;
  G4VPhysicalVolume* Clamp_phys;

  G4LogicalVolume* Clamp40_log;
  G4VPhysicalVolume* BPClamp_phys;

  G4LogicalVolume* LTube_log;
  G4VPhysicalVolume* LTube_phys;

  G4LogicalVolume* Support_log;
  G4VPhysicalVolume* Support_phys;

  G4LogicalVolume* Flat_log;
  G4VPhysicalVolume* Flat_phys;

  G4LogicalVolume* Base_log;
  G4VPhysicalVolume* Base_phys;

  G4VPhysicalVolume* LBKF50_phys;

  G4LogicalVolume* Wing_log;
  G4VPhysicalVolume* RightWing_phys;
  G4VPhysicalVolume* LeftWing_phys;

  G4LogicalVolume* TLadder_log;
  G4VPhysicalVolume* TLadder_phys;
  G4LogicalVolume* TPole_log;
  G4VPhysicalVolume* TPole_phys;
  G4AssemblyVolume* TLadderassembly;
  G4LogicalVolume* TLadderEnvelope_log;
  G4VPhysicalVolume* TLadderEnvelope_phys;

  G4LogicalVolume* Mount_log;
  G4VPhysicalVolume* L90_phys;
  G4VPhysicalVolume* R90_phys;
  G4VPhysicalVolume* L270_phys;
  G4VPhysicalVolume* R270_phys;
  G4VPhysicalVolume* L220_phys;
  G4VPhysicalVolume* R220_phys;
  G4VPhysicalVolume* L142_phys;
  G4VPhysicalVolume* R142_phys;

  G4LogicalVolume* Horizon_log;
  G4VPhysicalVolume* Horizon_phys;
  G4LogicalVolume* Meridian_log;
  G4VPhysicalVolume* Meridian_phys;
  G4LogicalVolume* Down_log;
  G4VPhysicalVolume* Down_phys;

  G4LogicalVolume* KF25C_log;
  G4VPhysicalVolume* RKF25C_phys;
  G4VPhysicalVolume* LMKF25C_phys;
  G4VPhysicalVolume* TKF25C_phys;
  G4VPhysicalVolume* BUKF25C_phys;
  G4VPhysicalVolume* BLKF25C_phys;
  G4VPhysicalVolume* LDKF25C_phys;

  G4LogicalVolume* LargeCradle_log;
  G4LogicalVolume* SmallCradle_log;
  G4VPhysicalVolume* Cradle0_phys;
  G4VPhysicalVolume* Cradle1_phys;
  G4VPhysicalVolume* Cradle2_phys;
  G4VPhysicalVolume* Cradle3_phys;
  G4VPhysicalVolume* Cradle4_phys;
  G4VPhysicalVolume* Cradle5_phys;
  G4VPhysicalVolume* Cradle6_phys;
  G4VPhysicalVolume* Cradle7_phys;
  G4VPhysicalVolume* Cradle8_phys;
};

#endif
