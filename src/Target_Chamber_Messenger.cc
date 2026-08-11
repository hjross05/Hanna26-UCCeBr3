#include "Target_Chamber_Messenger.hh"

Target_Chamber_Messenger::Target_Chamber_Messenger(Target_Chamber* C)
:Chamber(C)
{ 
  ChamberDir = new G4UIdirectory("/Chamber/");
  ChamberDir->SetGuidance("Chamber control.");

  cCmd = new G4UIcmdWithoutParameter("/Chamber/Construct",this);
  cCmd->SetGuidance("Construct the bench");
  cCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  TLadderDir = new G4UIdirectory("/Chamber/Ladder/");
  TLadderDir->SetGuidance("Target_Ladder control.");

  /*cLCmd = new G4UIcmdWithoutParameter("/Chamber/Ladder/Construct",this);
  cLCmd->SetGuidance("Construct the Target Ladder");
  cLCmd->AvailableForStates(G4State_PreInit,G4State_Idle);*/

  XCmd = new G4UIcmdWithADoubleAndUnit("/Chamber/Ladder/setX", this);
  XCmd->SetGuidance("Set the x position of the bottom right of the ladder");
  XCmd->SetParameterName("choice",false);
  XCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  YCmd = new G4UIcmdWithADoubleAndUnit("/Chamber/Ladder/setY", this);
  YCmd->SetGuidance("Set the y position of the bottom right of the ladder");
  YCmd->SetParameterName("choice",false);
  YCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  ZCmd = new G4UIcmdWithADoubleAndUnit("/Chamber/Ladder/setZ", this);
  ZCmd->SetGuidance("Set the z position of the bottom right of the ladder");
  ZCmd->SetParameterName("choice",false);
  ZCmd->AvailableForStates(G4State_PreInit,G4State_Idle);
}

Target_Chamber_Messenger::~Target_Chamber_Messenger()
{
  delete ChamberDir;
  delete cCmd;
  delete TLadderDir;
  delete XCmd;
  delete YCmd;
  delete ZCmd;
}

void Target_Chamber_Messenger::SetNewValue(G4UIcommand* command,G4String newValue)
{ 
  if( command == cCmd )
    {Chamber->Construct();}
  if (command == XCmd)  { Chamber->setLadderX(XCmd->GetNewDoubleValue(newValue)); }
  if (command == YCmd)  { Chamber->setLadderY(YCmd->GetNewDoubleValue(newValue)); }
  if (command == ZCmd)  { Chamber->setLadderZ(ZCmd->GetNewDoubleValue(newValue)); }
}
