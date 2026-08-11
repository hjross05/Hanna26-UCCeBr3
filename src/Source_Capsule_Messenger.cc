#include "Source_Capsule_Messenger.hh"

Source_Capsule_Messenger::Source_Capsule_Messenger(Source_Capsule* sc)
:capsule(sc)
{ 
 
  Source_CapsuleDir = new G4UIdirectory("/Source/Capsule/");
  Source_CapsuleDir->SetGuidance("Source_Capsule control.");

  tCmd = new G4UIcmdWithAString("/Source/Capsule/Type", this);
  tCmd->SetGuidance("Set the source capsule type (plastic || GF-152-M)");
  tCmd->SetParameterName("choice",false);
  tCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  XCmd = new G4UIcmdWithADoubleAndUnit("/Source/Capsule/setX", this);
  XCmd->SetGuidance("Set the x position of the source capsule");
  XCmd->SetParameterName("choice",false);
  XCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  YCmd = new G4UIcmdWithADoubleAndUnit("/Source/Capsule/setY", this);
  YCmd->SetGuidance("Set the y position of the source capsule");
  YCmd->SetParameterName("choice",false);
  YCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  ZCmd = new G4UIcmdWithADoubleAndUnit("/Source/Capsule/setZ", this);
  ZCmd->SetGuidance("Set the z position of the bottom right of the ladder");
  ZCmd->SetParameterName("choice",false);
  ZCmd->AvailableForStates(G4State_PreInit,G4State_Idle);
  
  rXCmd = new G4UIcmdWithADoubleAndUnit("/Source/Capsule/rotateX", this);
  rXCmd->SetGuidance("Rotate the capsule about the x axis");
  rXCmd->SetParameterName("choice",false);
  rXCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  rYCmd = new G4UIcmdWithADoubleAndUnit("/Source/Capsule/rotateY", this);
  rYCmd->SetGuidance("Rotate the capsule about the y axis");
  rYCmd->SetParameterName("choice",false);
  rYCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  rZCmd = new G4UIcmdWithADoubleAndUnit("/Source/Capsule/rotateZ", this);
  rZCmd->SetGuidance("Rotate the capsule about the z axis");
  rZCmd->SetParameterName("choice",false);
  rZCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

}

Source_Capsule_Messenger::~Source_Capsule_Messenger()
{
  delete Source_CapsuleDir;
  delete tCmd;
  delete rXCmd;
  delete rYCmd;
  delete rZCmd;
}


void Source_Capsule_Messenger::SetNewValue(G4UIcommand* command,G4String newValue)
{ 
  if( command == tCmd )
    {capsule->setType(newValue);}
  if (command == XCmd)  { capsule->setX(XCmd->GetNewDoubleValue(newValue)); }
  if (command == YCmd)  { capsule->setY(YCmd->GetNewDoubleValue(newValue)); }
  if (command == ZCmd)  { capsule->setZ(ZCmd->GetNewDoubleValue(newValue)); }
  if( command == rXCmd )
    {capsule->rotateX(rXCmd->GetNewDoubleValue(newValue));}
  if( command == rYCmd )
    {capsule->rotateY(rYCmd->GetNewDoubleValue(newValue));}
  if( command == rZCmd )
    {capsule->rotateZ(rZCmd->GetNewDoubleValue(newValue));}
}

