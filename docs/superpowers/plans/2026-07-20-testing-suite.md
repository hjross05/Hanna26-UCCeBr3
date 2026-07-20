# UCCeBrA Testing Suite Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a Python `unittest` testing suite with smoke tests, functional output validation, and benchmark logging for the UCCeBrA Geant4 simulation binary.

**Architecture:** A `tests/` directory at the repo root with a shared `output_parser.py` utility (binary discovery, mac patching, subprocess execution, output parsing, processor info), three `unittest` test modules (smoke, functional, benchmark), a persistent `benchmarks/event-counts.json` (git-tracked), and an appended `benchmark.log` (gitignored). Macros are generated at runtime by patching example `.mac` files.

**Tech Stack:** Python 3, `unittest`, `subprocess`, `platform`, `tempfile`, `json`, `time`

## Global Constraints

- Python standard library only — no `pytest`, no third-party packages
- Requires compiled binary at `bin/Linux-g++/UCCeBrA` relative to repo root
- Must work on Linux, macOS, and Windows
- co60 tests must be run with `cwd=examples/co60/` (relative path references in macro)
- cs137_simple output filename must be an absolute path (run from any directory)
- Smoke and functional tests: 1,000 events (`/run/beamOn 1000`)
- Benchmark tests: 10,000 events (`/run/beamOn 10000`)
- Event rate → `tests/benchmark.log` (appended, gitignored)
- Event count → `tests/benchmarks/event-counts.json` (tracked by git)
- If `event-counts.json` does not exist, create it from current run and pass
- Both log files include: CPU model, core count, clock speed (MHz), OS name/version
- Processor info collected via `platform` module + OS-specific tools (`/proc/cpuinfo`, `sysctl`, `wmic`) — no extra installs
- Never modify any C++ or Python source files in `src/` or `include/`
- Comments required on all new code explaining what it does and why

---

## File Structure

| File | Responsibility |
|------|---------------|
| `tests/output_parser.py` | Binary discovery, processor info, mac patching, subprocess runner, ASCII output parser |
| `tests/test_smoke.py` | Smoke tests: exit code 0, output file non-empty |
| `tests/test_functional.py` | Format, count, and physics sanity checks |
| `tests/test_benchmark.py` | Event rate + count benchmarking, log writing |
| `tests/run_tests.py` | Entry point: runs all suites, prints summary |
| `tests/__init__.py` | Makes tests/ a Python package |
| `tests/benchmarks/event-counts.json` | Persistent event count ground truth |
| `tests/benchmark.log` | Appended human-readable event rate log (gitignored) |

---

### Task 1: `output_parser.py` — binary discovery, processor info, mac patching, subprocess runner

**Files:**
- Create: `tests/output_parser.py`

**Interfaces:**
- Produces:
  - `find_binary() -> str` — absolute path to binary
  - `get_git_hash() -> str` — 7-char git hash
  - `get_processor_info() -> dict` with keys: `cpu` (str), `cores` (int), `mhz` (float), `os` (str)
  - `patch_mac(source_mac: str, output_filepath: str, n_events: int) -> str` — returns path to temp `.mac` file
  - `run_simulation(binary_path: str, macro_path: str, cwd: str) -> tuple` — returns `(returncode, stdout, stderr, elapsed_seconds)`

- [ ] **Step 1: Create `tests/` directory and `tests/output_parser.py`**

```python
# tests/output_parser.py
#
# Shared utilities for the UCCeBrA test suite.
# Provides: binary discovery, processor info collection, macro patching,
# simulation subprocess execution, and ASCII output parsing.
#
# No third-party dependencies — Python standard library only.

import os
import subprocess
import platform
import tempfile
import time

# ---------------------------------------------------------------------------
# Repository root: two levels up from this file (tests/output_parser.py)
# ---------------------------------------------------------------------------
REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def find_binary():
    """
    Locate the compiled UCCeBrA binary.
    Returns the absolute path to the binary.
    Raises FileNotFoundError with a clear message if not found.
    """
    binary_path = os.path.join(REPO_ROOT, "bin", "Linux-g++", "UCCeBrA")
    if not os.path.isfile(binary_path):
        raise FileNotFoundError(
            f"UCCeBrA binary not found at {binary_path}.\n"
            "Please build the simulation first with 'make' at the repository root,\n"
            "ensuring Geant4 is sourced (source /path/to/geant4.sh)."
        )
    return binary_path


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
                # wmic csv: Node,MaxClockSpeed,Name,NumberOfLogicalProcessors
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


def patch_mac(source_mac, output_filepath, n_events):
    """
    Read a Geant4 macro file and return a path to a patched temporary copy.
    Replaces:
      - '/run/beamOn <N>'       with '/run/beamOn <n_events>'
      - '/Output/Filename <x>'  with '/Output/Filename <output_filepath>'
    output_filepath must be an absolute path so the binary can be run
    from any working directory.
    Returns the path to the temporary macro file (caller must delete it).
    """
    assert os.path.isabs(output_filepath), \
        f"output_filepath must be absolute, got: {output_filepath}"

    with open(source_mac, "r") as f:
        lines = f.readlines()

    patched = []
    for line in lines:
        stripped = line.strip()
        if stripped.startswith("/run/beamOn"):
            patched.append(f"/run/beamOn {n_events}\n")
        elif stripped.startswith("/Output/Filename"):
            patched.append(f"/Output/Filename {output_filepath}\n")
        else:
            patched.append(line)

    # Write to a named temp file that persists until the caller deletes it
    tmp = tempfile.NamedTemporaryFile(
        mode="w", suffix=".mac", delete=False, prefix="ucce_test_"
    )
    tmp.writelines(patched)
    tmp.flush()
    tmp.close()
    return tmp.name


def run_simulation(binary_path, macro_path, cwd):
    """
    Run the UCCeBrA binary with the given macro file.
    cwd: the working directory for the subprocess (important for macros that
         reference geometry files by relative path, e.g. co60.mac).
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
```

- [ ] **Step 2: Verify the file is importable**

```bash
cd /home/nuc/Hanna26-UCCeBr3
python3 -c "from tests.output_parser import find_binary, get_git_hash, get_processor_info, patch_mac, run_simulation; print('import OK')"
```
Expected: `import OK`

- [ ] **Step 3: Commit**

```bash
git add tests/output_parser.py
git commit -m "tests: add output_parser utility module"
```

---

### Task 2: `output_parser.py` — ASCII output parser

**Files:**
- Modify: `tests/output_parser.py`

**Interfaces:**
- Consumes: `tests/output_parser.py` from Task 1
- Produces:
  - `parse_output(filepath: str) -> list` — list of event dicts, each with:
    - `event_id` (int)
    - `n_dets_hit` (int)
    - `hits` (list of dicts: `det_id` int, `edep` float, `x` float, `y` float, `z` float, `fep` int, `global_time` float)
    - `n_emitted` (int)
    - `emitted_gammas` (list of dicts: `energy` float, `x` float, `y` float, `z` float, `phi` float, `theta` float)
    - `raw_lines` (list of str)

- [ ] **Step 1: Append `parse_output` to `tests/output_parser.py`**

```python
def parse_output(filepath):
    """
    Parse a UCCeBrA ASCII output file into a list of event dictionaries.

    The UCCeBrA output format (defined in src/EventAction.cc) writes records
    event by event as the simulation runs. Each event consists of:

      D-line:  'D<NDetsHit><event_id>'  — detected event header
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
            # Format: D <NDetsHit> <event_id>
            n_dets = int(words[1])
            eid = int(words[2])
            ev = get_or_create(eid)
            ev["n_dets_hit"] = n_dets
            ev["raw_lines"].append(raw)
            current_event_id = eid

        elif words[0] == "C":
            # C-line: per-detector hit record
            # Format: C <DetID> <Edep> <X> <Y> <Z> <FEP> <GlobalTime>
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
            # Format: E <NEmittedGammas> <event_id>
            n_emitted = int(words[1])
            eid = int(words[2])
            ev = get_or_create(eid)
            ev["n_emitted"] = n_emitted
            ev["raw_lines"].append(raw)
            current_event_id = eid

        elif raw.startswith("     "):
            # Gamma sub-record: 5 leading spaces, no record-type character
            # Format: <Energy> <X> <Y> <Z> <Phi> <Theta>
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
```

- [ ] **Step 2: Verify parse_output is importable**

```bash
cd /home/nuc/Hanna26-UCCeBr3
python3 -c "from tests.output_parser import parse_output; print('parse_output OK')"
```
Expected: `parse_output OK`

- [ ] **Step 3: Commit**

```bash
git add tests/output_parser.py
git commit -m "tests: add ASCII output parser to output_parser"
```

---

### Task 3: Smoke tests (`test_smoke.py`)

**Files:**
- Create: `tests/test_smoke.py`

**Interfaces:**
- Consumes: `find_binary()`, `patch_mac()`, `run_simulation()` from `tests/output_parser.py`

- [ ] **Step 1: Write `tests/test_smoke.py`**

```python
# tests/test_smoke.py
#
# Smoke tests for UCCeBrA.
# Verifies that the binary runs without error and produces non-empty output
# for both the cs137_simple and co60 example scenarios.
#
# Each scenario is run once in setUpClass and shared across all tests in
# the class, so the binary is only invoked once per scenario.
#
# Event count: 1,000 (fast, suitable for TDD cycles).

import os
import unittest
import tempfile

from tests.output_parser import find_binary, patch_mac, run_simulation

# Paths to the example macro files, relative to repo root
REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CS137_MAC = os.path.join(REPO_ROOT, "examples", "cs137", "cs137_simple.mac")
CO60_MAC  = os.path.join(REPO_ROOT, "examples", "co60", "co60.mac")
CO60_CWD  = os.path.join(REPO_ROOT, "examples", "co60")

SMOKE_EVENTS = 1000


class SmokeTestCs137(unittest.TestCase):
    """
    Smoke tests for the cs137_simple scenario.
    Runs the simulation once and checks exit code and output file.
    """

    @classmethod
    def setUpClass(cls):
        cls.binary = find_binary()
        cls.tmpdir = tempfile.mkdtemp(prefix="ucce_smoke_cs137_")
        cls.outfile = os.path.join(cls.tmpdir, "smoke_cs137.out")
        cls.macfile = patch_mac(CS137_MAC, cls.outfile, SMOKE_EVENTS)
        cls.returncode, cls.stdout, cls.stderr, _ = run_simulation(
            cls.binary, cls.macfile, cwd=cls.tmpdir
        )

    @classmethod
    def tearDownClass(cls):
        # Clean up temp files after all tests in this class have run
        for f in [cls.macfile, cls.outfile]:
            if os.path.exists(f):
                os.remove(f)
        if os.path.exists(cls.tmpdir):
            os.rmdir(cls.tmpdir)

    def test_binary_exits_cleanly(self):
        """Binary must exit with return code 0."""
        self.assertEqual(
            self.returncode, 0,
            f"Binary exited with code {self.returncode}.\nstderr: {self.stderr}"
        )

    def test_output_file_created_and_nonempty(self):
        """Output file must exist and contain data."""
        self.assertTrue(
            os.path.isfile(self.outfile),
            f"Output file not found: {self.outfile}"
        )
        self.assertGreater(
            os.path.getsize(self.outfile), 0,
            "Output file exists but is empty."
        )


class SmokeTestCo60(unittest.TestCase):
    """
    Smoke tests for the co60 scenario.
    co60.mac references demonstrator.geo and bricks.geo by relative path,
    so the simulation must be run from examples/co60/.
    """

    @classmethod
    def setUpClass(cls):
        cls.binary = find_binary()
        cls.tmpdir = tempfile.mkdtemp(prefix="ucce_smoke_co60_")
        cls.outfile = os.path.join(cls.tmpdir, "smoke_co60.out")
        cls.macfile = patch_mac(CO60_MAC, cls.outfile, SMOKE_EVENTS)
        cls.returncode, cls.stdout, cls.stderr, _ = run_simulation(
            cls.binary, cls.macfile, cwd=CO60_CWD
        )

    @classmethod
    def tearDownClass(cls):
        for f in [cls.macfile, cls.outfile]:
            if os.path.exists(f):
                os.remove(f)
        if os.path.exists(cls.tmpdir):
            os.rmdir(cls.tmpdir)

    def test_binary_exits_cleanly(self):
        """Binary must exit with return code 0."""
        self.assertEqual(
            self.returncode, 0,
            f"Binary exited with code {self.returncode}.\nstderr: {self.stderr}"
        )

    def test_output_file_created_and_nonempty(self):
        """Output file must exist and contain data."""
        self.assertTrue(
            os.path.isfile(self.outfile),
            f"Output file not found: {self.outfile}"
        )
        self.assertGreater(
            os.path.getsize(self.outfile), 0,
            "Output file exists but is empty."
        )


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: Verify importable**

```bash
cd /home/nuc/Hanna26-UCCeBr3
python3 -c "import tests.test_smoke; print('import OK')"
```
Expected: `import OK`

- [ ] **Step 3: Commit**

```bash
git add tests/test_smoke.py
git commit -m "tests: add smoke tests for cs137_simple and co60"
```

---

### Task 4: Functional tests (`test_functional.py`)

**Files:**
- Create: `tests/test_functional.py`

**Interfaces:**
- Consumes: `find_binary()`, `patch_mac()`, `run_simulation()`, `parse_output()` from `tests/output_parser.py`

- [ ] **Step 1: Write `tests/test_functional.py`**

```python
# tests/test_functional.py
#
# Functional tests for UCCeBrA output format, event counts, and physics sanity.
# Tests both the cs137_simple (single detector, 662 keV direct gun) and
# co60 (9-detector array, radioactive decay) scenarios at 1,000 events.
#
# Each class runs the simulation once in setUpClass and parses the output.
# All tests in the class share the parsed result.

import os
import unittest
import tempfile

from tests.output_parser import find_binary, patch_mac, run_simulation, parse_output

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CS137_MAC = os.path.join(REPO_ROOT, "examples", "cs137", "cs137_simple.mac")
CO60_MAC  = os.path.join(REPO_ROOT, "examples", "co60", "co60.mac")
CO60_CWD  = os.path.join(REPO_ROOT, "examples", "co60")

FUNCTIONAL_EVENTS = 1000

# cs137_simple emits a monoenergetic 662 keV gamma (Cs-137 photopeak).
# No deposited energy can exceed the source energy.
CS137_MAX_ENERGY_KEV = 662.0
CS137_DETECTOR_ID    = 1       # single detector setup
CS137_EMITTED_ENERGY = 662.0
CS137_ENERGY_TOL     = 0.01    # keV tolerance for emitted energy check

# co60 emits a 1173/1332 keV cascade. Max possible deposited energy is 1332 keV.
CO60_MAX_ENERGY_KEV  = 1332.0
CO60_MIN_DETECTOR_ID = 1
CO60_MAX_DETECTOR_ID = 9


def _validate_raw_line_format(line):
    """
    Return True if a raw output line has a valid record type:
      - starts with 'D' (detected event header)
      - starts with 'C' (hit record)
      - starts with 'E' (emitted gamma header)
      - starts with 5 spaces (gamma sub-record)
      - is blank
    """
    if not line.strip():
        return True
    if line.startswith("D") or line.startswith("C") or line.startswith("E"):
        return True
    if line.startswith("     "):
        return True
    return False


class FunctionalTestCs137(unittest.TestCase):
    """
    Functional tests for the cs137_simple scenario.
    Validates output format, event counts, and physics bounds.
    """

    @classmethod
    def setUpClass(cls):
        cls.binary = find_binary()
        cls.tmpdir = tempfile.mkdtemp(prefix="ucce_func_cs137_")
        cls.outfile = os.path.join(cls.tmpdir, "func_cs137.out")
        cls.macfile = patch_mac(CS137_MAC, cls.outfile, FUNCTIONAL_EVENTS)
        rc, stdout, stderr, _ = run_simulation(
            cls.binary, cls.macfile, cwd=cls.tmpdir
        )
        if rc != 0:
            raise RuntimeError(
                f"Simulation failed (rc={rc}). Cannot run functional tests.\n"
                f"stderr: {stderr}"
            )
        cls.events = parse_output(cls.outfile)

        # Collect all raw lines for format checks
        with open(cls.outfile) as f:
            cls.raw_lines = f.readlines()

    @classmethod
    def tearDownClass(cls):
        for f in [cls.macfile, cls.outfile]:
            if os.path.exists(f):
                os.remove(f)
        if os.path.exists(cls.tmpdir):
            os.rmdir(cls.tmpdir)

    # --- Format checks ---

    def test_line_format(self):
        """Every line must be a valid record type: D, C, E, sub-record, or blank."""
        for i, line in enumerate(self.raw_lines):
            self.assertTrue(
                _validate_raw_line_format(line.rstrip("\n")),
                f"Line {i+1} has unexpected format: {line!r}"
            )

    def test_d_line_fields(self):
        """D-lines must have exactly 2 parseable fields: int NDetsHit, int event_id."""
        for ev in self.events:
            if ev["n_dets_hit"] > 0:
                self.assertIsInstance(ev["n_dets_hit"], int)
                self.assertIsInstance(ev["event_id"], int)

    def test_c_line_fields(self):
        """C-lines must have 7 parseable fields with correct types."""
        for ev in self.events:
            for hit in ev["hits"]:
                self.assertIsInstance(hit["det_id"],      int)
                self.assertIsInstance(hit["edep"],        float)
                self.assertIsInstance(hit["x"],           float)
                self.assertIsInstance(hit["y"],           float)
                self.assertIsInstance(hit["z"],           float)
                self.assertIsInstance(hit["fep"],         int)
                self.assertIsInstance(hit["global_time"], float)

    def test_e_line_fields(self):
        """E-lines must have 2 parseable fields: int NEmittedGammas, int event_id."""
        for ev in self.events:
            self.assertIsInstance(ev["n_emitted"], int)
            self.assertIsInstance(ev["event_id"],  int)

    def test_gamma_subrecord_fields(self):
        """Gamma sub-records must have 6 parseable float fields."""
        for ev in self.events:
            for g in ev["emitted_gammas"]:
                for key in ("energy", "x", "y", "z", "phi", "theta"):
                    self.assertIsInstance(g[key], float)

    # --- Count checks ---

    def test_event_count(self):
        """Total unique event IDs must equal FUNCTIONAL_EVENTS (1,000)."""
        self.assertEqual(
            len(self.events), FUNCTIONAL_EVENTS,
            f"Expected {FUNCTIONAL_EVENTS} events, got {len(self.events)}"
        )

    def test_d_c_line_consistency(self):
        """NDetsHit on each D-line must match the number of C-lines that follow."""
        for ev in self.events:
            self.assertEqual(
                ev["n_dets_hit"], len(ev["hits"]),
                f"Event {ev['event_id']}: D-line says {ev['n_dets_hit']} hits "
                f"but found {len(ev['hits'])} C-lines"
            )

    def test_e_gamma_consistency(self):
        """NEmittedGammas on each E-line must match the number of sub-records."""
        for ev in self.events:
            self.assertEqual(
                ev["n_emitted"], len(ev["emitted_gammas"]),
                f"Event {ev['event_id']}: E-line says {ev['n_emitted']} gammas "
                f"but found {len(ev['emitted_gammas'])} sub-records"
            )

    # --- Physics sanity checks ---

    def test_energy_bounds(self):
        """Deposited energy must be > 0 and <= 662.0 keV (source energy)."""
        for ev in self.events:
            for hit in ev["hits"]:
                self.assertGreater(hit["edep"], 0.0,
                    f"Event {ev['event_id']}: non-positive energy {hit['edep']}")
                self.assertLessEqual(hit["edep"], CS137_MAX_ENERGY_KEV,
                    f"Event {ev['event_id']}: energy {hit['edep']} exceeds source "
                    f"energy {CS137_MAX_ENERGY_KEV} keV")

    def test_detector_id(self):
        """All hits must be on detector 1 (single-detector setup)."""
        for ev in self.events:
            for hit in ev["hits"]:
                self.assertEqual(hit["det_id"], CS137_DETECTOR_ID,
                    f"Event {ev['event_id']}: unexpected detector ID {hit['det_id']}")

    def test_fep_flag(self):
        """FEP flag must be 0 or 1."""
        for ev in self.events:
            for hit in ev["hits"]:
                self.assertIn(hit["fep"], (0, 1),
                    f"Event {ev['event_id']}: invalid FEP flag {hit['fep']}")

    def test_global_time(self):
        """GlobalTime must be >= 0."""
        for ev in self.events:
            for hit in ev["hits"]:
                self.assertGreaterEqual(hit["global_time"], 0.0,
                    f"Event {ev['event_id']}: negative GlobalTime {hit['global_time']}")

    def test_emitted_energy(self):
        """Emitted gamma energy must be 662.0 +/- 0.01 keV (monoenergetic gun)."""
        for ev in self.events:
            for g in ev["emitted_gammas"]:
                self.assertAlmostEqual(
                    g["energy"], CS137_EMITTED_ENERGY, delta=CS137_ENERGY_TOL,
                    msg=f"Event {ev['event_id']}: emitted energy {g['energy']} "
                        f"not within {CS137_ENERGY_TOL} keV of {CS137_EMITTED_ENERGY}"
                )


class FunctionalTestCo60(unittest.TestCase):
    """
    Functional tests for the co60 scenario.
    Validates output format, event counts, and physics bounds for the
    full 9-detector CeBrA array with Co-60 radioactive decay source.
    """

    @classmethod
    def setUpClass(cls):
        cls.binary = find_binary()
        cls.tmpdir = tempfile.mkdtemp(prefix="ucce_func_co60_")
        cls.outfile = os.path.join(cls.tmpdir, "func_co60.out")
        cls.macfile = patch_mac(CO60_MAC, cls.outfile, FUNCTIONAL_EVENTS)
        rc, stdout, stderr, _ = run_simulation(
            cls.binary, cls.macfile, cwd=CO60_CWD
        )
        if rc != 0:
            raise RuntimeError(
                f"Simulation failed (rc={rc}). Cannot run functional tests.\n"
                f"stderr: {stderr}"
            )
        cls.events = parse_output(cls.outfile)

        with open(cls.outfile) as f:
            cls.raw_lines = f.readlines()

    @classmethod
    def tearDownClass(cls):
        for f in [cls.macfile, cls.outfile]:
            if os.path.exists(f):
                os.remove(f)
        if os.path.exists(cls.tmpdir):
            os.rmdir(cls.tmpdir)

    def test_line_format(self):
        """Every line must be a valid record type."""
        for i, line in enumerate(self.raw_lines):
            self.assertTrue(
                _validate_raw_line_format(line.rstrip("\n")),
                f"Line {i+1} has unexpected format: {line!r}"
            )

    def test_event_count(self):
        """Total unique event IDs must equal FUNCTIONAL_EVENTS (1,000)."""
        self.assertEqual(
            len(self.events), FUNCTIONAL_EVENTS,
            f"Expected {FUNCTIONAL_EVENTS} events, got {len(self.events)}"
        )

    def test_d_c_line_consistency(self):
        """NDetsHit on each D-line must match the number of C-lines that follow."""
        for ev in self.events:
            self.assertEqual(
                ev["n_dets_hit"], len(ev["hits"]),
                f"Event {ev['event_id']}: D-line says {ev['n_dets_hit']} hits "
                f"but found {len(ev['hits'])} C-lines"
            )

    def test_e_gamma_consistency(self):
        """NEmittedGammas on each E-line must match the number of sub-records."""
        for ev in self.events:
            self.assertEqual(
                ev["n_emitted"], len(ev["emitted_gammas"]),
                f"Event {ev['event_id']}: E-line says {ev['n_emitted']} gammas "
                f"but found {len(ev['emitted_gammas'])} sub-records"
            )

    def test_energy_bounds(self):
        """Deposited energy must be > 0 and <= 1332.0 keV."""
        for ev in self.events:
            for hit in ev["hits"]:
                self.assertGreater(hit["edep"], 0.0,
                    f"Event {ev['event_id']}: non-positive energy {hit['edep']}")
                self.assertLessEqual(hit["edep"], CO60_MAX_ENERGY_KEV,
                    f"Event {ev['event_id']}: energy {hit['edep']} exceeds "
                    f"{CO60_MAX_ENERGY_KEV} keV")

    def test_detector_id_range(self):
        """All detector IDs must be in range 1-9."""
        for ev in self.events:
            for hit in ev["hits"]:
                self.assertGreaterEqual(hit["det_id"], CO60_MIN_DETECTOR_ID,
                    f"Event {ev['event_id']}: detector ID {hit['det_id']} < 1")
                self.assertLessEqual(hit["det_id"], CO60_MAX_DETECTOR_ID,
                    f"Event {ev['event_id']}: detector ID {hit['det_id']} > 9")

    def test_fep_flag(self):
        """FEP flag must be 0 or 1."""
        for ev in self.events:
            for hit in ev["hits"]:
                self.assertIn(hit["fep"], (0, 1),
                    f"Event {ev['event_id']}: invalid FEP flag {hit['fep']}")

    def test_global_time(self):
        """GlobalTime must be >= 0."""
        for ev in self.events:
            for hit in ev["hits"]:
                self.assertGreaterEqual(hit["global_time"], 0.0,
                    f"Event {ev['event_id']}: negative GlobalTime {hit['global_time']}")


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: Verify importable**

```bash
cd /home/nuc/Hanna26-UCCeBr3
python3 -c "import tests.test_functional; print('import OK')"
```
Expected: `import OK`

- [ ] **Step 3: Commit**

```bash
git add tests/test_functional.py
git commit -m "tests: add functional tests for cs137_simple and co60"
```

---

### Task 5: Benchmark tests (`test_benchmark.py`) + `event-counts.json`

**Files:**
- Create: `tests/test_benchmark.py`
- Create: `tests/benchmarks/event-counts.json`

**Interfaces:**
- Consumes: all functions from `tests/output_parser.py`

- [ ] **Step 1: Create `tests/benchmarks/` and initial `event-counts.json`**

Create `tests/benchmarks/event-counts.json`:
```json
{
  "cs137_simple": {"expected_events": 10000},
  "co60":         {"expected_events": 10000}
}
```

- [ ] **Step 2: Write `tests/test_benchmark.py`**

```python
# tests/test_benchmark.py
#
# Benchmark tests for UCCeBrA.
# Measures wall-clock event rate and verifies event counts for two scenarios:
#   - cs137_simple: single detector, direct 662 keV gamma gun
#   - co60:         full 9-detector array, Co-60 radioactive decay
#
# Event count: 10,000 (larger run for more stable rate measurement).
#
# Results are written to two persistent files:
#   tests/benchmark.log          - human-readable rate log (appended, gitignored)
#   tests/benchmarks/event-counts.json - event count ground truth (tracked by git)
#
# If event-counts.json does not exist, it is created from the current run
# and the test passes. On subsequent runs, the test asserts that the event
# count matches the stored expected value.

import os
import json
import unittest
import tempfile
from datetime import datetime

from tests.output_parser import (
    find_binary, patch_mac, run_simulation, parse_output,
    get_git_hash, get_processor_info
)

REPO_ROOT        = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CS137_MAC        = os.path.join(REPO_ROOT, "examples", "cs137", "cs137_simple.mac")
CO60_MAC         = os.path.join(REPO_ROOT, "examples", "co60", "co60.mac")
CO60_CWD         = os.path.join(REPO_ROOT, "examples", "co60")
BENCHMARK_LOG    = os.path.join(REPO_ROOT, "tests", "benchmark.log")
COUNTS_JSON      = os.path.join(REPO_ROOT, "tests", "benchmarks", "event-counts.json")
BENCHMARK_EVENTS = 10000


def _load_event_counts():
    """
    Load event-counts.json. Returns the dict, or None if the file does not exist.
    """
    if not os.path.isfile(COUNTS_JSON):
        return None
    with open(COUNTS_JSON, "r") as f:
        return json.load(f)


def _save_event_counts(counts):
    """
    Write event-counts.json. Creates the benchmarks/ directory if needed.
    """
    os.makedirs(os.path.dirname(COUNTS_JSON), exist_ok=True)
    with open(COUNTS_JSON, "w") as f:
        json.dump(counts, f, indent=2)


def _append_benchmark_log(scenario, n_events, elapsed, proc_info, git_hash):
    """
    Append one line to benchmark.log in human-readable format.
    Format:
      YYYY-MM-DD HH:MM:SS | <scenario> | <n> events | <rate> events/s |
      bin: <hash> | CPU: <model> | cores: <n> | MHz: <mhz> | OS: <os>
    """
    rate = int(n_events / elapsed) if elapsed > 0 else 0
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    line = (
        f"{timestamp} | {scenario} | {n_events} events | {rate} events/s | "
        f"bin: {git_hash} | CPU: {proc_info['cpu']} | cores: {proc_info['cores']} | "
        f"MHz: {proc_info['mhz']} | OS: {proc_info['os']}\n"
    )
    with open(BENCHMARK_LOG, "a") as f:
        f.write(line)


def _run_benchmark(mac_path, cwd, scenario):
    """
    Run a benchmark scenario. Returns (n_events_in_output, elapsed_seconds).
    Raises RuntimeError if the binary exits non-zero.
    """
    binary = find_binary()
    tmpdir = tempfile.mkdtemp(prefix=f"ucce_bench_{scenario}_")
    outfile = os.path.join(tmpdir, f"bench_{scenario}.out")
    macfile = patch_mac(mac_path, outfile, BENCHMARK_EVENTS)

    rc, stdout, stderr, elapsed = run_simulation(binary, macfile, cwd=cwd)

    # Clean up temp macro
    if os.path.exists(macfile):
        os.remove(macfile)

    if rc != 0:
        raise RuntimeError(
            f"Benchmark simulation failed for {scenario} (rc={rc}).\n"
            f"stderr: {stderr}"
        )

    events = parse_output(outfile)
    n_events = len(events)

    # Clean up output file and temp directory
    if os.path.exists(outfile):
        os.remove(outfile)
    os.rmdir(tmpdir)

    return n_events, elapsed


class BenchmarkTests(unittest.TestCase):
    """
    Benchmark tests for UCCeBrA.
    Measures event rate and logs to benchmark.log.
    Asserts event count matches event-counts.json (creates file if absent).
    """

    def _run_and_record(self, scenario, mac_path, cwd):
        """
        Shared logic for a benchmark test:
        1. Run the simulation and measure elapsed time.
        2. Append results to benchmark.log.
        3. Assert or initialise event-counts.json.
        """
        n_events, elapsed = _run_benchmark(mac_path, cwd, scenario)

        proc_info = get_processor_info()
        git_hash  = get_git_hash()

        # Append to human-readable rate log (hardware-dependent, gitignored)
        _append_benchmark_log(scenario, n_events, elapsed, proc_info, git_hash)

        # Load or create event-counts.json (hardware-independent, git-tracked)
        counts = _load_event_counts()
        if counts is None:
            counts = {}

        if scenario not in counts:
            # First run for this scenario: store current count and pass
            counts[scenario] = {
                "expected_events": n_events,
                "cpu":   proc_info["cpu"],
                "cores": proc_info["cores"],
                "mhz":   proc_info["mhz"],
                "os":    proc_info["os"],
            }
            _save_event_counts(counts)
            # Test passes on first run (baseline established)
            return

        # Subsequent runs: assert count matches stored expected value
        expected = counts[scenario]["expected_events"]
        self.assertEqual(
            n_events, expected,
            f"{scenario}: expected {expected} events in output, got {n_events}. "
            f"This may indicate a change in the output format or simulation logic."
        )

    def test_cs137_benchmark(self):
        """Benchmark cs137_simple: single detector, 662 keV direct gamma gun."""
        self._run_and_record(
            scenario="cs137_simple",
            mac_path=CS137_MAC,
            cwd=os.path.join(REPO_ROOT, "tests"),
        )

    def test_co60_benchmark(self):
        """Benchmark co60: full 9-detector array, Co-60 radioactive decay."""
        self._run_and_record(
            scenario="co60",
            mac_path=CO60_MAC,
            cwd=CO60_CWD,
        )


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 3: Verify importable**

```bash
cd /home/nuc/Hanna26-UCCeBr3
python3 -c "import tests.test_benchmark; print('import OK')"
```
Expected: `import OK`

- [ ] **Step 4: Commit**

```bash
git add tests/test_benchmark.py tests/benchmarks/event-counts.json
git commit -m "tests: add benchmark tests and initial event-counts.json"
```

---

### Task 6: Entry point (`run_tests.py`), `__init__.py`, and `.gitignore` updates

**Files:**
- Create: `tests/__init__.py`
- Create: `tests/run_tests.py`
- Modify: `.gitignore`

- [ ] **Step 1: Create `tests/__init__.py`**

```python
# tests/__init__.py
# Makes tests/ a Python package so modules can import from each other
# using 'from tests.output_parser import ...' syntax.
```

- [ ] **Step 2: Write `tests/run_tests.py`**

```python
# tests/run_tests.py
#
# Entry point for the UCCeBrA test suite.
# Discovers and runs all three test modules in order:
#   1. Smoke tests       — binary runs, output file created
#   2. Functional tests  — format, counts, physics sanity
#   3. Benchmark tests   — event rate and event count logging
#
# Usage (run from repository root):
#   python tests/run_tests.py           # run all suites
#   python tests/test_smoke.py          # run smoke tests only
#   python tests/test_functional.py     # run functional tests only
#   python tests/test_benchmark.py      # run benchmark tests only

import os
import sys
import unittest

# Ensure repo root is on the path so 'from tests.output_parser import ...' works
REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if REPO_ROOT not in sys.path:
    sys.path.insert(0, REPO_ROOT)

import tests.test_smoke
import tests.test_functional
import tests.test_benchmark


def run_suite(module, label):
    """
    Load and run all tests from a module.
    Prints a one-line result summary.
    Returns (n_passed, n_total, was_successful).
    """
    loader = unittest.TestLoader()
    suite  = loader.loadTestsFromModule(module)
    n_total = suite.countTestCases()

    # Run with minimal verbosity; capture output to avoid cluttering summary
    buf = open(os.devnull, "w")
    runner = unittest.TextTestRunner(verbosity=0, stream=buf)
    result = runner.run(suite)
    buf.close()

    n_passed = n_total - len(result.failures) - len(result.errors)
    ok = result.wasSuccessful()
    status = "OK" if ok else "FAILED"
    print(f"{label:<25} {status} ({n_passed}/{n_total})")

    # Print failure details if any
    for test, traceback in result.failures + result.errors:
        print(f"  FAIL: {test}")
        last_line = [l for l in traceback.strip().splitlines() if l.strip()][-1]
        print(f"        {last_line}")

    return n_passed, n_total, ok


def main():
    # Change to repo root so relative paths in test modules resolve correctly
    os.chdir(REPO_ROOT)

    print()
    print("UCCeBrA Test Suite")
    print("=" * 40)

    suites = [
        (tests.test_smoke,       "Smoke tests"),
        (tests.test_functional,  "Functional tests"),
        (tests.test_benchmark,   "Benchmark tests"),
    ]

    total_passed = 0
    total_tests  = 0
    all_ok       = True

    for module, label in suites:
        n_passed, n_total, ok = run_suite(module, label)
        total_passed += n_passed
        total_tests  += n_total
        all_ok = all_ok and ok

    print()
    benchmark_log = os.path.join(REPO_ROOT, "tests", "benchmark.log")
    counts_json   = os.path.join(REPO_ROOT, "tests", "benchmarks", "event-counts.json")
    if os.path.exists(benchmark_log):
        print(f"Benchmark log:   tests/benchmark.log")
    if os.path.exists(counts_json):
        print(f"Event counts:    tests/benchmarks/event-counts.json")
    print()

    if all_ok:
        print(f"All tests passed ({total_passed}/{total_tests}).")
        sys.exit(0)
    else:
        print(f"FAILED: {total_tests - total_passed} test(s) failed.")
        sys.exit(1)


if __name__ == "__main__":
    main()
```

- [ ] **Step 3: Append to `.gitignore`**

Add these lines to `.gitignore`:
```
# Test suite outputs (hardware-dependent or temporary)
tests/benchmark.log
tests/*.out
tests/tmp/
tests/ucce_test_*.mac
```

- [ ] **Step 4: Verify importable**

```bash
cd /home/nuc/Hanna26-UCCeBr3
python3 -c "import tests.run_tests; print('import OK')"
```
Expected: `import OK`

- [ ] **Step 5: Commit**

```bash
git add tests/__init__.py tests/run_tests.py .gitignore
git commit -m "tests: add run_tests entry point, __init__.py, gitignore updates"
```

---

### Task 7: Write design spec to disk and update AGENTS.md

**Files:**
- Create: `docs/superpowers/specs/2026-07-20-testing-suite-design.md` (already written)
- Modify: `AGENTS.md` (Section 5, Testing subsection)

- [ ] **Step 1: Update AGENTS.md Section 5 (Testing)**

Replace the Testing subsection:
```
### Testing

A testing suite for functionality and performance is planned but not yet present.
There is currently no automated way to verify that a change is correct. "It compiles
and the example runs" is a necessary condition, not a sufficient one.
```

With:
```
### Testing

A Python `unittest` testing suite lives in `tests/`. Run it from the repository root:

    python tests/run_tests.py

Individual suites can be run directly:

    python tests/test_smoke.py        # binary runs, output file created
    python tests/test_functional.py   # format, counts, physics sanity
    python tests/test_benchmark.py    # event rate and event count logging

The suite requires the binary to be built (`make`) before running.
Benchmark event rates are logged to `tests/benchmark.log` (gitignored,
hardware-dependent). Event counts are stored in
`tests/benchmarks/event-counts.json` (tracked by git, hardware-independent).
```

- [ ] **Step 2: Commit**

```bash
git add docs/superpowers/specs/2026-07-20-testing-suite-design.md AGENTS.md
git commit -m "docs: add testing suite design spec and update AGENTS.md"
```
