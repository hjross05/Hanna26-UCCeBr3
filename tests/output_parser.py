# tests/output_parser.py
#
# Shared utilities for the UCCeBrA test suite.
# Provides: binary discovery, processor info collection, macro patching,
# simulation subprocess execution, and ASCII output parsing.
#
# No third-party dependencies — Python standard library only.

import os
import shutil
import subprocess
import platform
import tempfile
import time

# ---------------------------------------------------------------------------
# Repository root: two levels up from this file (tests/output_parser.py)
# ---------------------------------------------------------------------------
REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Directory where test working directories are created. Using a path inside
# the repository guarantees write access on all platforms, avoiding macOS
# /var/folders permission issues. Gitignored.
TESTS_TMP = os.path.join(REPO_ROOT, "tests", "tmp")

# Geometry files that co60.mac references by relative path. They are copied
# into each co60 test directory so the binary can find them when run with
# cwd=<test_dir>.
CO60_GEO_SRC = os.path.join(REPO_ROOT, "examples", "co60")
CO60_GEO_FILES = ["demonstrator.geo", "bricks.geo"]


def find_binary():
    """
    Locate the compiled UCCeBrA binary.

    The Geant4 build system places the binary at:
      $G4WORKDIR/bin/$G4SYSTEM/UCCeBrA

    $G4WORKDIR defaults to ~/geant4_workdir if the environment variable is set,
    but may also be set to a custom path. $G4SYSTEM is typically 'Linux-g++'.
    When building in-source (G4WORKDIR not set), the binary lands inside the
    repository at bin/Linux-g++/UCCeBrA.

    Search order:
      1. $G4WORKDIR/bin/$G4SYSTEM/UCCeBrA  (canonical Geant4 out-of-source path)
      2. REPO_ROOT/bin/$G4SYSTEM/UCCeBrA   (in-source fallback)

    Returns the absolute path to the first binary found.
    Raises FileNotFoundError with a clear message listing all paths checked.
    """
    g4workdir = os.environ.get("G4WORKDIR", "")
    g4system  = os.environ.get("G4SYSTEM", "Linux-g++")

    candidates = []

    # Canonical out-of-source path: $G4WORKDIR/bin/$G4SYSTEM/UCCeBrA
    if g4workdir:
        candidates.append(os.path.join(g4workdir, "bin", g4system, "UCCeBrA"))

    # In-source fallback: <repo_root>/bin/$G4SYSTEM/UCCeBrA
    candidates.append(os.path.join(REPO_ROOT, "bin", g4system, "UCCeBrA"))

    for path in candidates:
        if os.path.isfile(path):
            return path

    checked = "\n  ".join(candidates)
    raise FileNotFoundError(
        f"UCCeBrA binary not found. Paths checked:\n  {checked}\n"
        "Please build the simulation first with 'make' at the repository root,\n"
        "ensuring Geant4 is sourced (source /path/to/geant4.sh)."
    )


def get_git_hash():
    """
    Return the current short git commit hash (7 characters).
    Returns 'unknown' if git is unavailable or the repo has no commits.
    """
    try:
        result = subprocess.run(
            ["git", "rev-parse", "--short", "HEAD"],
            capture_output=True, text=True, cwd=REPO_ROOT
        )
        return result.stdout.strip() if result.returncode == 0 else "unknown"
    except FileNotFoundError:
        return "unknown"


def get_processor_info():
    """
    Collect detailed processor information in a cross-platform way.
    Returns a dict with keys:
        cpu   (str)   - CPU model name
        cores (int)   - logical core count
        mhz   (float) - clock speed in MHz (0.0 if unavailable)
        os    (str)   - OS name and version
    Uses /proc/cpuinfo on Linux, sysctl on macOS, wmic on Windows.
    Falls back to platform module values if OS-specific tools fail.
    """
    system = platform.system()
    cpu = "unknown"
    mhz = 0.0
    cores = os.cpu_count() or 1
    os_str = f"{platform.system()} {platform.release()}"

    if system == "Linux":
        # Read /proc/cpuinfo for model name and MHz
        try:
            with open("/proc/cpuinfo", "r") as f:
                for line in f:
                    if line.startswith("model name") and cpu == "unknown":
                        cpu = line.split(":", 1)[1].strip()
                    if line.startswith("cpu MHz") and mhz == 0.0:
                        try:
                            mhz = float(line.split(":", 1)[1].strip())
                        except ValueError:
                            pass
        except OSError:
            cpu = platform.processor() or "unknown"

    elif system == "Darwin":
        # Use sysctl for CPU info on macOS
        try:
            r = subprocess.run(
                ["sysctl", "-n", "machdep.cpu.brand_string"],
                capture_output=True, text=True
            )
            if r.returncode == 0:
                cpu = r.stdout.strip()
            r2 = subprocess.run(
                ["sysctl", "-n", "hw.cpufrequency"],
                capture_output=True, text=True
            )
            if r2.returncode == 0 and r2.stdout.strip():
                mhz = float(r2.stdout.strip()) / 1_000_000
        except (FileNotFoundError, ValueError):
            cpu = platform.processor() or "unknown"

    elif system == "Windows":
        # Use wmic for CPU info on Windows
        try:
            r = subprocess.run(
                ["wmic", "cpu", "get", "Name,MaxClockSpeed,NumberOfLogicalProcessors",
                 "/format:csv"],
                capture_output=True, text=True
            )
            for line in r.stdout.splitlines():
                parts = line.strip().split(",")
                # wmic csv output: Node,MaxClockSpeed,Name,NumberOfLogicalProcessors
                if len(parts) >= 4 and parts[1].strip().isdigit():
                    mhz = float(parts[1].strip())
                    cpu = parts[2].strip()
                    try:
                        cores = int(parts[3].strip())
                    except ValueError:
                        pass
                    break
        except FileNotFoundError:
            cpu = platform.processor() or "unknown"

    return {"cpu": cpu, "cores": cores, "mhz": round(mhz, 1), "os": os_str}


def prepare_test_dir(name):
    """
    Create (or clear and recreate) tests/tmp/<name>/.
    Returns the absolute path to the directory.

    The directory is cleared on every run so the user always sees fresh results.
    Using a directory inside the repository avoids macOS /var/folders permission
    issues that arise when the binary tries to write to the system temp directory.
    """
    path = os.path.join(TESTS_TMP, name)
    if os.path.exists(path):
        # Remove all contents so the directory is clean for this run
        shutil.rmtree(path)
    os.makedirs(path)
    return path


def copy_co60_geometry(dest_dir):
    """
    Copy co60 geometry files (demonstrator.geo, bricks.geo) from examples/co60/
    into dest_dir.

    co60.mac references these files by relative path, so the binary must be run
    with cwd=dest_dir. Copying them in makes each test directory self-contained
    and lets users inspect all inputs in one place.
    """
    for fname in CO60_GEO_FILES:
        src = os.path.join(CO60_GEO_SRC, fname)
        dst = os.path.join(dest_dir, fname)
        shutil.copy2(src, dst)


def patch_mac(source_mac, output_filepath, n_events, dir=None):
    """
    Read a Geant4 macro file and return a path to a patched copy.
    Replaces:
      - '/run/beamOn <N>'       with '/run/beamOn <n_events>'
      - '/Output/Filename <x>'  with '/Output/Filename <output_filepath>'
        The replacement is inserted immediately before '/run/beamOn' regardless
        of where the original line appeared, ensuring the output file is set
        after all geometry and physics initialisation.

    output_filepath must be an absolute path.

    dir: directory in which to create the patched macro file. Defaults to the
         system temp directory. Pass a path inside the repository to avoid
         macOS /var/folders permission issues.

    Returns the path to the patched macro file.
    """
    assert os.path.isabs(output_filepath), \
        f"output_filepath must be absolute, got: {output_filepath}"

    with open(source_mac, "r") as f:
        lines = f.readlines()

    patched = []
    for line in lines:
        stripped = line.strip()
        if stripped.startswith("/run/beamOn"):
            # Always place /Output/Filename immediately before /run/beamOn,
            # regardless of where it appeared in the original macro. This
            # ensures the filename is set after /run/initialize and all
            # geometry setup, which matters for macros like co60.mac where
            # the original /Output/Filename line follows /run/initialize.
            patched.append(f"/Output/Filename {output_filepath}\n")
            patched.append(f"/run/beamOn {n_events}\n")
        elif stripped.startswith("/Output/Filename"):
            # Skip the original line — it is replaced before /run/beamOn above
            pass
        else:
            patched.append(line)

    # Write the patched macro into the specified directory (or system temp if
    # dir is None). Using dir=<test_subdir> keeps all test inputs together and
    # avoids macOS /var/folders write-permission issues.
    tmp = tempfile.NamedTemporaryFile(
        mode="w", suffix=".mac", delete=False, prefix="ucce_test_", dir=dir
    )
    tmp.writelines(patched)
    tmp.flush()
    tmp.close()
    return tmp.name


def run_simulation(binary_path, macro_path, cwd):
    """
    Run the UCCeBrA binary with the given macro file.
    cwd: the working directory for the subprocess (important for macros that
         reference geometry files by relative path, e.g. co60.mac references
         demonstrator.geo and bricks.geo).
    Returns (returncode, stdout, stderr, elapsed_seconds).
    elapsed_seconds is wall-clock time measured around the subprocess call.
    """
    start = time.time()
    result = subprocess.run(
        [binary_path, macro_path],
        capture_output=True, text=True, cwd=cwd
    )
    elapsed = time.time() - start
    return result.returncode, result.stdout, result.stderr, elapsed


def parse_output(filepath):
    """
    Parse a UCCeBrA ASCII output file into a list of event dictionaries.

    The UCCeBrA output format (defined in src/EventAction.cc) writes records
    event by event as the simulation runs. Each event consists of:

      D-line:  'D<NDetsHit><event_id>'  — detected event header (only if Nhits > 0)
      C-lines: 'C<DetID><Edep><X><Y><Z><FEP><GlobalTime>' — one per hit detector
      E-line:  'E<NEmittedGammas><event_id>' — emitted gamma header
      sub-records: '     <Energy><X><Y><Z><Phi><Theta>' — one per emitted gamma
                   (5 leading spaces, no record-type character)

    If /Output/DetectorsOnly is active, E-lines and sub-records are absent.

    Returns a list of event dicts. Each dict has:
      event_id      (int)
      n_dets_hit    (int)   — 0 if no D-line for this event
      hits          (list)  — list of hit dicts (empty if no D-line)
      n_emitted     (int)   — 0 if no E-line
      emitted_gammas(list)  — list of gamma dicts (empty if no E-line)
      raw_lines     (list)  — all raw lines for this event (for format checks)
    """
    events = {}  # keyed by event_id

    def get_or_create(eid):
        # Create a new event dict if this event_id has not been seen before
        if eid not in events:
            events[eid] = {
                "event_id": eid,
                "n_dets_hit": 0,
                "hits": [],
                "n_emitted": 0,
                "emitted_gammas": [],
                "raw_lines": [],
            }
        return events[eid]

    with open(filepath, "r") as f:
        lines = f.readlines()

    current_event_id = None  # tracks which event sub-records belong to

    for line in lines:
        raw = line.rstrip("\n")
        if not raw.strip():
            continue  # skip blank lines

        words = raw.split()

        if words[0] == "D":
            # D-line: detected event header
            # Format after split: ['D', '<NDetsHit>', '<event_id>']
            n_dets = int(words[1])
            eid = int(words[2])
            ev = get_or_create(eid)
            ev["n_dets_hit"] = n_dets
            ev["raw_lines"].append(raw)
            current_event_id = eid

        elif words[0] == "C":
            # C-line: per-detector hit record
            # Format after split: ['C', DetID, Edep, X, Y, Z, FEP, GlobalTime]
            hit = {
                "det_id":      int(words[1]),
                "edep":        float(words[2]),
                "x":           float(words[3]),
                "y":           float(words[4]),
                "z":           float(words[5]),
                "fep":         int(words[6]),
                "global_time": float(words[7]),
            }
            if current_event_id is not None:
                ev = get_or_create(current_event_id)
                ev["hits"].append(hit)
                ev["raw_lines"].append(raw)

        elif words[0] == "E":
            # E-line: emitted gamma header
            # Format after split: ['E', '<NEmittedGammas>', '<event_id>']
            n_emitted = int(words[1])
            eid = int(words[2])
            ev = get_or_create(eid)
            ev["n_emitted"] = n_emitted
            ev["raw_lines"].append(raw)
            current_event_id = eid

        elif raw.startswith("     "):
            # Gamma sub-record: exactly 5 leading spaces, no record-type character
            # Format after split: [Energy, X, Y, Z, Phi, Theta]
            gamma = {
                "energy": float(words[0]),
                "x":      float(words[1]),
                "y":      float(words[2]),
                "z":      float(words[3]),
                "phi":    float(words[4]),
                "theta":  float(words[5]),
            }
            if current_event_id is not None:
                ev = get_or_create(current_event_id)
                ev["emitted_gammas"].append(gamma)
                ev["raw_lines"].append(raw)

    return list(events.values())
