#include "DetectorConstruction.hh"

DetectorConstruction::DetectorConstruction()
{
  //  Materials* materials=new Materials();
  materials=new Materials();

  // Experimental Hall

  Experimental_Hall* ExperimentalHall = new Experimental_Hall(materials);
  ExpHall_phys=ExperimentalHall->Construct();
  ExpHall_log=ExperimentalHall->GetLogVolume();
  //   ExperimentalHall->Report();
  ExperimentalHallMessenger = new Experimental_Hall_Messenger(ExperimentalHall);

  capsule = new Source_Capsule(ExpHall_log, materials);

  capsule_Messenger = new Source_Capsule_Messenger(capsule);

  source_cradle = new Source_Cradle(ExpHall_log, materials);

  source_cradle_Messenger = new Source_Cradle_Messenger(source_cradle);
  
  brick = new Lead_Brick(ExpHall_log, materials);

  brick_Messenger = new Lead_Brick_Messenger(brick);

  bench = new Lab_Bench(ExpHall_log, materials);

  bench_Messenger = new Lab_Bench_Messenger(bench);

  chamber = new Target_Chamber(ExpHall_log, materials);

  chamber_Messenger = new Target_Chamber_Messenger(chamber);

  cradle = new Cradle(ExpHall_log, materials);

  cradle_Messenger = new Cradle_Messenger(cradle);
  
  the_CeBr3_Array = new CeBr3_Array(ExpHall_log, materials, cradle);

  the_CeBr3_Array_Messenger = new CeBr3_Array_Messenger(the_CeBr3_Array);

}

DetectorConstruction::~DetectorConstruction()
{
  delete ExperimentalHallMessenger;
  delete TrackerGammaSDMessenger;
}

G4VPhysicalVolume* DetectorConstruction::Construct()
{

  the_CeBr3_Array->Construct();

  //------------------------------------------------ 
  // Sensitive detectors
  //------------------------------------------------ 

  G4SDManager* SDman = G4SDManager::GetSDMpointer();

  //------------------------------------------------
  // Detectors sensitive for gamma-ray tracking
  //-------------------------------------------------

  TrackerGamma = new TrackerGammaSD("GammaTracker");
  TrackerGammaSDMessenger = new TrackerGammaSD_Messenger(TrackerGamma);
  SDman->AddNewDetector(TrackerGamma);
  the_CeBr3_Array->MakeSensitive(TrackerGamma);

  return ExpHall_phys;
}
