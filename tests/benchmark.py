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
