#ifndef CeBrA_Array_H
#define CeBrA_Array_H

#include "CeBr3_2x2_Detector.hh"
#include "CeBr3_3x3_Detector.hh"
#include "CeBr3_3x4_Detector.hh"
#include "CeBr3_3x6_Detector.hh"
#include "Cradle.hh"
#include "TrackerGammaSD.hh"

class CeBrA_Array 
{
public:
  
  CeBrA_Array(G4LogicalVolume*, Materials*, Cradle*);
  ~CeBrA_Array();

  // Used by the CeBrA_Array_Messenger for placing a single detector
  // (when a geometry file is not specified).
  void setX(G4double x){assemblyPos.setX(x);}
  void setY(G4double y){assemblyPos.setY(y);}
  void setZ(G4double z){assemblyPos.setZ(z);}
  void rotateX(G4double ax){assemblyRot.rotateX(ax);}
  void rotateY(G4double ay){assemblyRot.rotateY(ay);}
  void rotateZ(G4double az){assemblyRot.rotateZ(az);}
  void setType(G4String t){detectorType = t;}
  
  void setGeoFile(G4String fname){geoFileName = fname;}

  void Construct();
  
  void MakeSensitive(TrackerGammaSD*);

private:
  G4LogicalVolume *expHall_log;
  Materials *materials;

  CeBr3_2x2_Detector* detector_2x2;
  CeBr3_3x3_Detector* detector_3x3;
  CeBr3_3x4_Detector* detector_3x4;
  CeBr3_3x6_Detector* detector_3x6;
  Cradle* cradle;
  //Beam_Pipe* pipe;
  
  G4ThreeVector assemblyPos;
  G4RotationMatrix assemblyRot;
  G4String detectorType;
  
  G4String geoFileName;
  std::ifstream geoFile;

};

#endif

