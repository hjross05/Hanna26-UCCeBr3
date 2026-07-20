# AGENTS.md — UCCeBrA Contributor and AI Agent Guide

This document is for anyone working in this codebase: human contributors and AI
agents alike. Read it top to bottom if you are new here. If you are an AI agent,
you can also navigate directly to a section by heading.

---

## 1. Project Overview

UCCeBrA is a Monte Carlo simulation of gamma-ray detectors used in nuclear physics
experiments. It models arrays of CeBr3 (Cerium Bromide) scintillator crystals — the
CeBrA demonstrator array — and simulates how gamma rays interact with them. It is
built on Geant4, a C++ particle physics simulation toolkit developed at CERN.

The simulation is used by experimental nuclear physicists to understand detector
response, optimise experimental setups, and compare measured data against theoretical
predictions.

**Critical safety note for all contributors — human and AI:** A change that compiles
cleanly and runs without error can still produce physically incorrect simulation
output. There is currently no automated test suite to catch this. If you are unsure
whether a change is physically safe, stop and ask a human with domain knowledge
before proceeding.

The primary user documentation is `README.md` at the repository root. Read it to
understand how the simulation is configured and run.

---

## 2. Geant4 Primer

Geant4 is a C++ toolkit for simulating how particles travel through and interact with
matter. UCCeBrA uses it to simulate gamma rays being emitted by a radioactive source,
passing through air and structural materials, and depositing energy in CeBr3 detector
crystals.

You do not need a physics background to work in this codebase, but you do need to
understand the Geant4 patterns it uses.

### Core classes

Each of the following classes has a specific, well-defined role. Changing one without
understanding its role risks breaking the simulation in non-obvious ways.

- **`DetectorConstruction`** — builds the physical geometry of the simulation:
  detector positions, materials, and volumes. This is the most sensitive class in the
  codebase.
- **`PrimaryGeneratorAction`** — defines the particle source: what particles are
  emitted, from where, with what energy, and in what direction.
- **`EventAction`** and **`RunAction`** — collect and record results. `EventAction`
  runs once per simulated event (a single particle history); `RunAction` runs once
  per simulation job. These classes control what gets written to the output file.
- **`PhysicsList`** — defines which physical processes are active in the simulation
  (e.g. photoelectric effect, Compton scattering, pair production). Changing this
  changes what physics the simulation actually models.
- **`TrackingAction`** and **`StepMax`** — control how particle tracks are stepped
  through the geometry. Modifications here affect simulation accuracy and performance
  in subtle ways.

### The Messenger pattern

Every major class has a paired `*_Messenger` class (e.g. `CeBrA_Array` has
`CeBrA_Array_Messenger`). Messengers expose their paired class to Geant4 macro files
using a command interface. For example, a Messenger might register a command
`/CeBrA/addDetector` that the user calls from a `.mac` file.

If you change a command name, remove a command, or change its expected parameters in
a Messenger, you silently break any `.mac` file that uses that command — including
the example files in `examples/`. The simulation will still run, but it will not do
what the user intended.

### Macro files

Macro files (`.mac`) are plain-text scripts that configure and run the simulation
without recompiling. They call the commands registered by Messenger classes. The
examples in `examples/` each have one or more `.mac` files that demonstrate how to
use the simulation for a real experiment scenario.

### Why silent failure is the primary risk

Geant4 simulations do not crash or throw errors when the physics or geometry is
wrong. They run to completion and produce output that looks plausible. The only way
to detect a physically incorrect result is to know what the correct result should
look like — which requires domain expertise. This is why physics-touching changes
always require human review.

---

## 3. Repository Layout

```
UCCeBrA/
├── UCCeBrA.cc          # Main entry point — wires Geant4 together, rarely needs editing
├── GNUmakefile         # Build system — sets project name, embeds git hash
├── README.md           # Primary user documentation — read this first
├── git_hash            # Auto-generated at build time — never edit by hand
│
├── include/            # C++ header files (.hh) — one per class
├── src/                # C++ source files (.cc) — one per class, mirrors include/
│
├── examples/           # Illustrative usage examples
│   ├── cs137/          # Single CeBr3 detector with a 137Cs source
│   └── co60/           # CeBrA demonstrator array with a 60Co source,
│                       #   including angular correlations
│
├── vis/                # Visualization assets (VRML files, visualization macros)
│                       #   Not part of the physics — safe to ignore unless
│                       #   working on visualization
│
└── docs/               # Design specs and plans (not user documentation)
```

### Key files to know

- **`include/CADMesh.hh`** — a large (62 KB) third-party library for importing CAD
  geometry. Do not modify it.
- **`src/PhysicsList.cc`** — the largest and most physics-sensitive source file.
  See Danger Zones.
- **`src/Target_Chamber.cc`** — the detailed CeBrA target chamber model, contributed
  by Hanna J. Ross (summer 2026).
- **`src/EventAction.cc`** — defines what gets written to the output file, event by
  event. Tightly coupled to the Python post-processing scripts.

### The include/src pairing rule

Every class has exactly one `.hh` file in `include/` and one `.cc` file in `src/`.
They are always edited as a pair when changing a class interface. Never modify one
without checking whether the other also needs updating.

---

## 4. Build System

### Prerequisites

- Geant4 v10.7.4, installed and sourced (i.e. `source /path/to/geant4.sh` must have
  been run in your shell before building)
- A C++ compiler (GCC recommended)
- ROOT is required for post-processing only, not for building the simulation

### Building

```bash
make
```

The `GNUmakefile` embeds the current git commit hash and branch name into the binary.
Always build from a known git state. The compiled binary is placed at:

```
bin/Linux-g++/UCCeBrA
```

### Verifying a build

Run the simulation on a simple example macro to confirm it starts correctly:

```bash
./bin/Linux-g++/UCCeBrA examples/cs137/cs137_simple.mac
```

A successful run produces output in the current directory and exits cleanly.

### Cleaning

```bash
make clean
```

This removes build artifacts. The `git_hash` file is regenerated automatically on the
next build.

### What not to do

- Never edit `git_hash` by hand.
- Never commit build artifacts. The `.gitignore` excludes `.root`, `.wrl`, object
  files, and logs — respect these exclusions.
- Never commit a build and claim a task is complete on that basis alone.

**A successful build does not mean the physics is correct.** If your change touches
physics code, geometry, hit collection, or output format, a human must review and
verify the results before the work is considered done.

---

## 5. Code Conventions

### Header/source pairing

Always edit `.hh` and `.cc` files as a pair when changing a class interface. If you
add a method to a header, implement it in the source. If you remove one, remove it
from both.

### The Messenger pattern

When adding a new configurable parameter to a class, follow the existing Messenger
pattern:

1. Add the parameter and its setter to the class (`.hh` and `.cc`)
2. Register a new command in the paired `*_Messenger` constructor
3. Handle the command in `*_Messenger::SetNewValue()`
4. Update any relevant `.mac` files and examples that should use the new command

Never rename or remove existing Messenger commands — this silently breaks existing
`.mac` files.

### Naming

Class names use descriptive, underscore-separated names: `Target_Chamber`,
`CeBr3_2x2_Detector`, `CeBrA_Array_Messenger`. Follow this style exactly. Do not
introduce camelCase or abbreviations that are not already present in the codebase.

### Materials

All materials are defined centrally in `src/Materials.cc`. Never define a material
inline inside a detector or geometry class. If a new material is needed, add it to
`Materials.cc` and reference it from there.

### Readability over micro-optimisation

Where the performance impact is minimal, prefer clear and readable code over clever
or compact code. This codebase is used and maintained by physicists who may not have
a software engineering background. Code that is easy to read is less likely to
introduce errors during future maintenance.

### Comments on changes

Any code you add or modify — in C++ or Python — must include clear comments
explaining what it does and why. Do not leave changes uncommented. A future
contributor should be able to understand your change without reading the surrounding
code in detail.

### Testing

A Python `unittest` testing suite lives in `tests/`. Run it from the repository root:

```bash
python tests/run_tests.py
```

Individual suites can be run directly:

```bash
python tests/test_smoke.py        # binary runs, output file created
python tests/test_functional.py   # format, counts, physics sanity
python tests/test_benchmark.py    # event rate and event count logging
```

The suite requires the binary to be built (`make`) before running.
Benchmark event rates are logged to `tests/benchmark.log` (gitignored,
hardware-dependent). Event counts are stored in
`tests/benchmarks/event-counts.json` (tracked by git, hardware-independent).

"It compiles and the example runs" is a necessary condition, not a sufficient one.
Physics correctness still requires human review.

---

## 6. Danger Zones

This section lists areas of the codebase that require special care. Read it before
making any change.

### Hard stops — do not modify without human review and physical verification

These files control the physics and geometry of the simulation. A change here that is
physically wrong will produce incorrect output with no visible error. Always pause and
get a human with domain knowledge to review and verify before proceeding.

| File(s) | Why it is sensitive |
|---------|---------------------|
| `src/PhysicsList.cc`, `include/PhysicsList.hh` | Defines which physics processes are active. Changing this changes what the simulation actually models. |
| `src/EventAction.cc`, `include/EventAction.hh` | Controls what gets written to the output file, event by event. Changes here also break the Python sort scripts. |
| `src/RunAction.cc`, `include/RunAction.hh` | Controls per-run data collection and summary output. |
| `src/TrackingAction.cc`, `include/TrackingAction.hh` | Controls how particle tracks are stepped. Affects accuracy and performance. |
| `src/StepMax.cc`, `include/StepMax.hh` | Sets the maximum step length for particle tracking. |
| `src/DetectorConstruction.cc`, `include/DetectorConstruction.hh` | Assembles the full geometry. Changes affect detector response. |
| All `CeBr3_*_Detector` files | Define individual detector geometry and sensitive volumes. |
| `src/Target_Chamber.cc`, `include/Target_Chamber.hh` | Detailed chamber model. Geometry errors affect detector solid angle and response. |
| `src/Experimental_Hall.cc`, `include/Experimental_Hall.hh` | Defines the surrounding environment. |
| `src/CeBrA_Array.cc`, `include/CeBrA_Array.hh` | Assembles the full detector array. |
| Anything touching hit collection or energy deposition | Any class that records which detector was hit and how much energy was deposited is physics-critical. |

**Reminder:** no automated verification exists. You cannot confirm correctness of
physics-touching changes by building and running. Always escalate to a human.

### Cautions — modify carefully

- **Any `*_Messenger` class:** changing a command name, signature, or removing a
  command silently breaks any `.mac` file that uses it. Always check all `.mac` files
  in `examples/` and `vis/` after a Messenger change.

### Never touch

- **`include/CADMesh.hh`** — third-party library, 62 KB. It is not part of the
  project's own code and must not be modified.
- **`git_hash`** — auto-generated at build time. Any hand-edit will be overwritten on
  the next build and should never be committed.

---

## 7. Python Post-Processing

### How the pipeline works

The simulation writes output to an ASCII file **event by event as it runs** — not as
a single file at the end. Each line in the output file corresponds to one simulated
event. The format of these lines is defined in `src/EventAction.cc`.

After a simulation run (or during a long one), the output file is processed by a
Python sort script. Each example in `examples/` has its own sort script:

- `examples/cs137/cs137_sim_sort.py`
- `examples/co60/co60_sim_sort.py`

These scripts read the ASCII output, apply energy cuts and coincidence conditions, and
produce ROOT histograms (`.root` files) for physics analysis.

### Output format coupling

The ASCII output columns are defined in `src/EventAction.cc`. The Python sort scripts
parse those columns by position. **If you change what `EventAction.cc` writes —
adding, removing, or reordering columns — you must update the sort scripts in sync.**
They will not produce an error if the format changes; they will silently read the
wrong columns.

### Prerequisites for post-processing

- Python 3
- ROOT with PyROOT bindings (separate from the Geant4 build prerequisites)

### Risk level

The Python scripts are self-contained data processing. They contain no physics logic
of their own. They are lower risk than the C++ simulation code, but output format
changes must always be coordinated across both `EventAction.cc` and the relevant sort
script.

---

## 8. OpenCode / Superpowers Guidance

This section is specifically for AI agents working in this codebase via OpenCode with
the Superpowers plugin.

### Configuration

- **Tool:** OpenCode with the Superpowers plugin
- **Model:** Claude Sonnet 4.6 via API

### Work incrementally

Make small, focused changes. Build after every meaningful change to catch compilation
errors early. Do not accumulate a large set of changes before verifying the build.

### When to stop and ask

Stop and request human review before proceeding if your change touches any of:

- Physics processes (`PhysicsList`)
- Detector or chamber geometry
- Hit collection or energy deposition logic
- The output format in `EventAction.cc`
- Any `*_Messenger` command interface

A successful build is not sufficient justification to proceed with physics-touching
changes.

### Superpowers skills to invoke

| Situation | Skill to invoke |
|-----------|----------------|
| Before proposing any fix for a bug or unexpected behaviour | `systematic-debugging` |
| Before adding any new feature or component | `brainstorming` |
| Before claiming any task is complete | `verification-before-completion` |

### What "done" means

"Done" means the change compiles, the example runs, and — if the change touches
physics, geometry, or output format — a human has reviewed and confirmed the result is
physically correct. Always note explicitly what human verification is still needed
when handing off physics-touching work.

### Commit hygiene

- Never commit build artifacts (`.root`, `.wrl`, object files, logs)
- Never hand-edit or commit `git_hash`
- Write commit messages that match the existing style: imperative mood, lowercase,
  descriptive (e.g. `add target chamber collision geometry`, `fix CeBr3 3x3 placement`)
- Stage only the files relevant to the change
