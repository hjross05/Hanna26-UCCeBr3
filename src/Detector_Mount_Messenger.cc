#include "Detector_Mount_Messenger.hh"

Detector_Mount_Messenger::Detector_Mount_Messenger(Detector_Mount* LB)
:Mount(LB)
{ 
 
  MountDir = new G4UIdirectory("/Mount/");
  MountDir->SetGuidance("Mount control.");

  XCmd = new G4UIcmdWithADoubleAndUnit("/Mount/setX",this);
  XCmd->SetGuidance("Set the x position of the Mount center");
  XCmd->SetParameterName("choice",false);
  XCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  YCmd = new G4UIcmdWithADoubleAndUnit("/Mount/setY",this);
  YCmd->SetGuidance("Set the y position of the Mount center");
  YCmd->SetParameterName("choice",false);
  YCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  ZCmd = new G4UIcmdWithADoubleAndUnit("/Mount/setZ",this);
  ZCmd->SetGuidance("Set the z position of the Mount center");
  ZCmd->SetParameterName("choice",false);
  ZCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  rXCmd = new G4UIcmdWithADoubleAndUnit("/Mount/rotateX",this);
  rXCmd->SetGuidance("Rotate the Mount about the x axis");
  rXCmd->SetParameterName("choice",false);
  rXCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  rYCmd = new G4UIcmdWithADoubleAndUnit("/Mount/rotateY",this);
  rYCmd->SetGuidance("Rotate the Mount about the y axis");
  rYCmd->SetParameterName("choice",false);
  rYCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  rZCmd = new G4UIcmdWithADoubleAndUnit("/Mount/rotateZ",this);
  rZCmd->SetGuidance("Rotate the Mount about the z axis");
  rZCmd->SetParameterName("choice",false);
  rZCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  cCmd = new G4UIcmdWithoutParameter("/Mount/Construct",this);
  cCmd->SetGuidance("Construct the Mount");
  cCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  GCmd = new G4UIcmdWithAString("/Mount/GeometryFile",this);
  GCmd->SetGuidance("Set the Mount geometry file name.");
  GCmd->SetParameterName("choice",false);
  GCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

}

Detector_Mount_Messenger::~Detector_Mount_Messenger()
{
  delete MountDir;
  delete XCmd;
  delete YCmd;
  delete ZCmd;
  delete rXCmd;
  delete rYCmd;
  delete rZCmd;
  delete cCmd;
  delete GCmd;
}


void Detector_Mount_Messenger::SetNewValue(G4UIcommand* command,G4String newValue)
{ 
  if( command == XCmd )
    {Mount->setX(XCmd->GetNewDoubleValue(newValue));}
  if( command == YCmd )
    {Mount->setY(YCmd->GetNewDoubleValue(newValue));}
  if( command == ZCmd )
    {Mount->setZ(ZCmd->GetNewDoubleValue(newValue));}
  if( command == rXCmd )
    {Mount->rotateX(rXCmd->GetNewDoubleValue(newValue));}
  if( command == rYCmd )
    {Mount->rotateY(rYCmd->GetNewDoubleValue(newValue));}
  if( command == rZCmd )
    {Mount->rotateZ(rZCmd->GetNewDoubleValue(newValue));}
  if( command == cCmd )
    {Mount->Construct();}
  if( command == GCmd )
    {Mount->setGeoFile(newValue);}
}

