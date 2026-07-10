/* H. J. Ross 7/10/2026
   This model includes all peices of the CeBrA array that we deemed 
   necessary to simulate as of Summer 2026. We have determined
   that the gas handling cross connected to the target ladder
   extension above the gate valve can safely be omitted.
   The code for it is still here if that changes. In order to get rid
   of a peice of the model, comment out its G4PVPlacement() call. Some
   of the peices here are rudementary and do not fully represent what
   the dead material is in real life but we have determined that the
   level of precision here is adequate. */ 

#include "Target_Chamber.hh"

Target_Chamber::Target_Chamber(G4LogicalVolume* experimentalHall_log,
			       Materials* mat)
{
  materials = mat;
  expHall_log=experimentalHall_log;

  Al = materials->FindMaterial("Al");


  //Spherical Target Chamber
  /*Radius    = 4.05*2.54*cm;
  Thickness = 1.75*mm;*/

  //Beam Pipe
  BPRadius = 1 * 2.54 * cm;
  BPThickness = .2*2.54*cm;
  BPLength = 4.035 * 2.54 * cm;

  //Hemispherical Target Chamber
  Hemi_Radius    = 4.0*2.54*cm;
  Hemi_Thickness = .2*2.54*cm;

  //Cylinder on target chamber
  Mid_Radius = 4.05*2.54*cm;
  Mid_Thickness = .2*2.54*cm;
  Mid_Length = 1 * 2.54 *cm + 2*mm;

  //Wider ring attaced to mid
  Ring_Radius = 5*2.54*cm;
  Ring_Thickness = 1*2.54 * cm;
  Ring_Length = .25*2.54 * cm;

  //End peice of chamber
  End_Radius = 5*2.54*cm;
  End_Thickness = 2 * cm;
  End_Length = .25 *2.54* cm;

  //KF50 Flange
  fZPlane[0] = 0.00 * cm;   fZPlane[1] = 0.254 * cm;  fZPlane[2] = 0.3048 * cm; fZPlane[3] = 0.508 * cm;  fZPlane[4] = 1.905 * cm;
  fRInner[0] = 2.6289 * cm; fRInner[1] = 2.6289 * cm; fRInner[2] = 2.3114 * cm; fRInner[3] = 2.3114 * cm; fRInner[4] = 2.3114 * cm;
  fROuter[0] = 3.7465 * cm; fROuter[1] = 3.7465 * cm; fROuter[2] = 3.7465 * cm; fROuter[3] = 2.6289 * cm; fROuter[4] = 2.6289 * cm;

  //KF50 Flange Hole
  FDrill_Radius = 2.6289 * cm;
  FDrill_Length = 15 * cm; 
  FDrill_Thickness = .2*2.54*cm;

  //Gate Valve
  GateValve_Length = 68.85*mm;
  GateValve_Width = 37.6*mm;
  GateValve_Depth = 13.1*mm;
  
  //Valve Cap
  ValveCap_Length = 48.4*mm;
  ValveCap_Width = 25.35*mm;
  ValveCap_Depth = 5*mm;

  //KF50 Clamp
  CPlane[0] = 0.00 * mm;   CPlane[1] = 5.5 * mm; CPlane[2] = 5.5 * mm; CPlane[3] = 16.5 * mm; CPlane[4] = 16.5 * mm; CPlane[5] = 22 * mm;
  CInner[0] = 35 * mm; CInner[1] = 35 * mm; CInner[2] = 40 * mm; CInner[3] = 40 * mm; CInner[4] = 35 * mm; CInner[5] = 35 * mm;
  COuter[0] = 46 * mm; COuter[1] = 46 * mm; COuter[2] = 46 * mm; COuter[3] = 46 * mm; COuter[4] = 46 * mm; COuter[5] = 46 * mm;

  //Peices attached to clamps
  CBox_Length = 15.5 * mm;
  CBox_Width = 7.75 * mm;
  CBox_Depth = 7.75 * mm;

  //KF40 Clamp on Beam Pipe
  C40Plane[0] = 0.00 * mm; C40Plane[1] = 5.5 * mm; C40Plane[2] = 5.5 * mm; C40Plane[3] = 16.5 * mm; C40Plane[4] = 16.5 * mm; C40Plane[5] = 22 * mm;
  C40Inner[0] = 26 * mm; C40Inner[1] = 26 * mm; C40Inner[2] = 31 * mm; C40Inner[3] = 31 * mm; C40Inner[4] = 26 * mm; C40Inner[5] = 26 * mm;
  C40Outer[0] = 37 * mm; C40Outer[1] = 37 * mm; C40Outer[2] = 37 * mm; C40Outer[3] = 37 * mm; C40Outer[4] = 37 * mm; C40Outer[5] = 37 * mm;

  //Support Tube
  Support_Radius = 2.6289 * cm;
  Support_Length = 2.935 * 2.54 *cm;

  //Piece between support tube and base
  Flat_Radius = 5.1689 * cm;
  Flat_Length = .255*2.54*cm;

  //Base
  Base_Radius = 10*2.54*cm;
  Base_Length = .315*2.54*cm;

  //Wings
  Wing_Radius = 10*2.54*cm;
  Wing_Length = .19*2.54*cm;
  Wing_Thickness = 4.5*2.54*cm;

  //Ladder Tube
  LTube_Radius = 2.6289 * cm;
  LTube_Length = 4.5*2.54*cm;
  LTube_Thickness = .2*2.54*cm;

  //Guage Supports
  Horizon_Radius = .375*2.54*cm;
  Horizon_Length = 4.5*2.54*cm;

  Meridian_Radius = .375*2.54*cm;
  Meridian_Length = 4*2.54*cm;

  Down_Radius = .375*2.54*cm;
  Down_Length = 1*2.54*cm;

  //Solid KF25 Clamps (uniform diameter, no actual KF flange)
  KF25C_Radius = .875*2.54*cm;
  KF25C_Length = 7*mm;
  KF25C_Thickness = .5*2.54*cm;  

  //Sphere
  /*Pos.setX(0);
  Pos.setY(0);
  Pos.setZ(0);*/

  BPShift.setX(0);
  BPShift.setY(0);
  BPShift.setZ(-8.035*2.54*cm);

  Hemi_Pos.setX(0);
  Hemi_Pos.setY(0);
  Hemi_Pos.setZ(0);

  Mid_Shift.setX(0);
  Mid_Shift.setY(0);
  Mid_Shift.setZ(Mid_Length - 4 * mm);

  Ring_Shift.setX(0);
  Ring_Shift.setY(0);
  Ring_Shift.setZ(5.6*cm);

  End_Shift.setX(0);
  End_Shift.setY(0);
  End_Shift.setZ(6.7*cm);

  //Flange on top of chamber
  KF50_Shift.setX(0);
  KF50_Shift.setY(12.327*cm);
  KF50_Shift.setZ(0);

  //Flange for glass port
  LKF50_Shift.setX(14.327*cm);
  LKF50_Shift.setY(0);
  LKF50_Shift.setZ(0*cm);

  //Opp Flange Glass Port
  LBKF50_Shift.setX(14.7795*cm);
  LBKF50_Shift.setZ(0*cm);

  //Hole on top of chamber
  FDrill_Shift.setX(0);
  FDrill_Shift.setY(10.922*cm);
  FDrill_Shift.setZ(0);

  //Gate Valve
  GateValve_Shift.setY(15.8*cm);//13.462*cm
  GateValve_Shift.setZ(32.0*mm);//34.425*mm
  ValveCap_Shift.setX(-68.6*mm);//-34.3*mm
  GDrill_Shift.setX(33.2*mm);
  GKF50_Shift.setX(33.2*mm);
  GKF50_Shift.setZ(32.8*mm);

  //Clamp for gate valve
  Clamp_Shift.setX(0);
  Clamp_Shift.setY(13.5*cm);
  Clamp_Shift.setZ(0);

  RCBox_Shift.setX(54*mm);
  RCBox_Shift.setZ(11*mm);
  LCBox_Shift.setX(-54*mm);
  LCBox_Shift.setZ(11*mm);

  //Clamp for Beam Pipe
  BPClamp_Shift.setZ(-8.035*2.54*cm - 11*mm);
  
  R40CBox_Shift.setX(45*mm);
  R40CBox_Shift.setZ(11*mm);
  L40CBox_Shift.setX(-45*mm);
  L40CBox_Shift.setZ(11*mm);

  //Ladder Tube
  LTube_Shift.setY(28.18*cm);

  //Support Tube
  Support_Shift.setY(-6.935*2.54*cm);

  //Piece between support tube and base
  Flat_Shift.setY(-10.125*2.54*cm);

  //Base
  Base_Shift.setY(-10.44*2.54*cm);

  //Wings
  Wing_Shift.setY(-2.41*2.54*cm);

  //Mounts
  L90_Shift.setX(22*cm);
  L90_Shift.setY(-5.7*cm);
  L90_Shift.setZ(4.5*cm);

  R90_Shift.setX(22*cm);
  R90_Shift.setY(-5.7*cm);
  R90_Shift.setZ(-4.28*cm);

  L270_Shift.setX(-22*cm);
  L270_Shift.setY(-5.7*cm);
  L270_Shift.setZ(4.5*cm);

  R270_Shift.setX(-22*cm);
  R270_Shift.setY(-5.7*cm);
  R270_Shift.setZ(-4.28*cm);

  L220_Shift.setX(17.34*cm);
  L220_Shift.setY(-5.7*cm);
  L220_Shift.setZ(-13.54*cm);

  R220_Shift.setX(10.32*cm);
  R220_Shift.setY(-5.7*cm);
  R220_Shift.setZ(-19.42*cm);

  L142_Shift.setX(-9.892*cm);
  L142_Shift.setY(-5.7*cm);
  L142_Shift.setZ(-20.282*cm);

  R142_Shift.setX(-16.85*cm);
  R142_Shift.setY(-5.7*cm);
  R142_Shift.setZ(-14.14*cm);

  //Guage Supports
  Horizon_Shift.setX(10.770 * cm);
  Horizon_Shift.setY(36.308*cm);
  Horizon_Shift.setZ(-9.037*cm);

  Meridian_Shift.setX(7.851*cm);
  Meridian_Shift.setY(35.038*cm);
  Meridian_Shift.setZ(-6.588*cm);

  Down_Shift.setX(14.661*cm);
  Down_Shift.setY(33.133*cm);
  Down_Shift.setZ(-12.302*cm);

  //Cross Gauge Clamps (directions taken looking at the beam pipe side)
  //Right
  RKF25C_Shift.setX(3.96*cm);
  RKF25C_Shift.setY(36.308*cm);
  RKF25C_Shift.setZ(-3.323*cm);

  //Left Middle
  LMKF25C_Shift.setX(11.743*cm);
  LMKF25C_Shift.setY(36.308*cm);
  LMKF25C_Shift.setZ(-9.853*cm);

  //Top
  TKF25C_Shift.setX(7.851*cm);
  TKF25C_Shift.setY(40.88*cm);
  TKF25C_Shift.setZ(-6.588*cm);

  //Very Bottom (Bottom Lower)
  BLKF25C_Shift.setX(7.851*cm);
  BLKF25C_Shift.setY(27.926*cm);
  BLKF25C_Shift.setZ(-6.588*cm);

  //Bottom Upper
  BUKF25C_Shift.setX(7.851*cm);
  BUKF25C_Shift.setY(31.736*cm);
  BUKF25C_Shift.setZ(-6.588*cm);

  //Left Down
  LDKF25C_Shift.setX(14.661*cm);
  LDKF25C_Shift.setY(31.736*cm);
  LDKF25C_Shift.setZ(-12.302*cm);



  Rot = G4RotationMatrix::IDENTITY;

  KF50_Rot = G4RotationMatrix::IDENTITY;
  KF50_Rot.rotateX(90*deg);

  FDrill_Rot = G4RotationMatrix::IDENTITY;
  FDrill_Rot.rotateX(90.01*deg); //90.01 instead of 90 so the drill shows across the union

  LKF50_Rot = G4RotationMatrix::IDENTITY;
  LKF50_Rot.rotateY(-90.01*deg);

  //Gate Valve
  GDrill_Rot = G4RotationMatrix::IDENTITY;
  GDrill_Rot.rotateZ(90*deg);
  GDrill_Rot.rotateX(180*deg);

  GateValve_Rot = G4RotationMatrix::IDENTITY;
  GateValve_Rot.rotateX(90*deg);
  GateValve_Rot.rotateY(90*deg);
  
  ValveCap_Rot = G4RotationMatrix::IDENTITY;
  ValveCap_Rot.rotateX(90*deg);
  //ValveCap_Rot.rotateY(90*deg);
  ValveCap_Rot.rotateZ(90*deg);

  //Clamp
  Clamp_Rot = G4RotationMatrix::IDENTITY;
  Clamp_Rot.rotateX(90*deg);
  Clamp_Rot.rotateY(38*deg);

  CBox_Rot = G4RotationMatrix::IDENTITY;
  CBox_Rot.rotateX(0);
  CBox_Rot.rotateZ(90*deg);

  //Opp Flange Glass Port
  LBKF50_Rot = G4RotationMatrix::IDENTITY;
  LBKF50_Rot.rotateY(-270*deg);

  //Wings
  LeftWing_Rot = G4RotationMatrix::IDENTITY;
  LeftWing_Rot.rotateX(90*deg);
  LeftWing_Rot.rotateY(90*deg);
  
  RightWing_Rot = G4RotationMatrix::IDENTITY;
  RightWing_Rot.rotateX(270*deg);
  RightWing_Rot.rotateY(90*deg); 

  //Detector Mounts
  M270_Rot = G4RotationMatrix::IDENTITY;
  M270_Rot.rotateY(180*deg);

  M220_Rot = G4RotationMatrix::IDENTITY;
  M220_Rot.rotateY(-310*deg);

  M142_Rot = G4RotationMatrix::IDENTITY;
  M142_Rot.rotateY(-232*deg);

  //Cross Guage
  Horizon_Rot = G4RotationMatrix::IDENTITY;
  Horizon_Rot.rotateY(130*deg);



}


Target_Chamber::~Target_Chamber()
{  
}

void Target_Chamber::Construct()
{
  /*G4Sphere* sphere = new G4Sphere("SphericalShell", Radius-Thickness, Radius,
				  0, 360.0*deg, 0, 180.0*deg);
  chamber_log = new G4LogicalVolume(sphere, Al, "chamber_log");
  
  chamber_phys = new G4PVPlacement(G4Transform3D(Rot, Pos),
                                   chamber_log, "Chamber",
                                   expHall_log, false, 0);*/


  G4Tubs* cylinder = new G4Tubs("HollowCylinder", BPRadius-BPThickness, BPRadius, BPLength, 0*deg, 360*deg);

  pipe_log = new G4LogicalVolume(cylinder, Al, "pipe_log");

  pipe_phys = new G4PVPlacement(G4Transform3D(Rot, BPShift),
                                pipe_log, "Pipe", 
                                expHall_log, false, 0);


  G4Sphere* sphere = new G4Sphere("HemiShell", Hemi_Radius-Hemi_Thickness, Hemi_Radius,
				  0*deg, 360.0*deg, 90*deg, 90*deg);
  /*Hemi_log = new G4LogicalVolume(sphere, Al, "Hemi_log");
  
  Hemi_phys = new G4PVPlacement(G4Transform3D(Rot, Hemi_Pos),
                                   Hemi_log, "Hemi",
                                   expHall_log, false, 0);*/


  G4Tubs* cylinderMid = new G4Tubs("Mid", Mid_Radius-Mid_Thickness, Mid_Radius, Mid_Length, 0*deg, 360*deg);

  /*Mid_log = new G4LogicalVolume(cylinderMid, Al, "Mid_log");

  Mid_phys = new G4PVPlacement(G4Transform3D(Rot, Mid_Shift),
                                Mid_log, "Mid", 
                                expHall_log, false, 0);*/

  G4UnionSolid* Bowl = new G4UnionSolid("Bowl", sphere, cylinderMid, G4Transform3D(Rot, Mid_Shift));

  /*Bowl_log = new G4LogicalVolume(Bowl, Al, "Bowl_log");

  Bowl_phys = new G4PVPlacement(G4Transform3D(Rot, Hemi_Pos), Bowl_log, "Bowl", expHall_log, false, 0);*/

  G4Tubs* FDrill = new G4Tubs("FDrill", 0, FDrill_Radius, FDrill_Length, 0*deg, 360*deg);
  //FDrill_log = new G4LogicalVolume(FDrill, Steel, "FDrill_log");
  G4Tubs* LTube = new G4Tubs("LTube", LTube_Radius-LTube_Thickness, LTube_Radius, LTube_Length, 0*deg, 360*deg);
  LTube_log = new G4LogicalVolume(LTube, Steel, "LTube_log");
  LTube_phys = new G4PVPlacement(G4Transform3D(FDrill_Rot, LTube_Shift), LTube_log, "LTube", expHall_log, false, 0);


  G4SubtractionSolid* Top = new G4SubtractionSolid("Top", Bowl, FDrill, G4Transform3D(FDrill_Rot, FDrill_Shift));

  G4SubtractionSolid* Both = new G4SubtractionSolid("Both", Top, FDrill, G4Transform3D(LKF50_Rot, LKF50_Shift));

  Both_log = new G4LogicalVolume(Both, Al, "Both_log");

  Both_phys = new G4PVPlacement(G4Transform3D(Rot, Hemi_Pos), Both_log, "Both", expHall_log, false, 0);


  G4Tubs* cylinderRing = new G4Tubs("Ring", Ring_Radius-Ring_Thickness, Ring_Radius, Ring_Length, 0*deg, 360*deg);

  Ring_log = new G4LogicalVolume(cylinderRing, Al, "Ring_log");

  Ring_phys = new G4PVPlacement(G4Transform3D(Rot, Ring_Shift),
                                Ring_log, "Ring", 
                                expHall_log, false, 0);


  G4Tubs* cylinderEnd = new G4Tubs("End", 0*mm, End_Radius, End_Length, 0*deg, 360*deg);

  End_log = new G4LogicalVolume(cylinderEnd, Al, "End_log");

  End_phys = new G4PVPlacement(G4Transform3D(Rot, End_Shift),
                                End_log, "End", 
                                expHall_log, false, 0);

  
  //KF50 Flanges
  G4Polycone* solidKF50 = new G4Polycone("KF50", 0.0*deg, 360.0*deg, fNumZPlanes, fZPlane, fRInner, fROuter);

  KF50_log = new G4LogicalVolume(solidKF50, Steel, "KF50_log");

  //Flange on top of chamber
  KF50_phys = new G4PVPlacement(G4Transform3D(KF50_Rot, KF50_Shift),
                                  KF50_log, "KF50",
                                  expHall_log, false, 0);

  //Glass Port Flanges                                
  LKF50_phys = new G4PVPlacement(G4Transform3D(LKF50_Rot, LKF50_Shift), KF50_log, "LKF50", expHall_log, false, 0);
  LBKF50_phys = new G4PVPlacement(G4Transform3D(LBKF50_Rot, LBKF50_Shift), KF50_log, "LBKF50", expHall_log, false, 0);
  
  //Gate Valve
  G4Box* GateBox = new G4Box("GateBox", GateValve_Length, GateValve_Width, GateValve_Depth);
  G4Box* ValveCap = new G4Box("GateCap", ValveCap_Length, ValveCap_Width, ValveCap_Depth);
  G4UnionSolid* T = new G4UnionSolid("T", GateBox, ValveCap, G4Transform3D(ValveCap_Rot, ValveCap_Shift));
  G4SubtractionSolid* KFT = new G4SubtractionSolid("KFT", T, FDrill, G4Transform3D(GDrill_Rot, GDrill_Shift));
  G4UnionSolid* GateValve = new G4UnionSolid("GateValve", KFT, solidKF50, G4Transform3D(GDrill_Rot, GKF50_Shift));

  GateValve_log = new G4LogicalVolume(GateValve, Steel, "GateValve_log");

  GateValve_phys = new G4PVPlacement(G4Transform3D(GateValve_Rot, GateValve_Shift), GateValve_log, "GateValve", expHall_log, false, 0);

  //Clamp on Gate Valve
  G4Polycone* CRing = new G4Polycone("CRing", 0.0*deg, 360.0*deg, NumCPlanes, CPlane, CInner, COuter);
  G4Box* CBox = new G4Box("CBox", CBox_Length, CBox_Width, CBox_Depth);
  G4UnionSolid* Left = new G4UnionSolid("Left", CRing, CBox, G4Transform3D(CBox_Rot, LCBox_Shift));
  G4UnionSolid* Clamp = new G4UnionSolid("Left", Left, CBox, G4Transform3D(CBox_Rot, RCBox_Shift));

  Clamp_log = new G4LogicalVolume(Clamp, Al, "Clamp_log");

  Clamp_phys = new G4PVPlacement(G4Transform3D(Clamp_Rot, Clamp_Shift), Clamp_log, "Clamp", expHall_log, false, 0);

  //Clamp on Beam Pipe
  G4Polycone* C40Ring = new G4Polycone("C40Ring", 0.0*deg, 360.0*deg, NumCPlanes, C40Plane, C40Inner, C40Outer);
  G4UnionSolid* Left40 = new G4UnionSolid("Left40", C40Ring, CBox, G4Transform3D(CBox_Rot, L40CBox_Shift));
  G4UnionSolid* Clamp40 = new G4UnionSolid("Clamp40", Left40, CBox, G4Transform3D(CBox_Rot, R40CBox_Shift));

  Clamp40_log = new G4LogicalVolume(Clamp40, Al, "Clamp40_log");

  BPClamp_phys = new G4PVPlacement(G4Transform3D(Rot, BPClamp_Shift), Clamp40_log, "BPClamp", expHall_log, false, 0);

  //Support Pipe
  G4Tubs* Support = new G4Tubs("Support", 0, Support_Radius, Support_Length, 0*deg, 360*deg);

  Support_log = new G4LogicalVolume(Support, Steel, "Support_log");

  Support_phys = new G4PVPlacement(G4Transform3D(FDrill_Rot, Support_Shift), Support_log, "Support", expHall_log, false, 0);

  //Small Peice Above Base
  G4Tubs* Flat = new G4Tubs("Flat", 0, Flat_Radius, Flat_Length, 0*deg, 360*deg);

  Flat_log = new G4LogicalVolume(Flat, Steel, "Flat_log");

  Flat_phys = new G4PVPlacement(G4Transform3D(FDrill_Rot, Flat_Shift), Flat_log, "Flat", expHall_log, false, 0);

  //Base
  G4Tubs* Base = new G4Tubs("Base", 0, Base_Radius, Base_Length, 0*deg, 360*deg);

  Base_log = new G4LogicalVolume(Base, Steel, "Base_log");

  Base_phys = new G4PVPlacement(G4Transform3D(FDrill_Rot, Base_Shift), Base_log, "Base", expHall_log, false, 0);

  //Wings
  G4Tubs* Wing = new G4Tubs("Wing", Wing_Radius-Wing_Thickness, Wing_Radius, Wing_Length, 23.97*deg, 67.5*deg);

  Wing_log = new G4LogicalVolume(Wing, Steel, "Wing_log");

  LeftWing_phys = new G4PVPlacement(G4Transform3D(LeftWing_Rot, Wing_Shift), Wing_log, "LeftWing", expHall_log, false, 0);
  RightWing_phys = new G4PVPlacement(G4Transform3D(RightWing_Rot, Wing_Shift), Wing_log, "RightWing", expHall_log, false, 0);


  //Vertical Detector Mounts
  std::vector<G4TwoVector> polygon;
  polygon.push_back(G4TwoVector( 0.00 * 2.54 * cm,  0.00 * 2.54 * cm)); // V1: Bottom-Right
  polygon.push_back(G4TwoVector( 0.00 * 2.54 * cm,  5.26 * 2.54 * cm)); // V2: Bottom-Left
  polygon.push_back(G4TwoVector(-2.31 * 2.54 * cm,  7.57 * 2.54 * cm)); // V3: Inner bend
  polygon.push_back(G4TwoVector(-3.37 * 2.54 * cm,  6.51 * 2.54 * cm)); // V4: Top-Left corner
  polygon.push_back(G4TwoVector(-1.50 * 2.54 * cm,  4.64 * 2.54 * cm)); // V5: Top-Right corner
  polygon.push_back(G4TwoVector(-1.50 * 2.54 * cm,  0.00 * 2.54 * cm)); // V6: Outer bend corner

  std::vector<G4ExtrudedSolid::ZSection> zSections;
  zSections.push_back(G4ExtrudedSolid::ZSection(-2.5 * mm, G4TwoVector(0,0), 1.0));
  zSections.push_back(G4ExtrudedSolid::ZSection( 2.5 * mm, G4TwoVector(0,0), 1.0));

  G4ExtrudedSolid* rawPlate = new G4ExtrudedSolid("RawPlate", polygon, zSections);

  Mount_log = new G4LogicalVolume(rawPlate, Steel, "Mount_log");

  L90_phys = new G4PVPlacement(G4Transform3D(Rot, L90_Shift), Mount_log, "L90", expHall_log, false, 0);
  R90_phys = new G4PVPlacement(G4Transform3D(Rot, R90_Shift), Mount_log, "R90", expHall_log, false, 0);
  L270_phys = new G4PVPlacement(G4Transform3D(M270_Rot, L270_Shift), Mount_log, "L270", expHall_log, false, 0);
  R270_phys = new G4PVPlacement(G4Transform3D(M270_Rot, R270_Shift), Mount_log, "R270", expHall_log, false, 0);
  L220_phys = new G4PVPlacement(G4Transform3D(M220_Rot, L220_Shift), Mount_log, "L220", expHall_log, false, 0);
  R220_phys = new G4PVPlacement(G4Transform3D(M220_Rot, R220_Shift), Mount_log, "R220", expHall_log, false, 0);
  L142_phys = new G4PVPlacement(G4Transform3D(M142_Rot, L142_Shift), Mount_log, "L142", expHall_log, false, 0);
  R142_phys = new G4PVPlacement(G4Transform3D(M142_Rot, R142_Shift), Mount_log, "R142", expHall_log, false, 0);

  //Guage Supports
  /*G4Tubs* Horizon = new G4Tubs("Horizon", 0, Horizon_Radius, Horizon_Length, 0*deg, 360*deg);
  Horizon_log = new G4LogicalVolume(Horizon, Steel, "Horizon_log");
  Horizon_phys = new G4PVPlacement(G4Transform3D(Horizon_Rot, Horizon_Shift), Horizon_log, "Horizon", expHall_log, false, 0);

  G4Tubs* Meridian = new G4Tubs("Meridian", 0, Meridian_Radius, Meridian_Length, 0*deg, 360*deg);
  Meridian_log = new G4LogicalVolume(Meridian, Steel, "Meridian_log");
  Meridian_phys = new G4PVPlacement(G4Transform3D(Clamp_Rot, Meridian_Shift), Meridian_log, "Meridian", expHall_log, false, 0);

  G4Tubs* Down = new G4Tubs("Down", 0, Down_Radius, Down_Length, 0*deg, 360*deg);
  Down_log = new G4LogicalVolume(Down, Steel, "Down_log");
  Down_phys = new G4PVPlacement(G4Transform3D(Clamp_Rot, Down_Shift), Down_log, "Down", expHall_log, false, 0);

  G4Tubs* KF25C = new G4Tubs("KF25C", KF25C_Radius-KF25C_Thickness, KF25C_Radius, KF25C_Length, 0*deg, 360*deg);
  KF25C_log = new G4LogicalVolume(KF25C, Al, "KF25C_log");
  RKF25C_phys = new G4PVPlacement(G4Transform3D(Horizon_Rot, RKF25C_Shift), KF25C_log, "RKF25C", expHall_log, false, 0);
  LMKF25C_phys = new G4PVPlacement(G4Transform3D(Horizon_Rot, LMKF25C_Shift), KF25C_log, "LMKF25C", expHall_log, false, 0); 
  TKF25C_phys = new G4PVPlacement(G4Transform3D(Clamp_Rot, TKF25C_Shift), KF25C_log, "TKF25C", expHall_log, false, 0); 
  BUKF25C_phys = new G4PVPlacement(G4Transform3D(Clamp_Rot, BUKF25C_Shift), KF25C_log, "BUKF25C", expHall_log, false, 0); 
  BLKF25C_phys = new G4PVPlacement(G4Transform3D(Clamp_Rot, BLKF25C_Shift), KF25C_log, "BLKF25C", expHall_log, false, 0); 
  LDKF25C_phys = new G4PVPlacement(G4Transform3D(Clamp_Rot, LDKF25C_Shift), KF25C_log, "LDKF25C", expHall_log, false, 0); */




  

  G4Colour dGrey (0.8, 0.8, 0.8, 1.0);
  G4VisAttributes* Vis = new G4VisAttributes(dGrey);
  Vis->SetVisibility(true);
  Vis->SetForceSolid(true);

  //Hemi_log->SetVisAttributes(Vis);
  //Mid_log->SetVisAttributes(Vis);
  //Ring_log->SetVisAttributes(Vis);
  //End_log->SetVisAttributes(Vis);
  //chamber_log->SetVisAttributes(Vis);
  //Bowl_log->SetVisAttributes(Vis);
  //Both_log->SetVisAttributes(Vis);

  return; 
}
