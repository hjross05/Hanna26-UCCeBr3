#include "Target_Chamber.hh"

Target_Chamber::Target_Chamber(G4LogicalVolume* experimentalHall_log,
			       Materials* mat)
{
  materials = mat;
  expHall_log=experimentalHall_log;

  Al = materials->FindMaterial("Al");

  Pos.setX(0);
  Pos.setY(0);
  Pos.setZ(0);

  Rot = G4RotationMatrix::IDENTITY;
  
  Radius    = 4.05*2.54*cm;
  Thickness = 1.75*mm;
}

Target_Chamber::~Target_Chamber()
{  
}

void Target_Chamber::Construct()
{

  G4Sphere* sphere = new G4Sphere("SphericalShell", Radius-Thickness, Radius,
				  0, 360.0*deg, 0, 180.0*deg);
  chamber_log = new G4LogicalVolume(sphere, Al, "chamber_log");

  /*  
  auto chamber_mesh = CADMesh::TessellatedMesh::FromSTL("/nuc/CeBrA/geant4/UCCeBr3/cadModels/assembly_chamber_02_ascii.stl");
  auto chamber_solid = chamber_mesh->GetSolid();

  chamber_log = new G4LogicalVolume(chamber_solid, Al, "chamber_log");
  */
  
  chamber_phys = new G4PVPlacement(G4Transform3D(Rot, Pos),
                                   chamber_log, "Chamber",
                                   expHall_log, false, 0);

  G4Colour dGrey (0.8, 0.8, 0.8, 1.0);
  G4VisAttributes* Vis = new G4VisAttributes(dGrey);
  Vis->SetVisibility(true);
  Vis->SetForceSolid(true);

  chamber_log->SetVisAttributes(Vis);

  return; 
}
