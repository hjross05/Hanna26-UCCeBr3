#include "CeBr3_3x3_Detector.hh"

CeBr3_3x3_Detector::CeBr3_3x3_Detector(G4LogicalVolume* experimentalHall_log,
				       Materials* mat)
{
  materials=mat;
  expHall_log=experimentalHall_log;

  CeBr3 = materials->FindMaterial("CeBr3");
  Al = materials->FindMaterial("Al");
  MuMetal = materials->FindMaterial("MuMetal");
  //pmtMat = materials->FindMaterial("pmtMat");
  quartz = materials->FindMaterial("quartz");
  TeflonTape = materials->FindMaterial("TeflonTape");
  
  crystalLength = 76*mm;       // 3 inches
  crystalRadius = 76/2.0*mm;   // 1.5 inch

  startAngle      = 45.*deg;
  spanningAngle   = 360.*deg;

  canLength    = 79*mm; // 104 mm for 3"x4" 154 mm for 3"x6"
  canThickness = 1.0*mm;
  canRadius    = 82.8/2*mm;

  reflectorSideThickness  = 2.3*mm;
  reflectorFrontThickness = 1.0*mm;
  
  // This is the position of the center of the front face of the can.
  DetPos.setX(0);
  DetPos.setY(0);
  DetPos.setZ(0);

  crystalShift.setX(0);
  crystalShift.setY(0);
  crystalShift.setZ(crystalLength/2 + 2*canThickness);
  crystalPos = DetPos + crystalShift;

  reflectorShift.setX(0);
  reflectorShift.setY(0);
  reflectorShift.setZ(2*canThickness - reflectorFrontThickness);
  reflectorPos = DetPos + reflectorShift;

  pmtLength        = 261*mm - 104*mm;
  shieldThickness  = 0.64*mm;
  shieldLength     = pmtLength - 35*mm; // Measured by hand

  pmtThickness   = 1*mm;
  pmtFrontRadius = 83.2/2*mm - shieldThickness*mm;
  pmtFrontLength = 45.8*mm;

  pmtBaseRadius  = 58.8/2*mm;
  pmtBaseLength  = 101*mm;

  pmtTransitionLength = pmtLength - pmtFrontLength - pmtBaseLength;

  windowRadius   = crystalRadius;
  windowLength   = 12.5*mm;

  windowShift.setX(0);
  windowShift.setY(0);
  windowShift.setZ(1); 
  windowShift.setMag(canLength + windowLength/2);
  windowPos = DetPos + windowShift;

  // The G4Polycone position is the center of the z=0 plane, not
  // the center of the solid.
  // pmtShift.setMag(Length/2.0 + pmtLength/2.0);
  pmtShift.setX(0);
  pmtShift.setY(0);
  pmtShift.setZ(1); 
  pmtShift.setMag(canLength); 
  pmtPos = DetPos + pmtShift;

  thetad = 90.*deg;
  phid = 90.*deg;

  DetRot=G4RotationMatrix::IDENTITY;
  DetRot.rotateX(180.*deg);
  DetRot.rotateY(90.*deg+thetad);
  DetRot.rotateZ(phid);

  assembly    = new G4AssemblyVolume();

}

CeBr3_3x3_Detector::~CeBr3_3x3_Detector()
{
}

void CeBr3_3x3_Detector::Construct()
{

  // Material surrounding the crystal

  const G4double czPlane[4] =
    {0,
     canThickness,
     canThickness,
     canLength};
  const G4double crInner[4] = 
    {0,
     0,
     canRadius-canThickness,
     canRadius-canThickness};
  const G4double crOuter[4] =
    {canRadius,
     canRadius,
     canRadius,
     canRadius};
  can = new G4Polycone("Can", startAngle, spanningAngle, 4,
		       czPlane, crInner, crOuter);

  can_log = new G4LogicalVolume(can, Al, "can_log", 0, 0, 0);

  const G4double rzPlane[4] =
    {0,
     reflectorFrontThickness,
     reflectorFrontThickness,
     crystalLength+reflectorFrontThickness};
  const G4double rrInner[4] = 
    {0,
     0,
     crystalRadius,
     crystalRadius};
  const G4double rrOuter[4] =
    {crystalRadius+reflectorSideThickness,
     crystalRadius+reflectorSideThickness,
     crystalRadius+reflectorSideThickness,
     crystalRadius+reflectorSideThickness};
  reflector = new G4Polycone("Reflector", startAngle, spanningAngle, 4,
			     rzPlane, rrInner, rrOuter);

  reflector_log = new G4LogicalVolume(reflector, TeflonTape, "reflector_log",
				      0, 0, 0);

  const G4double pzPlane[6] =
    {0,
     pmtFrontLength,
     pmtFrontLength+pmtTransitionLength,
     pmtLength - pmtThickness,
     pmtLength - pmtThickness,
     pmtLength};
  const G4double prInner[6] = 
    {pmtFrontRadius - pmtThickness,
     pmtFrontRadius - pmtThickness,
     pmtBaseRadius - pmtThickness,
     pmtBaseRadius - pmtThickness,
     0,
     0};
  const G4double prOuter[6] =
    {pmtFrontRadius,
     pmtFrontRadius,
     pmtBaseRadius,
     pmtBaseRadius,
     pmtBaseRadius,
     pmtBaseRadius};
  pmt = new G4Polycone("PMT", startAngle, spanningAngle, 6,
		       pzPlane, prInner, prOuter);

  pmt_log = new G4LogicalVolume(pmt, MuMetal, "pmt_log", 0, 0, 0);

  // magnetic shield
  
  const G4double szPlane[4] =
    {0,
     pmtFrontLength,
     pmtFrontLength+pmtTransitionLength,
     shieldLength};
  const G4double srInner[4] = 
    {pmtFrontRadius,
     pmtFrontRadius,
     pmtBaseRadius,
     pmtBaseRadius};
  const G4double srOuter[4] =
    {pmtFrontRadius + shieldThickness,
     pmtFrontRadius + shieldThickness,
     pmtBaseRadius + shieldThickness,
     pmtBaseRadius + shieldThickness};
  shield = new G4Polycone("shield", startAngle, spanningAngle, 4,
			  szPlane, srInner, srOuter);

  shield_log = new G4LogicalVolume(shield, MuMetal, "pmt_log", 0, 0, 0);

  // window

  window = new G4Tubs("Window", 0, windowRadius, windowLength/2,
		      startAngle, spanningAngle);

  window_log = new G4LogicalVolume(window, quartz, "window_log",
				   0, 0, 0);
  
  // detector

  crystal = new G4Tubs("crystal", 0, crystalRadius, crystalLength/2,
		       startAngle, spanningAngle);

  crystal_log = new G4LogicalVolume(crystal, CeBr3, "crystal_log",
				    0, 0, 0);

  assembly->AddPlacedVolume(crystal_log, crystalPos, &DetRot);

  assembly->AddPlacedVolume(can_log, canPos, &DetRot);

  assembly->AddPlacedVolume(reflector_log, reflectorPos, &DetRot);

  assembly->AddPlacedVolume(pmt_log, pmtPos, &DetRot);

  assembly->AddPlacedVolume(shield_log, pmtPos, &DetRot);

  assembly->AddPlacedVolume(window_log, windowPos, &DetRot);

  //Visualization Attributes

  //   CeBr3 Crystal
  G4Colour dgreen (0.0,0.75, 0.0, 1.0); 
  G4VisAttributes* Vis_1 = new G4VisAttributes(dgreen);
  Vis_1->SetVisibility(true);
  Vis_1->SetForceSolid(true);

  //   Can and Reflector
  G4Colour transGrey (0.5, 0.5, 0.5, 0.3);
  G4VisAttributes* Vis_2 = new G4VisAttributes(transGrey);
  Vis_2->SetVisibility(true);
  Vis_2->SetForceSolid(true);

  //   Dead material
  G4Colour dGrey (0.8, 0.8, 0.8, 1.0);
  G4VisAttributes* Vis_3 = new G4VisAttributes(dGrey);
  Vis_3->SetVisibility(true);
  Vis_3->SetForceSolid(true);

  crystal_log->SetVisAttributes(Vis_1);

  can_log->SetVisAttributes(Vis_2);

  reflector_log->SetVisAttributes(Vis_2);

  pmt_log->SetVisAttributes(Vis_3);

  shield_log->SetVisAttributes(Vis_3);

  window_log->SetVisAttributes(Vis_3);
 
}
//---------------------------------------------------------------------
void CeBr3_3x3_Detector::MakeSensitive(TrackerGammaSD* TrackerGamma)
{
  crystal_log->SetSensitiveDetector(TrackerGamma);
}
//---------------------------------------------------------------------
