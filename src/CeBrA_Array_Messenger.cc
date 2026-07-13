#include "CeBrA_Array_Messenger.hh"

CeBrA_Array_Messenger::CeBrA_Array_Messenger(CeBrA_Array* NA)
:CeBr3Array(NA)
{ 
 
  CeBr3Dir = new G4UIdirectory("/CeBr3/");
  CeBr3Dir->SetGuidance("CeBr3 control.");

  XCmd = new G4UIcmdWithADoubleAndUnit("/CeBr3/setX",this);
  XCmd->SetGuidance("Set the x position of the detector (crystal center)");
  XCmd->SetParameterName("choice",false);
  XCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  YCmd = new G4UIcmdWithADoubleAndUnit("/CeBr3/setY",this);
  YCmd->SetGuidance("Set the y position of the detector (crystal center)");
  YCmd->SetParameterName("choice",false);
  YCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  ZCmd = new G4UIcmdWithADoubleAndUnit("/CeBr3/setZ",this);
  ZCmd->SetGuidance("Set the z position of the detector (crystal center)");
  ZCmd->SetParameterName("choice",false);
  ZCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  rXCmd = new G4UIcmdWithADoubleAndUnit("/CeBr3/rotateX",this);
  rXCmd->SetGuidance("Rotate the detector about the x axis");
  rXCmd->SetParameterName("choice",false);
  rXCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  rYCmd = new G4UIcmdWithADoubleAndUnit("/CeBr3/rotateY",this);
  rYCmd->SetGuidance("Rotate the detector about the y axis");
  rYCmd->SetParameterName("choice",false);
  rYCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  rZCmd = new G4UIcmdWithADoubleAndUnit("/CeBr3/rotateZ",this);
  rZCmd->SetGuidance("Rotate the detector about the z axis");
  rZCmd->SetParameterName("choice",false);
  rZCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  TCmd = new G4UIcmdWithAString("/CeBr3/Type",this);
  TCmd->SetGuidance("Detector type (2x2 | 3x4 | 3x6)");
  TCmd->SetParameterName("choice",false);
  TCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  GCmd = new G4UIcmdWithAString("/CeBr3/GeometryFile",this);
  GCmd->SetGuidance("Set the geometry file name.");
  GCmd->SetParameterName("choice",false);
  GCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

}



CeBrA_Array_Messenger::~CeBrA_Array_Messenger()
{
  delete CeBr3Dir;
  delete XCmd;
  delete YCmd;
  delete ZCmd;
  delete rXCmd;
  delete rYCmd;
  delete rZCmd;
  delete TCmd;
  delete GCmd;
}


void CeBrA_Array_Messenger::SetNewValue(G4UIcommand* command,G4String newValue)
{ 
  if( command == XCmd )
    {CeBr3Array->setX(XCmd->GetNewDoubleValue(newValue));}
  if( command == YCmd )
    {CeBr3Array->setY(YCmd->GetNewDoubleValue(newValue));}
  if( command == ZCmd )
    {CeBr3Array->setZ(ZCmd->GetNewDoubleValue(newValue));}
  if( command == ZCmd )
    {CeBr3Array->setZ(ZCmd->GetNewDoubleValue(newValue));}
  if( command == rXCmd )
    {CeBr3Array->rotateX(rXCmd->GetNewDoubleValue(newValue));}
  if( command == rYCmd )
    {CeBr3Array->rotateY(rYCmd->GetNewDoubleValue(newValue));}
  if( command == rZCmd )
    {CeBr3Array->rotateZ(rZCmd->GetNewDoubleValue(newValue));}
  if( command == TCmd )
    {CeBr3Array->setType(newValue);}
  if( command == GCmd )
    {CeBr3Array->setGeoFile(newValue);}
}
