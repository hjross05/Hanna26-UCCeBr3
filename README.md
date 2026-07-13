# UCCeBrA

## Compile and install

Install version [10.7.4 of the Geant4 libraries](https://geant4.web.cern.ch/download/10.7.4.html). You will need the data files for low energy electromagnetic processes, photon evaporation, and radioactive decay.

Set up your environment (consider adding this to your `.bashrc`):

    $ source <Path to Geant4>/bin/geant4.sh
    $ source <Path to Geant4>/share/Geant4-10.7.4/geant4make/geant4make.sh

Compile:

    $ make


The executable is automatically installed in

    $G4WORKDIR/bin/$G4SYSTEM

(which is added to your path when you source `geant4make.sh`)

## Examples
Run an example by typing `make` at the command line in the corresponding directory. Python 3.x is requred to run the sorting codes, and the [root data analysis framework](https://root.cern/) is needed to work with the sorted histograms.

### `./examples/cs137`

This is a simple example collecting a spectrum with a single CeBr detector from a <sup>137</sup>Cs source. Two macro files are included. cs137.mac uses the radioactive decay class to simulate the decay of 137Cs. cs137_simple.mac emits 662 keV gamma rays directly. A sorting code `cs137_sim_sort.py` is included which produces histograms in a `.root` file. A `Makefile` is included.

### `./examples/co60`

This is a simulation of a <sup>60</sup>Co source placed in the CeBrA demonstrator. co60.mac simulates a 60Co source placed at the target position of CeBrA. The demonstrator configuration from 2024-2025 is placed using a geometry file: demonstrator.geo. Lead shielding bricks are placed with bricks.geo. The sorting code `co60_sim_sort.py` sorts singles spectra, with and without energy resolution folded in, a gamma-gamma coincidence matrix, and spectra of gamma rays collected in coincidence with the 1332 keV transition. A `Makefile` is included.

## Macro File Commands

### CeBr3 Detector Placement

    /CeBr3/Type <2x2 | 3x3 | 3x4 | 3x6>

> Set the type of CeBr3 detector to place. If "Cradle" or "cradle" is included in the <type> string (3x4cradle, e.g.), a cradle is included with the detector.

    /CeBr3/setX <double> <unit>
    /CeBr3/setY <double> <unit>
    /CeBr3/setZ <double> <unit>

> Set the position of the detector.

    /CeBr3/rotateX <double> <unit>
    /CeBr3/rotateY <double> <unit>
    /CeBr3/rotateZ <double> <unit>

> Orient the detector by rotating about X, Y, Z.

    /CeBr3/GeometryFile <filename>

> Set the name of the optional geometry file. If this command is present, a CeBr3 detector is placed for each line in the specified file. Each line has the format:

        <type(2x2 | 3x4 | 3x6)>  <X (mm)>  <Y (mm)>  <Z (mm)>  <X rotation (deg)>  <Y rotation (deg)>  <Z rotation (deg)>

> If a geometry file is specified, the positioning and rotation commands above are ignored. If "Cradle" or "cradle" is included in the <type> string (3x4cradle, e.g.), a cradle is included with the detector.

### Source

Realistic simulations of radioactive sources can be run as illustrated by `./examples/cs137/cs137.mac` and `/examples/co60/co60.mac`. The Simple source (`./examples/cs137/cs137_simple.mac`) is a computationally more efficient alternative that does not rely on the accuracy of the G4RadioactiveDecay class.

    /Source/Simple <double> <unit>

> Use a simple monoenergetic gamma-ray source with the specified energy.

    /Source/setX <double> <unit>
    /Source/setY <double> <unit>
    /Source/setZ <double> <unit>

> Set the position of the source (and capsule if present).

    /Source/Capsule/rotateX <double> <unit>
    /Source/Capsule/rotateY <double> <unit>
    /Source/Capsule/rotateZ <double> <unit>

> Orient the source capsule by rotating about X, Y, Z.

    /Source/Capsule/Construct

> Include the source capsule. Must be issued after the source positioning and capsule rotation commands.

### Lab Bench

Optionally, an epoxy resin lab bench can be included in simulations.

    /Bench/setX <double> <unit>
    /Bench/setY <double> <unit>
    /Bench/setZ <double> <unit>

> Set the position of the bench.

    /Brick/Construct

> Include the bench. Must be issued after the positioning commands.

### Lead Bricks

Optionally, 2" x 4" x 6" lead bricks can be included in simulations.

    /Brick/setX <double> <unit>
    /Brick/setY <double> <unit>
    /Brick/setZ <double> <unit>

> Set the position of the brick.

    /Brick/rotateX <double> <unit>
    /Brick/rotateY <double> <unit>
    /Brick/rotateZ <double> <unit>

> Orient the brick by rotating about X, Y, Z. (The order of these commands matters.)

    /Brick/GeometryFile <filename>

> Set the name of the optional geometry file. If this command is present, a brick  is placed for each line in the specified file.  Each line has the format:

        <X (mm)>  <Y (mm)>  <Z (mm)>  <X rotation (deg)>  <Y rotation (deg)>  <Z rotation (deg)>

> If a geometry file is specified, the positioning and rotation commands above are ignored.

    /Brick/Construct

> Include the brick. Must be issued after the positioning and rotation commands.

### Aluminum Targets

Optionally, cylindrical aluminum targets can be included in simulations.

    /Target/setR <double> <unit>

> Set the radius of the target(s).

    /Target/setL <double> <unit>

> Set the length of the target(s).

    /Target/setX <double> <unit>
    /Target/setY <double> <unit>
    /Target/setZ <double> <unit>

> Set the position of the target.

    /Target/rotateX <double> <unit>
    /Target/rotateY <double> <unit>
    /Target/rotateZ <double> <unit>

> Orient the target by rotating about X, Y, Z. (The order of these commands matters.)

    /Target/GeometryFile <filename>

> Set the name of the optional geometry file. If this command is present, a target is placed for each line in the specified file.  Each line has the format:

        <X (mm)>  <Y (mm)>  <Z (mm)>  <X rotation (deg)>  <Y rotation (deg)>  <Z rotation (deg)>

> If a geometry file is specified, the positioning and rotation commands above are ignored.

    /Target/Construct

> Include the target(s). Must be issued after the positioning and rotation commands.

## Output

Output is written to a text file specified with:

    /Output/Filename <File Name>

If energy was deposited one or more detectors in an event, a detected event block is written:

    D   <Number of Detectors Hit>  <Event>
    C   <Detector ID>   <Energy>   <Hit X>   <Hit Y>   <Hit Z>   <Full Energy>
    ...
    C   <Detector ID>   <Energy>   <Hit X>   <Hit Y>   <Hit Z>   <Full Energy>

- `<Event>` : event number
- `<Detector ID>` : integer identifying the detector registering the event
- `<Energy>` : energy deposited in keV
- `<Hit X>`, `<Hit Y>`, `<Hit Z>` : position in mm of the highest-energy hit
- `<Full Energy>` : == 1 if the gamma ray deposited its total energy in the detector (including full-energy pileup), ==  0 otherwise

Gamma-ray energies, emission positions, and emission directions are written for each gamma ray emitted in every event:

    E   <# of emitted gamma rays>   <Event #>
        <Energy>   <X>   <Y>   <Z>   <phi>   <theta>
        <Energy>   <X>   <Y>   <Z>   <phi>   <theta>
        ...

These emmitted gamma-ray blocks can be omitted from the output with:

    /Output/DetectorsOnly

## Electromagnetic Physics List

The electromagnetic physics list can be specified with:

    /PhysicsList/SelectEmPhysics <physics list name>

Available electromagnetic physics lists are `emstandard_opt0`, `emstandard_opt1`, `emstandard_opt2`, `emstandard_opt3`, **`emstandard_opt4` (default)**, `emlivermore`, `empenelope`, `emstandardGS`, `emlowenergy`, `emstandardWVI`, and `emstandardSS`. They are described [in the Geant4 Physics List Guide](https://geant4-userdoc.web.cern.ch/UsersGuides/PhysicsListGuide/BackupVersions/V10.7/html/electromagnetic/index.html). In addition, `emstandard_opt4_Atima` in a modified version of `emstandard_opt4` that uses ATIMA stopping powers for beam particles.

## Gamma-Ray Angular Correlations (see also /examples/sources/co60)

Gamma-ray angular correlations are built into the `G4PhotonEvaporation/G4GammaTransition` classes (starting with geant4.10.4). This functionality is disabled by default but can be enabled with:

    /PhysicsList/AngularCorrelations true

The `./examples/sources/co60` example simulates the angular correlations in the 4 -> 2 -> 0 cascade in <SUP>60</SUP>Ni.

## Visualization

Run the macro file `vis/vis.mac` an interactive session:

    $ UCCeBrA
    
    Idle> /control/execute vis/vis.mac
    Idle> exit

This generates a VRML 2 file named `g4_XX.wrl` which can be viewed with a VRML viewer (like view3dscene, FreeWRL, or mayavi2).

The macro file `./vis/trajectories.mac` illustrates how to add particle trajectories to visualizations.
