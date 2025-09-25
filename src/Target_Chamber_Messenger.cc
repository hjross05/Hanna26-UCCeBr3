#include "Target_Chamber_Messenger.hh"

Target_Chamber_Messenger::Target_Chamber_Messenger(Target_Chamber* C)
:Chamber(C)
{ 
  ChamberDir = new G4UIdirectory("/Chamber/");
  ChamberDir->SetGuidance("Chamber control.");

  cCmd = new G4UIcmdWithoutParameter("/Chamber/Construct",this);
  cCmd->SetGuidance("Construct the bench");
  cCmd->AvailableForStates(G4State_PreInit,G4State_Idle);
}

Target_Chamber_Messenger::~Target_Chamber_Messenger()
{
  delete ChamberDir;
  delete cCmd;
}

void Target_Chamber_Messenger::SetNewValue(G4UIcommand* command,G4String newValue)
{ 
  if( command == cCmd )
    {Chamber->Construct();}
}
