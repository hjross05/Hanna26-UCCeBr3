#include "Beam_Pipe.hh"

// The constructor signature now uses standard names without typos
Beam_Pipe::Beam_Pipe(G4LogicalVolume* experimentalHall_log, Materials* mat)
{
  materials = mat;
  expHall_log = experimentalHall_log;

  Al = materials->FindMaterial("Al");
  
  Pos.setX(0);
  Pos.setY(0);
  Pos.setZ(20.574*cm);

  Rot = G4RotationMatrix::IDENTITY;
  
  Radius = 1.5 * 2.54 * cm;
  Thickness = 1.75 * mm;
  hz = 10 * 2.54 * cm;
}

Beam_Pipe::~Beam_Pipe()
{
}

void Beam_Pipe::Construct()
{
  G4cout << "!!! GEANT4 DEBUG: Building the Beam Pipe Right Now !!!" << G4endl;
  
  // G4Tubs is now recognized because we added the header
  G4Tubs* cylinder = new G4Tubs("HollowCylinder", Radius-Thickness, Radius, hz, 0*deg, 360*deg);

  pipe_log = new G4LogicalVolume(cylinder, Al, "pipe_log");

  // expHall_log is spelled correctly here now
  pipe_phys = new G4PVPlacement(G4Transform3D(Rot, Pos),
                                pipe_log, "Pipe", 
                                expHall_log, false, 0);

  G4Colour dGrey (0.8, 0.8, 0.8, 1.0);
  G4VisAttributes* Vis = new G4VisAttributes(dGrey);
  Vis->SetVisibility(true);
  Vis->SetForceSolid(true);
  pipe_log->SetVisAttributes(Vis);

  return; 
}