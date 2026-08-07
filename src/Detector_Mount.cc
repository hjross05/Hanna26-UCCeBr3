#include "Detector_Mount.hh"

Detector_Mount::Detector_Mount(G4LogicalVolume* experimentalHall_log,
		     Materials* mat)
{
  materials=mat;
  expHall_log=experimentalHall_log;

  EpoxyResin = materials->FindMaterial("EpoxyResin");
  Steel      = materials->FindMaterial("G4_STAINLESS-STEEL");

  
  LCradleBox_Length = 10.16/2 * cm;
  LCradleBox_Width = 4.7752/2 * cm;
  LCradleBox_Depth = 5.08/2 * cm;
  LCradleHole_Radius = 7.82/2 * cm;
 
    
  BPCradleBox_Length = 7.62/2 * cm;
  BPCradleBox_Width = 3.81/2 * cm;
  BPCradleBox_Depth = 2.54/2 * cm;
  BPCradleHole_Radius = 5.08/2 * cm;
 

  //Plastic Detector Cradles
  UpperCradle_Shift.setX(-3.48*cm);
  UpperCradle_Shift.setY(13.15*cm);
  UpperCradle_Shift.setZ(0);

  BPUpperCradle_Shift.setX(2.665*cm); //-3.165
  BPUpperCradle_Shift.setY(4.85*cm);
  BPUpperCradle_Shift.setZ(0);

  LowerCradle_Shift.setX(0);
  LowerCradle_Shift.setY(0);
  LowerCradle_Shift.setZ(0);

  CradleHole_Shift.setZ(2.54*cm);
  BPCradleHole_Shift.setZ(-1.905*cm);
    
  //Mounts
  LLeft_Shift.setX(2.3876*cm);
  LLeft_Shift.setY(-2.54*cm);
  LLeft_Shift.setZ(5.33*cm);

  LRight_Shift.setX(2.3876*cm); 
  LRight_Shift.setY(-2.54*cm);
  LRight_Shift.setZ(-5.33*cm);

  BPLeft_Shift.setX(2.105*cm);
  BPLeft_Shift.setY(4.06*cm);
  BPLeft_Shift.setZ(4.06*cm);

  BPRight_Shift.setX(2.105*cm);
  BPRight_Shift.setY(4.06*cm);
  BPRight_Shift.setZ(-4.06*cm);

  //Cradles
  CradleHole_Rot = G4RotationMatrix::IDENTITY;
  CradleHole_Rot.rotateX(90*deg);

  Cradle0_Rot = G4RotationMatrix::IDENTITY;
  Cradle0_Rot.rotateX(-90*deg);
  Cradle0_Rot.rotateY(90*deg);

  Cradle1_Rot = G4RotationMatrix::IDENTITY;
  Cradle1_Rot.rotateX(-135*deg);
  Cradle1_Rot.rotateY(90*deg);

  BBPCradle_Rot = G4RotationMatrix::IDENTITY;
  BBPCradle_Rot.rotateX(-55*deg);
  BBPCradle_Rot.rotateY(90*deg);

  TBPCradle_Rot = G4RotationMatrix::IDENTITY;
  TBPCradle_Rot.rotateX(90*deg);
  TBPCradle_Rot.rotateY(90*deg);
  

  BPMount_Rot = G4RotationMatrix::IDENTITY;
  BPMount_Rot.rotateZ(0*deg);
  BPMount_Rot.rotateY(0);


  Rot = G4RotationMatrix::IDENTITY;

  Pos.setX(0);
  Pos.setY(0);
  Pos.setZ(0);


  assemblyRot = Rot;
  assemblyPos.setX(0);
  assemblyPos.setY(0);
  assemblyPos.setZ(0);
    
  Lassembly    = new G4AssemblyVolume();
  BPassembly    = new G4AssemblyVolume();

  //isBeamPipe = false;

  constructed = false;

  geoFileName = "";
}


Detector_Mount::~Detector_Mount()
{
}

void Detector_Mount::Construct()
{
  //Regular side brackets
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

  LMount_log = new G4LogicalVolume(rawPlate, Steel, "LMount_log");

  

  G4Box* LCradleBox = new G4Box("LCradleBox", LCradleBox_Length, LCradleBox_Width, LCradleBox_Depth);
  G4Tubs* LCradleHole = new G4Tubs("LCradleHole", 0, LCradleHole_Radius, LCradleBox_Length, 0*deg, 360*deg);
  G4SubtractionSolid* LCradle = new G4SubtractionSolid("LCradle", LCradleBox, LCradleHole, G4Transform3D(CradleHole_Rot, CradleHole_Shift));

  LargeCradle_log = new G4LogicalVolume(LCradle, EpoxyResin, "LargeCradle_log");

  Lassembly->AddPlacedVolume(LargeCradle_log, LowerCradle_Shift, &Cradle0_Rot);

  Lassembly->AddPlacedVolume(LargeCradle_log, UpperCradle_Shift, &Cradle1_Rot);

  Lassembly->AddPlacedVolume(LMount_log, LRight_Shift, &Rot);

  Lassembly->AddPlacedVolume(LMount_log, LLeft_Shift, &Rot);




  //Beam Pipe Mount side brackets
  std::vector<G4TwoVector> bppolygon;
  bppolygon.push_back(G4TwoVector( 1.00 * 2.54 * cm,  1.60 * 2.54 * cm)); // V1: Top-Right
  bppolygon.push_back(G4TwoVector( 0.00 * 2.54 * cm,  1.60 * 2.54 * cm)); // V2: Top-Left
  bppolygon.push_back(G4TwoVector( 0.00 * 2.54 * cm,  0.00 * 2.54 * cm)); // V3: Inner bend
  bppolygon.push_back(G4TwoVector(-1.31 * 2.54 * cm, -1.84 * 2.54 * cm)); // V4: Bottom-Left
  bppolygon.push_back(G4TwoVector(-0.50 * 2.54 * cm, -2.42 * 2.54 * cm)); // V5: Bottom-Right
  bppolygon.push_back(G4TwoVector( 1.00 * 2.54 * cm, -0.32 * 2.54 * cm)); // V6: Outer bend

  std::vector<G4ExtrudedSolid::ZSection> bpzSections;
  bpzSections.push_back(G4ExtrudedSolid::ZSection(-2.5 * mm, G4TwoVector(0,0), 1.0));
  bpzSections.push_back(G4ExtrudedSolid::ZSection( 2.5 * mm, G4TwoVector(0,0), 1.0));

  G4ExtrudedSolid* BeamPlate = new G4ExtrudedSolid("BeamPlate", bppolygon, bpzSections);

  BPMount_log = new G4LogicalVolume(BeamPlate, Steel, "BPMount_log");

  G4Box* BPCradleBox = new G4Box("BPCradleBox", BPCradleBox_Length, BPCradleBox_Width, BPCradleBox_Depth);
  G4Tubs* BPCradleHole = new G4Tubs("BPCradleHole", 0, BPCradleHole_Radius, BPCradleBox_Length, 0*deg, 360*deg);
  G4SubtractionSolid* BPCradle = new G4SubtractionSolid("BPCradle", BPCradleBox, BPCradleHole, G4Transform3D(CradleHole_Rot, BPCradleHole_Shift));

  BeamCradle_log = new G4LogicalVolume(BPCradle, EpoxyResin, "BeamCradle_log");

  BPassembly->AddPlacedVolume(BeamCradle_log, LowerCradle_Shift, &BBPCradle_Rot);

  BPassembly->AddPlacedVolume(BeamCradle_log, BPUpperCradle_Shift, &TBPCradle_Rot);

  BPassembly->AddPlacedVolume(BPMount_log, BPRight_Shift, &BPMount_Rot);

  BPassembly->AddPlacedVolume(BPMount_log, BPLeft_Shift, &BPMount_Rot);




  G4Colour dGrey (0.8, 0.8, 0.8, 1.0);
  G4VisAttributes* Vis = new G4VisAttributes(dGrey);
  Vis->SetVisibility(true);
  Vis->SetForceSolid(true);

  //fCradle_log->SetVisAttributes(Vis);

  constructed = true;

  PlaceMount();
  
  return;
}
//---------------------------------------------------------------------
void Detector_Mount::PlaceMount()
{
  if (!constructed) {
    Construct();
  }

  /*if (isBeamPipe) {
    BPassembly->MakeImprint(expHall_log, assemblyPos, &assemblyRot, copyNo);
  } else {
    Lassembly->MakeImprint(expHall_log, assemblyPos, &assemblyRot, copyNo);
  }*/
  // If there is a Mount geometry file, use it for placement;
  // if not, use the current Pos and Rot to place a single Mount.
  if(geoFileName != ""){
    geoFile.open(geoFileName.c_str());
    if (!geoFile.is_open())
      G4cout<< "ERROR opening Mount geometry file." << G4endl;
    else
      G4cout << "\nPositioning Mounts using the geometry file: " << geoFileName << G4endl;
    
    char Label[50];
    G4int MountID = 0;
    G4String type;
    G4double x, y, z, ax, ay, az;
    while(geoFile >> type >> x >> y >> z >> ax >> ay >> az){
      sprintf(Label, "Mount%d", MountID);
      Pos.setX(x);
      Pos.setY(y);
      Pos.setZ(z);
      Rot = G4RotationMatrix::IDENTITY;
      Rot.rotateX(ax*deg);
      Rot.rotateY(ay*deg);
      Rot.rotateZ(az*deg);
      if (type.contains("BP") || type.contains("bp")){
        BPassembly->MakeImprint(expHall_log, Pos, &Rot, MountID);
      }else if (type.contains("3x3x")){
        Lassembly->MakeImprint(expHall_log, Pos, &Rot, MountID);
      }
      MountID++;
    }
    geoFile.close();
  } else {
    G4cout<< "ERROR: unrecognized mount type" << G4endl;
  }
}
/*//---------------------------------------------------------------------
void Detector_Mount::setX(G4double x)
{
  Pos.setX(x);
}
//---------------------------------------------------------------------
void Detector_Mount::setY(G4double y)
{
  Pos.setY(y);
}
//---------------------------------------------------------------------
void Detector_Mount::setZ(G4double z)
{
  Pos.setZ(z);
}
//---------------------------------------------------------------------
void Detector_Mount::rotateX(G4double ax)
{
  Rot.rotateX(ax);
}
//---------------------------------------------------------------------
void Detector_Mount::rotateY(G4double ay)
{
  Rot.rotateY(ay);
}
//---------------------------------------------------------------------
void Detector_Mount::rotateZ(G4double az)
{
  Rot.rotateZ(az);
}
//---------------------------------------------------------------------


//---------------------------------------------------------------------*/
