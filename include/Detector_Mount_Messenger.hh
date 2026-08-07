#ifndef Detector_Mount_Messenger_h
#define Detector_Mount_Messenger_h 1

#include "Detector_Mount.hh"
#include "globals.hh"
#include "G4UImessenger.hh"
#include "G4UIdirectory.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWithADouble.hh"
#include "G4UIcmdWithoutParameter.hh"

class Detector_Mount_Messenger: public G4UImessenger
{
  public:
    Detector_Mount_Messenger(Detector_Mount*);
   ~Detector_Mount_Messenger();
    
    void SetNewValue(G4UIcommand*, G4String);
    
  private:
    Detector_Mount* Mount;
   
    G4UIdirectory*             MountDir;  

    G4UIcmdWithADoubleAndUnit* XCmd;
    G4UIcmdWithADoubleAndUnit* YCmd;
    G4UIcmdWithADoubleAndUnit* ZCmd;
    G4UIcmdWithADoubleAndUnit* rXCmd;
    G4UIcmdWithADoubleAndUnit* rYCmd;
    G4UIcmdWithADoubleAndUnit* rZCmd;
    G4UIcmdWithoutParameter*   cCmd;
    G4UIcmdWithAString*        GCmd;
};


#endif

