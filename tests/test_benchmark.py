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
#   tests/benchmark.log               - human-readable rate log (appended, gitignored)
#                                       Hardware-dependent; records event rate and
#                                       processor info for context.
#   tests/benchmarks/event-counts.json - event count ground truth (tracked by git)
#                                       Hardware-independent; records how many events
#                                       appear in the output file for a given run size.
#
# If event-counts.json does not exist, or a scenario is not yet recorded,
# the test creates/updates the file with the current run's results and passes.
# On subsequent runs, the test asserts the event count matches the stored value.

import os
import sys
import json
import unittest
import tempfile
from datetime import datetime

# Ensure the repository root is on sys.path so this file can be run directly
# with 'python3 tests/test_benchmark.py' as well as via the module form.
_REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if _REPO_ROOT not in sys.path:
    sys.path.insert(0, _REPO_ROOT)

from tests.output_parser import (
    find_binary, patch_mac, run_simulation, parse_output,
    get_git_hash, get_processor_info
)

REPO_ROOT        = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CS137_MAC        = os.path.join(REPO_ROOT, "examples", "cs137", "cs137_simple.mac")
CO60_MAC         = os.path.join(REPO_ROOT, "examples", "co60", "co60.mac")
# co60.mac references geometry files by relative path; must run from this directory
CO60_CWD         = os.path.join(REPO_ROOT, "examples", "co60")
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


def _append_benchmark_log(scenario, n_events, elapsed, proc_info, git_hash):
    """
    Append one timestamped line to benchmark.log.
    This file is gitignored because event rates are hardware-dependent.

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
    Run a benchmark scenario and measure elapsed wall-clock time.
    Returns (n_events_in_output, elapsed_seconds).
    Raises RuntimeError if the binary exits non-zero.
    Cleans up temporary files before returning.
    """
    binary = find_binary()
    tmpdir = tempfile.mkdtemp(prefix=f"ucce_bench_{scenario}_")
    outfile = os.path.join(tmpdir, f"bench_{scenario}.out")
    macfile = patch_mac(mac_path, outfile, BENCHMARK_EVENTS)

    rc, stdout, stderr, elapsed = run_simulation(binary, macfile, cwd=cwd)

    # Remove temp macro immediately after run
    if os.path.exists(macfile):
        os.remove(macfile)

    if rc != 0:
        raise RuntimeError(
            f"Benchmark simulation failed for {scenario} (rc={rc}).\n"
            f"stderr: {stderr}"
        )

    # Count events in output; each unique event_id counts as one event
    events = parse_output(outfile)
    n_events = len(events)

    # Remove output file and temp directory
    if os.path.exists(outfile):
        os.remove(outfile)
    os.rmdir(tmpdir)

    return n_events, elapsed


class BenchmarkTests(unittest.TestCase):
    """
    Benchmark tests for UCCeBrA.
    Measures event rate (logged to benchmark.log, gitignored) and verifies
    event counts (stored in event-counts.json, git-tracked).
    """

    def _run_and_record(self, scenario, mac_path, cwd):
        """
        Shared logic for a benchmark test:
        1. Run the simulation and measure elapsed time.
        2. Collect processor info and git hash for context.
        3. Append results to benchmark.log (hardware-dependent rate log).
        4. Assert or initialise event-counts.json (hardware-independent counts).
        """
        n_events, elapsed = _run_benchmark(mac_path, cwd, scenario)

        proc_info = get_processor_info()
        git_hash  = get_git_hash()

        # Always append to rate log — hardware-dependent, not tracked by git
        _append_benchmark_log(scenario, n_events, elapsed, proc_info, git_hash)

        # Load existing event-counts.json, or start fresh if it does not exist
        counts = _load_event_counts()
        if counts is None:
            counts = {}

        if scenario not in counts:
            # First run for this scenario: store current count and processor info,
            # then pass — the baseline has just been established.
            counts[scenario] = {
                "expected_events": n_events,
                "cpu":   proc_info["cpu"],
                "cores": proc_info["cores"],
                "mhz":   proc_info["mhz"],
                "os":    proc_info["os"],
            }
            _save_event_counts(counts)
            # Test passes on first run
            return

        # Subsequent runs: assert the event count matches the stored baseline.
        # A mismatch suggests a change to the output format or simulation logic.
        expected = counts[scenario]["expected_events"]
        self.assertEqual(
            n_events, expected,
            f"{scenario}: expected {expected} events in output, got {n_events}. "
            f"This may indicate a change in the output format or simulation logic "
            f"(e.g. EventAction.cc was modified). If the change is intentional, "
            f"delete the scenario entry from tests/benchmarks/event-counts.json "
            f"and re-run to establish a new baseline."
        )

    def test_cs137_benchmark(self):
        """Benchmark cs137_simple: single detector, 662 keV direct gamma gun."""
        self._run_and_record(
            scenario="cs137_simple",
            mac_path=CS137_MAC,
            # cs137_simple.mac has no relative file references; run from tests/
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
