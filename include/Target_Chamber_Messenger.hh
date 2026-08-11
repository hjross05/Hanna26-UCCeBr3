#ifndef Target_Chamber_Messenger_h
#define Target_Chamber_Messenger_h 1

#include "Target_Chamber.hh"
#include "globals.hh"
#include "G4UImessenger.hh"
#include "G4UIdirectory.hh"
#include "G4UIcmdWithoutParameter.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWithAnInteger.hh"

class Target_Chamber_Messenger: public G4UImessenger
{
  public:
    Target_Chamber_Messenger(Target_Chamber*);
   ~Target_Chamber_Messenger();
    
    void SetNewValue(G4UIcommand*, G4String);
    
  private:
    Target_Chamber* Chamber;
   
    G4UIdirectory*             ChamberDir;
    G4UIdirectory*             TLadderDir;
    G4UIcmdWithoutParameter*   cCmd;
    G4UIcmdWithADoubleAndUnit* XCmd;
    G4UIcmdWithADoubleAndUnit* YCmd;
    G4UIcmdWithADoubleAndUnit* ZCmd;
    G4UIcmdWithAnInteger* HCmd;

  };

#endif

