# UCCeBrA Test Suite Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Create `tests/benchmark.py`, a single test driver modelled on UCGretina's test suite, and update `GNUmakefile` and `AGENTS.md` to match.

**Architecture:** A single Python 3 script (`tests/benchmark.py`) driven by `--mode` flags. Three modes: `smoke` (100 events, exit/fatal/rate check), `sources` (1000 events, line-count vs `baselines.json`), `benchmark` (10,000 events, events/sec to `benchmark.log`). Support files for co60 are copied into isolated `tests/tmp/<name>/` directories. No third-party dependencies.

**Tech Stack:** Python 3 standard library only (`argparse`, `subprocess`, `json`, `math`, `os`, `re`, `shutil`, `datetime`). Geant4 Makefile build system.

## Global Constraints

- Python 3 standard library only — no pip installs, no pytest, no third-party packages.
- Binary located via `$G4WORKDIR/bin/$G4SYSTEM/UCCeBrA` — Geant4 env vars must be set before running.
- All file paths use `os.path.join` — no hardcoded separators.
- Output file for every test: `output.out` in the test's working directory.
- Baseline tolerance: `|observed - baseline| <= 2 * sqrt(baseline)`.
- `baselines.json` is git-tracked. `benchmark.log` and `tests/tmp/` are gitignored (already in `.gitignore`).
- Original example macros (`examples/cs137/cs137_simple.mac`, `examples/co60/co60.mac`) must never be modified.
- Co60 support files (`demonstrator.geo`, `bricks.geo`) are copied from `examples/co60/` into the test working directory.
- Script lives at `tests/benchmark.py`; all paths are computed relative to `PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))`.

---

## Task 1: Create `tests/benchmark.py` — infrastructure functions

**Files:**
- Create: `tests/benchmark.py`

**Interfaces:**
- Produces:
  - `find_binary(name: str) -> str` — returns path or exits
  - `setup_workdir(test_name: str, example_path: str, support_files: list[str]) -> str` — returns workdir path
  - `write_base_macro(base_macro_path: str, example_path: str, output_command: str, workdir: str) -> None`
  - `write_run_macro(base_macro_path: str, n_events: int, wrapper_path: str) -> None`
  - `run_sim(binary: str, macro_path: str, workdir: str) -> tuple[str, str, int]`
  - `parse_events_per_sec(stdout: str) -> float | None`
  - `check_fatal(stdout: str, stderr: str) -> bool`
  - `count_lines(filepath: str) -> int`
  - `load_baselines() -> dict`
  - `save_baselines(data: dict) -> None`
  - `check_baseline(name: str, observed: int, baselines: dict) -> tuple[bool, str]`
  - `get_git_info() -> tuple[str, str]`
  - `get_cpu_info() -> str`
  - `append_benchmark_log(rows: list) -> None`

- [ ] **Step 1: Create `tests/` directory and write the infrastructure section of `benchmark.py`**

Write `tests/benchmark.py` with the following complete content up through the infrastructure functions. This is modelled directly on UCGretina's `tests/benchmark.py`:

```python
#!/usr/bin/env python3
"""UCCeBrA testing suite: functional tests and performance benchmarks.

Modelled on the UCGretina test suite. Run via make targets:
  make test            # smoke + sources
  make test-smoke      # quick sanity check (100 events)
  make test-functional # line-count regression (1000 events)
  make test-benchmark  # events/sec timing (10000 events)
"""

import argparse
import datetime
import json
import math
import os
import re
import subprocess
import sys
import shutil

# Paths relative to the repository root.
PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TESTS_DIR = os.path.join(PROJECT_ROOT, "tests")
TMP_DIR = os.path.join(TESTS_DIR, "tmp")
BASELINES_FILE = os.path.join(TESTS_DIR, "baselines.json")
BENCHMARK_LOG = os.path.join(PROJECT_ROOT, "benchmark.log")


def find_binary(name):
    """Locate the UCCeBrA binary in $G4WORKDIR/bin/$G4SYSTEM/.

    Exits with an error message if the binary is not found or if the
    required Geant4 environment variables are not set.
    """
    g4workdir = os.environ.get("G4WORKDIR")
    g4system = os.environ.get("G4SYSTEM")
    if not g4workdir or not g4system:
        print("ERROR: G4WORKDIR and G4SYSTEM must be set.", file=sys.stderr)
        sys.exit(1)
    path = os.path.join(g4workdir, "bin", g4system, name)
    if not os.path.isfile(path):
        print(f"ERROR: Binary not found: {path}", file=sys.stderr)
        sys.exit(1)
    return path


def setup_workdir(test_name, example_path, support_files):
    """Create an isolated working directory for a test run.

    Creates tests/tmp/<test_name>/, wiping it first if it already exists,
    then copies each file in support_files from the example's directory
    into the working directory.

    Args:
        test_name: Unique name for this test (used as directory name).
        example_path: Repo-relative path to the example macro (used to
            locate the directory containing support files).
        support_files: List of filenames to copy from the example directory.

    Returns:
        Absolute path to the working directory.
    """
    workdir = os.path.join(TMP_DIR, test_name)
    if os.path.exists(workdir):
        shutil.rmtree(workdir)
    os.makedirs(workdir, exist_ok=True)
    src_dir = os.path.join(PROJECT_ROOT, os.path.dirname(example_path))
    for fname in support_files:
        shutil.copy(os.path.join(src_dir, fname), workdir)
    return workdir


def write_base_macro(base_macro_path, example_path, output_command, workdir):
    """Write a base macro for a test, stripped of run-control lines.

    Reads the original example macro, drops any /Output/Filename and
    /run/beamOn lines (these are controlled by the test), and appends
    the test's output_command. Writes the result to
    <workdir>/<base_macro_path>.

    Args:
        base_macro_path: Filename (not full path) for the output macro.
        example_path: Repo-relative path to the original example macro.
        output_command: Line to append, e.g. "/Output/Filename output.out".
        workdir: Absolute path to the test's working directory.
    """
    with open(os.path.join(PROJECT_ROOT, example_path), "r") as f:
        lines = f.readlines()
    with open(os.path.join(workdir, base_macro_path), "w") as f:
        for line in lines:
            # Drop the output filename and event count — the test controls these.
            if ("/Output/Filename" not in line) and ("/run/beamOn" not in line):
                f.write(line)
        if output_command:
            f.write(output_command + "\n")


def write_run_macro(base_macro_path, n_events, wrapper_path):
    """Write a two-line wrapper macro that runs a base macro then fires events.

    Args:
        base_macro_path: Filename of the base macro (relative to the
            working directory, as Geant4 resolves it).
        n_events: Number of events to simulate.
        wrapper_path: Absolute path where the wrapper macro is written.
    """
    with open(wrapper_path, "w") as f:
        f.write(f"/control/execute {base_macro_path}\n")
        f.write(f"/run/beamOn {n_events}\n")


def run_sim(binary, macro_path, workdir):
    """Run the simulation binary with a macro file.

    Args:
        binary: Absolute path to the UCCeBrA binary.
        macro_path: Absolute path to the wrapper macro.
        workdir: Directory to run the simulation in (cwd).

    Returns:
        Tuple of (stdout, stderr, returncode).
    """
    result = subprocess.run(
        [binary, macro_path],
        cwd=workdir,
        capture_output=True,
        text=True,
    )
    return result.stdout, result.stderr, result.returncode


def parse_events_per_sec(stdout):
    """Extract events/sec from the Geant4 end-of-run summary line.

    The EventAction progress lines and the RunAction end-of-run line both
    print "NNN events/s". We take the last match to get the final value.

    Returns float, or None if the pattern is not found.
    """
    matches = re.findall(r"([\d.]+)\s+events/s", stdout)
    if matches:
        return float(matches[-1])
    return None


def check_fatal(stdout, stderr):
    """Return True if any fatal-error indicator is present in the output."""
    fatal_patterns = [
        "Fatal Exception",
        "Segmentation fault",
        "FatalException",
        "G4Exception : Fatal",
    ]
    combined = stdout + stderr
    return any(p in combined for p in fatal_patterns)


def count_lines(filepath):
    """Return the number of lines in a file using wc -l."""
    result = subprocess.run(["wc", "-l", filepath], capture_output=True, text=True)
    if result.returncode != 0:
        return 0
    return int(result.stdout.strip().split()[0])


def load_baselines():
    """Load baselines.json and return the test-name -> line-count dict.

    Returns an empty dict if the file does not exist.
    Strips the '_meta' provenance key so callers only see test entries.
    """
    if not os.path.isfile(BASELINES_FILE):
        return {}
    with open(BASELINES_FILE) as f:
        data = json.load(f)
    data.pop("_meta", None)
    return data


def save_baselines(data):
    """Write baselines.json with provenance metadata.

    The '_meta' key records the git hash, branch, and CPU so that
    baselines can be traced back to the run that set them.
    """
    git_hash, git_branch = get_git_info()
    cpu = get_cpu_info()
    out = {
        "_meta": {
            "git_hash": git_hash,
            "git_branch": git_branch,
            "cpu": cpu,
        }
    }
    out.update(data)
    with open(BASELINES_FILE, "w") as f:
        json.dump(out, f, indent=2)
        f.write("\n")


def check_baseline(name, observed, baselines):
    """Compare an observed line count against a stored baseline.

    Uses a 2*sqrt(N) Poisson tolerance: Monte Carlo statistical variation
    between runs is expected to be within ~2 standard deviations.

    If no baseline exists for this test, records the observed count as
    the new baseline and returns (True, '[BASELINE SET] ...').

    Returns:
        Tuple of (passed: bool, message: str).
    """
    if name not in baselines:
        baselines[name] = observed
        return True, f"[BASELINE SET] {name}: {observed} lines"
    baseline = baselines[name]
    tolerance = 2 * math.sqrt(baseline)
    if abs(observed - baseline) <= tolerance:
        return True, (f"[PASS] {name:<30} output lines={observed}  "
                      f"baseline={baseline}  tolerance=\xb1{tolerance:.0f}")
    else:
        return False, (f"[FAIL] {name:<30} output lines={observed}  "
                       f"baseline={baseline}  tolerance=\xb1{tolerance:.0f}  "
                       f"** out of range **")


def get_git_info():
    """Return (6-char commit hash, branch name) for provenance metadata."""
    try:
        hash_ = subprocess.check_output(
            ["git", "rev-parse", "HEAD"],
            cwd=PROJECT_ROOT, text=True
        ).strip()[:6]
        branch = subprocess.check_output(
            ["git", "rev-parse", "--abbrev-ref", "HEAD"],
            cwd=PROJECT_ROOT, text=True
        ).strip()
    except subprocess.CalledProcessError:
        hash_, branch = "unknown", "unknown"
    return hash_, branch


def get_cpu_info():
    """Return a compact CPU identifier string.

    Tries /proc/cpuinfo (Linux), then sysctl (macOS), then hostname.
    """
    # Linux
    try:
        with open("/proc/cpuinfo") as f:
            for line in f:
                if line.startswith("model name"):
                    return line.split(":", 1)[1].strip()
    except OSError:
        pass
    # macOS
    try:
        result = subprocess.run(
            ["sysctl", "-n", "machdep.cpu.brand_string"],
            capture_output=True, text=True
        )
        if result.returncode == 0 and result.stdout.strip():
            return result.stdout.strip()
    except FileNotFoundError:
        pass
    import socket
    return socket.gethostname()


def append_benchmark_log(rows):
    """Append timing rows to benchmark.log (TSV).

    Writes a header line the first time the file is created.
    Each row is a tuple of values that will be joined with tabs.
    Columns: date, git_hash, git_branch, cpu, variant, events, events_per_sec.
    """
    header = "date\tgit_hash\tgit_branch\tcpu\tvariant\tevents\tevents_per_sec\n"
    write_header = not os.path.isfile(BENCHMARK_LOG)
    with open(BENCHMARK_LOG, "a") as f:
        if write_header:
            f.write(header)
        for row in rows:
            f.write("\t".join(str(v) for v in row) + "\n")
```

- [ ] **Step 2: Verify the file is syntactically valid**

```bash
python3 -c "import ast; ast.parse(open('tests/benchmark.py').read()); print('OK')"
```

Expected output: `OK`

- [ ] **Step 3: Commit**

```bash
cd /home/nuc/Hanna26-UCCeBr3
git add tests/benchmark.py
git commit -m "add test suite infrastructure functions"
```

---

## Task 2: Add smoke test mode to `tests/benchmark.py`

**Files:**
- Modify: `tests/benchmark.py` (append smoke constants, `run_smoke`, `_check_run_criteria`)

**Interfaces:**
- Consumes: all functions from Task 1
- Produces:
  - `SMOKE_EVENTS = 100`
  - `SMOKE_CASES` — list of `(test_name, binary_name, macro_file, example_path, support_files)`
  - `run_smoke() -> None` — prints results, exits 1 on any failure
  - `_check_run_criteria(test_name, stdout, stderr, returncode) -> tuple[bool, str]`

- [ ] **Step 1: Append smoke test code to `tests/benchmark.py`**

Add the following after the `append_benchmark_log` function:

```python
# ---------------------------------------------------------------------------
# Smoke tests
# ---------------------------------------------------------------------------

SMOKE_EVENTS = 100

# Each entry: (test_name, binary_name, base_macro_filename, example_path,
#              support_files)
# support_files are copied from the example's directory into tests/tmp/<name>/.
SMOKE_CASES = [
    (
        "smoke_cs137",
        "UCCeBrA",
        "func_smoke_cs137.mac",
        "examples/cs137/cs137_simple.mac",
        [],  # cs137_simple.mac needs no extra geometry files
    ),
    (
        "smoke_co60",
        "UCCeBrA",
        "func_smoke_co60.mac",
        "examples/co60/co60.mac",
        ["demonstrator.geo", "bricks.geo"],  # geometry files for the demonstrator array
    ),
]


def _check_run_criteria(test_name, stdout, stderr, returncode):
    """Check the four smoke pass criteria for a simulation run.

    Checks:
      1. Exit code is 0.
      2. No fatal error strings in stdout or stderr.
      3. End-of-run events/s line is present in stdout.
      4. Events/sec > 0.

    Returns:
        Tuple of (passed: bool, message: str).
    """
    if returncode != 0:
        return False, f"[FAIL] {test_name:<30} exit code {returncode}"
    if check_fatal(stdout, stderr):
        return False, f"[FAIL] {test_name:<30} fatal error in output"
    eps = parse_events_per_sec(stdout)
    if eps is None:
        return False, f"[FAIL] {test_name:<30} end-of-run line not found"
    if eps <= 0:
        return False, f"[FAIL] {test_name:<30} events/sec = {eps}"
    return True, f"[PASS] {test_name:<30} {eps:.0f} events/s"


def run_smoke():
    """Run all smoke tests and exit 1 if any fail.

    Each smoke test runs SMOKE_EVENTS events and checks that the binary
    exits cleanly with a valid events/sec rate. No output file validation.
    """
    print(f"\n=== test-smoke ({SMOKE_EVENTS} events) ===")
    failures = 0

    for test_name, binary_name, macro_file, example_path, support_files in SMOKE_CASES:
        binary = find_binary(binary_name)
        workdir = setup_workdir(test_name, example_path, support_files)
        write_base_macro(macro_file, example_path,
                         "/Output/Filename output.out", workdir)
        wrapper = os.path.join(workdir, "run.mac")
        write_run_macro(macro_file, SMOKE_EVENTS, wrapper)
        stdout, stderr, returncode = run_sim(binary, wrapper, workdir)
        ok, msg = _check_run_criteria(test_name, stdout, stderr, returncode)
        print(msg)
        if not ok:
            failures += 1

    if failures:
        print(f"\n{failures} FAILED")
        sys.exit(1)
    else:
        print("\nAll smoke tests passed.\n")
```

- [ ] **Step 2: Verify syntax**

```bash
python3 -c "import ast; ast.parse(open('tests/benchmark.py').read()); print('OK')"
```

Expected: `OK`

- [ ] **Step 3: Commit**

```bash
cd /home/nuc/Hanna26-UCCeBr3
git add tests/benchmark.py
git commit -m "add smoke test mode to benchmark.py"
```

---

## Task 3: Add functional (sources) and benchmark test modes to `tests/benchmark.py`

**Files:**
- Modify: `tests/benchmark.py` (append sources constants, `run_functional`, benchmark constants, `run_benchmark`, `update_baselines`, `main`)

**Interfaces:**
- Consumes: all functions from Tasks 1–2
- Produces:
  - `FUNCTIONAL_EVENTS = 1000`
  - `FUNCTIONAL_CASES` — dict mapping mode string to list of test tuples
  - `run_functional(mode: str) -> None`
  - `BENCHMARK_CASES` — list of benchmark tuples
  - `run_benchmark(n_events: int) -> None`
  - `update_baselines() -> None`
  - `main() -> None`

- [ ] **Step 1: Append functional + benchmark + main to `tests/benchmark.py`**

Add the following after `run_smoke`:

```python
# ---------------------------------------------------------------------------
# Functional (sources) tests
# ---------------------------------------------------------------------------

FUNCTIONAL_EVENTS = 1000

# Maps mode name -> list of (test_name, binary_name, base_macro_filename,
#                            example_path, support_files, output_filename)
FUNCTIONAL_CASES = {
    "sources": [
        (
            "sources_cs137",
            "UCCeBrA",
            "func_sources_cs137.mac",
            "examples/cs137/cs137_simple.mac",
            [],  # no extra geometry files needed
            "output.out",
        ),
        (
            "sources_co60",
            "UCCeBrA",
            "func_sources_co60.mac",
            "examples/co60/co60.mac",
            ["demonstrator.geo", "bricks.geo"],  # demonstrator array geometry
            "output.out",
        ),
    ],
}


def run_functional(mode):
    """Run functional tests for a given mode, comparing output line counts
    against stored baselines.

    Line count is a proxy for detection rate (more detections = more output
    lines). The tolerance is 2*sqrt(baseline) to accommodate Monte Carlo
    statistical variation between runs.

    Baselines are stored in tests/baselines.json. On first run, each test
    auto-records its result as the baseline ([BASELINE SET]).

    Exits 1 if any test fails.
    """
    print(f"\n=== test-{mode} ({FUNCTIONAL_EVENTS} events) ===")
    baselines = load_baselines()
    failures = 0

    for (test_name, binary_name, macro_file,
         example_path, support_files, out_file) in FUNCTIONAL_CASES[mode]:
        binary = find_binary(binary_name)
        workdir = setup_workdir(test_name, example_path, support_files)
        write_base_macro(macro_file, example_path,
                         f"/Output/Filename {out_file}", workdir)
        wrapper = os.path.join(workdir, "run.mac")
        write_run_macro(macro_file, FUNCTIONAL_EVENTS, wrapper)
        stdout, stderr, returncode = run_sim(binary, wrapper, workdir)

        ok, msg = _check_run_criteria(test_name, stdout, stderr, returncode)
        if not ok:
            print(msg)
            failures += 1
            continue

        output_path = os.path.join(workdir, out_file)
        if not os.path.isfile(output_path):
            print(f"[FAIL] {test_name:<30} output file not created: {output_path}")
            failures += 1
            continue

        observed = count_lines(output_path)
        passed, msg = check_baseline(test_name, observed, baselines)
        print(msg)
        if not passed:
            failures += 1

    save_baselines(baselines)

    if failures:
        print(f"\n{failures} FAILED")
        sys.exit(1)
    else:
        print(f"\nAll {mode} tests passed.\n")


# ---------------------------------------------------------------------------
# Benchmark tests
# ---------------------------------------------------------------------------

# Each entry: (variant_label, binary_name, base_macro_filename, example_path,
#              support_files)
BENCHMARK_CASES = [
    (
        "UCCeBrA_cs137",
        "UCCeBrA",
        "bench_cs137.mac",
        "examples/cs137/cs137_simple.mac",
        [],
    ),
    (
        "UCCeBrA_co60",
        "UCCeBrA",
        "bench_co60.mac",
        "examples/co60/co60.mac",
        ["demonstrator.geo", "bricks.geo"],
    ),
]


def run_benchmark(n_events):
    """Run all scenarios for n_events and log events/sec to benchmark.log.

    No pass/fail — this is for performance tracking over time.
    Results are appended to benchmark.log (TSV, gitignored).
    """
    git_hash, git_branch = get_git_info()
    cpu = get_cpu_info()
    today = datetime.date.today().isoformat()

    print(f"\n=== Benchmark ({n_events} events, commit {git_hash}, branch {git_branch}) ===")
    print(f"CPU: {cpu}")
    print(f"{'Variant':<20} {'Events/sec':>12}")
    print("-" * 34)

    rows = []

    for variant, binary_name, macro_file, example_path, support_files in BENCHMARK_CASES:
        binary = find_binary(binary_name)
        workdir = setup_workdir(f"bench_{variant}", example_path, support_files)
        write_base_macro(macro_file, example_path,
                         "/Output/Filename output.out", workdir)
        wrapper = os.path.join(workdir, "run.mac")
        write_run_macro(macro_file, n_events, wrapper)
        stdout, stderr, returncode = run_sim(binary, wrapper, workdir)

        eps = parse_events_per_sec(stdout)
        if eps is None or returncode != 0:
            print(f"  {variant:<20} {'ERROR':>12}")
            eps = 0
        else:
            print(f"  {variant:<20} {eps:>12.0f}")

        rows.append((today, git_hash, git_branch, cpu, variant, n_events, f"{eps:.0f}"))

    append_benchmark_log(rows)
    print(f"\nResults appended to benchmark.log")


# ---------------------------------------------------------------------------
# Baseline update
# ---------------------------------------------------------------------------

def update_baselines():
    """Re-run all functional tests and reset baselines to observed values.

    Wipes baselines.json first so every test triggers [BASELINE SET].
    If any test fails mid-update, the original baselines are restored.
    """
    print("Resetting all baselines...")
    old_content = None
    if os.path.isfile(BASELINES_FILE):
        with open(BASELINES_FILE) as f:
            old_content = f.read()
    if os.path.isfile(BASELINES_FILE):
        os.remove(BASELINES_FILE)
    try:
        for mode in ["sources"]:
            run_functional(mode)
    except SystemExit:
        if old_content is not None:
            with open(BASELINES_FILE, "w") as f:
                f.write(old_content)
            print("Baseline update failed — original baselines restored.")
        raise
    print("Baselines updated.")


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="UCCeBrA test and benchmark driver"
    )
    parser.add_argument(
        "--mode", required=False, default=None,
        choices=["smoke", "sources", "benchmark"],
        help="Test mode to run"
    )
    parser.add_argument(
        "--events", type=int, default=10000,
        help="Event count for benchmark mode (default: 10000)"
    )
    parser.add_argument(
        "--update-baselines", action="store_true",
        help="Reset all baselines to current observed values"
    )
    args = parser.parse_args()

    if args.update_baselines:
        update_baselines()
        return

    if args.mode is None:
        parser.error("--mode is required unless --update-baselines is specified")

    os.makedirs(TMP_DIR, exist_ok=True)

    if args.mode == "smoke":
        run_smoke()
    elif args.mode == "sources":
        run_functional("sources")
    elif args.mode == "benchmark":
        run_benchmark(args.events)


if __name__ == "__main__":
    main()
```

- [ ] **Step 2: Verify syntax**

```bash
python3 -c "import ast; ast.parse(open('tests/benchmark.py').read()); print('OK')"
```

Expected: `OK`

- [ ] **Step 3: Commit**

```bash
cd /home/nuc/Hanna26-UCCeBr3
git add tests/benchmark.py
git commit -m "add sources and benchmark modes to benchmark.py"
```

---

## Task 4: Update `GNUmakefile` test targets

**Files:**
- Modify: `GNUmakefile`

**Interfaces:**
- Consumes: `tests/benchmark.py` from Tasks 1–3

- [ ] **Step 1: Replace the four test targets in `GNUmakefile`**

The current targets (lines 27–42) reference the old split files. Replace them with:

```makefile
# Run the full test suite: smoke check then line-count regression.
# Requires the binary to be built first: make && make test
test:
	python3 tests/benchmark.py --mode smoke && python3 tests/benchmark.py --mode sources

# Run smoke tests only — quick check that the binary runs and produces output.
test-smoke:
	python3 tests/benchmark.py --mode smoke

# Run functional tests only — line-count regression against baselines.json.
test-functional:
	python3 tests/benchmark.py --mode sources

# Run benchmark tests only — event rate logging to benchmark.log.
test-benchmark:
	python3 tests/benchmark.py --mode benchmark
```

The old targets in the file are:
```
# Run the full test suite (smoke + functional + benchmark).
# Requires the binary to be built first: make && make test
test:
	python3 tests/run_tests.py

# Run smoke tests only — quick check that the binary runs and produces output.
test-smoke:
	python3 tests/test_smoke.py

# Run functional tests only — output format, event counts, physics sanity.
test-functional:
	python3 tests/test_functional.py

# Run benchmark tests only — event rate logging and event count verification.
test-benchmark:
	python3 tests/test_benchmark.py
```

Note: GNUmakefile uses **tab** indentation for recipe lines — ensure the recipe lines use a real tab character, not spaces.

- [ ] **Step 2: Verify the makefile parses**

```bash
cd /home/nuc/Hanna26-UCCeBr3
make -n test-smoke
```

Expected output: `python3 tests/benchmark.py --mode smoke` (dry run, no error)

- [ ] **Step 3: Commit**

```bash
cd /home/nuc/Hanna26-UCCeBr3
git add GNUmakefile
git commit -m "update GNUmakefile test targets to use benchmark.py"
```

---

## Task 5: Update `AGENTS.md` testing section

**Files:**
- Modify: `/home/nuc/workspace/AGENTS.md`

**Interfaces:**
- No code interfaces — documentation only.

- [ ] **Step 1: Replace the Testing subsection in AGENTS.md**

The current Testing subsection (lines 234–256 of `/home/nuc/workspace/AGENTS.md`) reads:

```
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
```

Replace it with:

```
### Testing

A Python test suite lives in `tests/benchmark.py`, modelled on the UCGretina test
suite. Run it from the repository root via make targets:

```bash
make test            # full suite: smoke + functional
make test-smoke      # quick sanity check (100 events; binary runs, no fatal, rate > 0)
make test-functional # line-count regression against baselines (1000 events)
make test-benchmark  # events/sec timing, no pass/fail (10000 events)
```

The suite requires the binary to be built (`make`) before running.

Baseline line counts are stored in `tests/baselines.json` (git-tracked,
hardware-independent). On the first run after a fresh clone, baselines are set
automatically. To reset baselines after an intentional physics or geometry change:

```bash
python3 tests/benchmark.py --update-baselines
```

Benchmark event rates are logged to `benchmark.log` (gitignored,
hardware-dependent).

"It compiles and the example runs" is a necessary condition, not a sufficient one.
Physics correctness still requires human review.
```

Also update the critical safety note in §1 (line 22): remove "There is currently no
automated test suite to catch this." — a test suite now exists. The updated sentence
should read:

```
An automated test suite exists (`make test`) but it cannot verify physical
correctness — it only detects changes in detection rates and build breakage.
If you are unsure whether a change is physically safe, stop and ask a human
with domain knowledge before proceeding.
```

- [ ] **Step 2: Verify the section looks correct**

```bash
grep -A 25 "### Testing" /home/nuc/workspace/AGENTS.md
```

Confirm the output shows the new text with `make test`, `make test-smoke`, etc.

- [ ] **Step 3: Commit**

```bash
cd /home/nuc/Hanna26-UCCeBr3
git add /home/nuc/workspace/AGENTS.md
git commit -m "update AGENTS.md testing section for new benchmark.py suite"
```

---

## Task 6: Smoke-test the suite end-to-end (if binary is available)

**Files:**
- No new files — verification only.

This task verifies the complete suite works against a built binary. If the binary
is not available in the current environment, skip to Task 6b.

- [ ] **Step 1: Check whether the binary exists**

```bash
ls $G4WORKDIR/bin/$G4SYSTEM/UCCeBrA 2>/dev/null && echo "BINARY FOUND" || echo "BINARY NOT FOUND"
```

If `BINARY NOT FOUND`, skip to Step 5 (syntax-only check).

- [ ] **Step 2: Run smoke tests**

```bash
cd /home/nuc/Hanna26-UCCeBr3
python3 tests/benchmark.py --mode smoke
```

Expected: both `smoke_cs137` and `smoke_co60` print `[PASS]` and exit 0.

- [ ] **Step 3: Run functional tests (sets baselines on first run)**

```bash
python3 tests/benchmark.py --mode sources
```

Expected: both tests print `[BASELINE SET]` (first run) or `[PASS]`, exit 0.
`tests/baselines.json` is created.

- [ ] **Step 4: Run functional tests a second time (verifies baseline comparison)**

```bash
python3 tests/benchmark.py --mode sources
```

Expected: both tests print `[PASS]` with `baseline=` and `tolerance=±` values.

- [ ] **Step 5 (binary not available): Verify script runs and fails gracefully**

```bash
python3 tests/benchmark.py --mode smoke 2>&1 | head -5
```

Expected: error message `ERROR: G4WORKDIR and G4SYSTEM must be set.` or
`ERROR: Binary not found:` — not a Python traceback.

- [ ] **Step 6: Commit `baselines.json` if created**

```bash
cd /home/nuc/Hanna26-UCCeBr3
git add tests/baselines.json 2>/dev/null; git status
```

If `baselines.json` appears as a new file, commit it:

```bash
git commit -m "add initial test baselines"
```

Otherwise commit only if other files were modified:

```bash
git status  # confirm clean or commit remaining changes
```
