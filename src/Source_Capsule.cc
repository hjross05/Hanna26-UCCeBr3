#include "Source_Capsule.hh"

Source_Capsule::Source_Capsule(G4LogicalVolume* experimentalHall_log,
		       Materials* mat)
{
  materials=mat;
  expHall_log=experimentalHall_log;

  capsuleType = "plastic";
  
  //  polyethylene = materials->FindMaterial("G4_POLYETHYLENE");
  plexiglass = materials->FindMaterial("G4_PLEXIGLASS");

  aluminum = materials->FindMaterial("Al");

  Pos.setX(0);
  Pos.setY(0);
  Pos.setZ(0);

  Rot = G4RotationMatrix::IDENTITY;
  
}

Source_Capsule::~Source_Capsule()
{
}

G4VPhysicalVolume* Source_Capsule::Construct()
{

  if(capsuleType == "plastic"){

    // Spectrum Techniques disk sources
    // https://www.spectrumtechniques.com/product/rss8
    
    //Depth=0.125*25.4*mm;           // 1/8 inch thickness
    Depth=5.56*mm; // FSU 152Eu and 60Co Sources
    Radius=25.4/2.0*mm;            // 1 inch OD
    innerRadius=0.125/2.0*25.4*mm; // 1/4 inch diameter divot
    startAngle      = 45.*deg;
    spanningAngle   = 360.*deg;

    const G4double zPlane[4] =
      {-Depth/2.0,      0,           0,   Depth/2.0};
    const G4double rInner[4] =
      {         0,      0, innerRadius, innerRadius};
    const G4double rOuter[4] =
      {    Radius, Radius,      Radius,      Radius};

    G4Tubs* capsule = new G4Tubs("Capsule", 0, Radius, Depth/2, 0*deg, 360*deg);
    
    /*G4Polycone* capsule = new G4Polycone("Capsule", startAngle, spanningAngle,
    					 4, zPlane, rInner, rOuter);*/

    //    G4Tubs* capsule = new G4Tubs("Capsule", 0, Radius, Depth/2.0,
    //				 startAngle,spanningAngle);
    
    capsule_log = new G4LogicalVolume(capsule, plexiglass, "capsule_log",
				      0, 0, 0);

  } else if(capsuleType == "GF-152-M"){

    // Eckert & Ziegler Type M disk
    // https://www.ezag.com/wp-content/uploads/2023/08/Gamma_Standards_M-Type.pdf
    Depth=3.18*mm;
    Thickness = 0.8*mm;
    Radius=25.4/2*mm;
    innerRadius=22.1/2*mm; // This is an estimate based on the drawing.
    startAngle      = 45.*deg;
    spanningAngle   = 360.*deg;

    // Origin at the activity center
    const G4double zPlane[4] =
      {-Thickness,            0,                0,  Depth-Thickness};
    const G4double rInner[4] =
      {innerRadius, innerRadius, Radius-Thickness, Radius-Thickness};
    const G4double rOuter[4] =
      {Radius,           Radius,           Radius,           Radius};
    
    G4Polycone* capsule = new G4Polycone("Capsule", startAngle, spanningAngle,
					 4, zPlane, rInner, rOuter);
    
    capsule_log = new G4LogicalVolume(capsule, aluminum, "capsule_log",
				      0, 0, 0);
  
  }else if (capsuleType == "FSU_Ba133"){
    Length = 23.33*mm;
    Width = 11.13*mm;
    Depth = 2.4*mm;

  } else {

    G4cerr << "Unknown capsule type: " << capsuleType << G4endl;
    exit(EXIT_FAILURE);

  }
  
  PlaceCapsule();

  G4Colour dGrey (0.8, 0.8, 0.8, 1.0);
  G4VisAttributes* Vis = new G4VisAttributes(dGrey);
  Vis->SetVisibility(true);
  Vis->SetForceSolid(true);

  capsule_log->SetVisAttributes(Vis);

  return capsule_phys;
}
//---------------------------------------------------------------------
void Source_Capsule::PlaceCapsule()
{

  capsule_phys = new G4PVPlacement(G4Transform3D(Rot,Pos),
				   capsule_log, "Capsule",
				   expHall_log, false, 0); 

}
//---------------------------------------------------------------------
