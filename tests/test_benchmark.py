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
#
#   tests/benchmark.log
#     Human-readable rate log (appended, gitignored). Hardware-dependent.
#     A header line with git hash and hardware info is written once per test
#     session at the top of each run block, followed by compact per-scenario
#     result lines.
#
#   tests/benchmarks/event-counts.json
#     Event count ground truth (tracked by git). Hardware-independent.
#     Stores emitted and detected event counts per scenario, plus a meta block
#     with git hash and hardware info from the run that established the baseline.
#
# If event-counts.json does not exist, or a scenario is not yet recorded,
# the test creates/updates the file with the current run's results and passes.
# On subsequent runs the test performs two checks:
#   1. Emitted event count matches the stored baseline exactly.
#   2. |current_detected - baseline_detected| <= 2 * sqrt(baseline_detected)
#      This is a Poisson consistency check on the detection rate.

import os
import sys
import json
import math
import unittest
from datetime import datetime

# Ensure the repository root is on sys.path so this file can be run directly
# with 'python3 tests/test_benchmark.py' as well as via the module form.
_REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if _REPO_ROOT not in sys.path:
    sys.path.insert(0, _REPO_ROOT)

from tests.output_parser import (
    find_binary, patch_mac, run_simulation, parse_output,
    get_git_hash, get_processor_info,
    prepare_test_dir, copy_co60_geometry
)

REPO_ROOT        = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CS137_MAC        = os.path.join(REPO_ROOT, "examples", "cs137", "cs137_simple.mac")
CO60_MAC         = os.path.join(REPO_ROOT, "examples", "co60", "co60.mac")
BENCHMARK_LOG    = os.path.join(REPO_ROOT, "tests", "benchmark.log")
COUNTS_JSON      = os.path.join(REPO_ROOT, "tests", "benchmarks", "event-counts.json")
BENCHMARK_EVENTS = 10000


def _load_event_counts():
    """
    Load event-counts.json. Returns the parsed dict, or None if the file
    does not exist. The file stores hardware-independent event count baselines.
    """
    if not os.path.isfile(COUNTS_JSON):
        return None
    with open(COUNTS_JSON, "r") as f:
        return json.load(f)


def _save_event_counts(counts):
    """
    Write the event counts dict to event-counts.json.
    Creates the benchmarks/ directory if needed.
    """
    os.makedirs(os.path.dirname(COUNTS_JSON), exist_ok=True)
    with open(COUNTS_JSON, "w") as f:
        json.dump(counts, f, indent=2)


def _write_benchmark_header(proc_info, git_hash):
    """
    Append a session header block to benchmark.log.
    Called once per test run (in setUpClass) so that git hash and hardware
    info appear only once per session rather than on every result line.

    Format:
      === YYYY-MM-DD HH:MM:SS | bin: <hash> | CPU: <model> | cores: <n> | MHz: <mhz> | OS: <os> ===
    """
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    header = (
        f"=== {timestamp} | bin: {git_hash} | "
        f"CPU: {proc_info['cpu']} | cores: {proc_info['cores']} | "
        f"MHz: {proc_info['mhz']} | OS: {proc_info['os']} ===\n"
    )
    with open(BENCHMARK_LOG, "a") as f:
        f.write(header)


def _append_benchmark_log(scenario, n_emitted, n_detected, elapsed):
    """
    Append one compact result line to benchmark.log.
    Git hash and hardware info are not repeated here — they appear in the
    session header written by _write_benchmark_header at the start of the run.

    Format:
      YYYY-MM-DD HH:MM:SS | <scenario> | <n_emitted> emitted | <rate> events/s | <n_detected> detected
    """
    rate = int(n_emitted / elapsed) if elapsed > 0 else 0
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    line = (
        f"{timestamp} | {scenario} | {n_emitted} emitted | "
        f"{rate} events/s | {n_detected} detected\n"
    )
    with open(BENCHMARK_LOG, "a") as f:
        f.write(line)


def _run_benchmark(mac_path, scenario, is_co60=False):
    """
    Run a benchmark scenario and measure elapsed wall-clock time.
    Returns (n_emitted, n_detected, elapsed_seconds) where:
      n_emitted  = total number of events in the output (one per E-line)
      n_detected = number of events with at least one detector hit (D-line present)
    Raises RuntimeError if the binary exits non-zero.

    Uses a named subdirectory under tests/tmp/ so the user can inspect the
    macro file, geometry files, and output file after the run.
    """
    binary  = find_binary()
    testdir = prepare_test_dir(f"benchmark_{scenario}")
    outfile = os.path.join(testdir, f"bench_{scenario}.out")

    if is_co60:
        # co60.mac references demonstrator.geo and bricks.geo by relative path;
        # copy them into the test directory so the binary finds them
        copy_co60_geometry(testdir)

    macfile = patch_mac(mac_path, outfile, BENCHMARK_EVENTS, dir=testdir)
    rc, stdout, stderr, elapsed = run_simulation(binary, macfile, cwd=testdir)

    if rc != 0:
        raise RuntimeError(
            f"Benchmark simulation failed for {scenario} (rc={rc}).\n"
            f"stderr: {stderr}"
        )

    # Parse output: count total events and those with at least one detector hit
    events     = parse_output(outfile)
    n_emitted  = len(events)
    n_detected = sum(1 for ev in events if ev["n_dets_hit"] > 0)

    return n_emitted, n_detected, elapsed


class BenchmarkTests(unittest.TestCase):
    """
    Benchmark tests for UCCeBrA.
    Measures event rate and detection statistics, logging results to
    benchmark.log (gitignored) and event-counts.json (git-tracked).
    """

    @classmethod
    def setUpClass(cls):
        """
        Write a session header to benchmark.log once before any scenario runs.
        This records git hash and hardware info a single time per test session
        rather than repeating them on every result line.
        """
        cls.proc_info = get_processor_info()
        cls.git_hash  = get_git_hash()
        _write_benchmark_header(cls.proc_info, cls.git_hash)

    def _run_and_record(self, scenario, mac_path, is_co60=False):
        """
        Shared logic for a benchmark test:
        1. Run the simulation and measure elapsed time.
        2. Append a compact result line to benchmark.log.
        3. Assert or initialise event-counts.json.

        On first run for a scenario:
          - Stores emitted_events, detected_events, git hash, and hardware info.
          - Test passes (baseline established).

        On subsequent runs:
          - Asserts emitted count matches baseline exactly.
          - Asserts |current_detected - baseline_detected| <= 2 * sqrt(baseline_detected).
            This is a Poisson consistency check: if the detection rate is stable,
            fluctuations should be within ~2 standard deviations.
        """
        n_emitted, n_detected, elapsed = _run_benchmark(mac_path, scenario, is_co60=is_co60)

        # Append compact result line; header was already written in setUpClass
        _append_benchmark_log(scenario, n_emitted, n_detected, elapsed)

        # Load existing event-counts.json, or start fresh if it does not exist
        counts = _load_event_counts()
        if counts is None:
            counts = {}

        if scenario not in counts:
            # First run for this scenario: record baseline and pass
            counts[scenario] = {
                "emitted_events":  n_emitted,
                "detected_events": n_detected,
            }
            # Update meta block with current git hash and hardware info
            counts["meta"] = {
                "git_hash": self.git_hash,
                "cpu":      self.proc_info["cpu"],
                "cores":    self.proc_info["cores"],
                "mhz":      self.proc_info["mhz"],
                "os":       self.proc_info["os"],
            }
            _save_event_counts(counts)
            # Test passes on first run — baseline has been established
            return

        # --- Check 1: emitted event count must match baseline exactly ---
        expected_emitted = counts[scenario]["emitted_events"]
        self.assertEqual(
            n_emitted, expected_emitted,
            f"{scenario}: expected {expected_emitted} emitted events, got {n_emitted}. "
            f"This may indicate a change in the output format or simulation logic "
            f"(e.g. EventAction.cc was modified). If intentional, delete the scenario "
            f"entry from tests/benchmarks/event-counts.json and re-run to reset baseline."
        )

        # --- Check 2: detected event count must be within 2*sqrt(baseline) ---
        baseline_detected = counts[scenario]["detected_events"]
        tolerance = 2.0 * math.sqrt(baseline_detected)
        deviation = abs(n_detected - baseline_detected)
        self.assertLessEqual(
            deviation, tolerance,
            f"{scenario}: detected events {n_detected} deviates from baseline "
            f"{baseline_detected} by {deviation:.1f}, which exceeds 2*sqrt({baseline_detected}) "
            f"= {tolerance:.1f}. This suggests a change in detection efficiency. "
            f"If intentional, reset the baseline by deleting the scenario entry from "
            f"tests/benchmarks/event-counts.json and re-running."
        )

    def test_cs137_benchmark(self):
        """Benchmark cs137_simple: single detector, 662 keV direct gamma gun."""
        self._run_and_record(
            scenario="cs137_simple",
            mac_path=CS137_MAC,
        )

    def test_co60_benchmark(self):
        """Benchmark co60: full 9-detector array, Co-60 radioactive decay."""
        self._run_and_record(
            scenario="co60",
            mac_path=CO60_MAC,
            is_co60=True,
        )


if __name__ == "__main__":
    unittest.main()
