#!/usr/bin/env python3
"""UCCeBrA testing suite: functional tests and performance benchmarks.

Modelled on the UCGretina test suite. Run via make targets:
  make test            # smoke + sources
  make test-smoke      # quick sanity check (1000 events)
  make test-functional # detection ratio comparison (100000 events)
  make test-benchmark  # events/sec timing, detection ratio (1000000 events)
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
FUNCTIONAL_LOG = os.path.join(PROJECT_ROOT, "functional_tests.log")


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

    The end-of-run line uses C++ stream output which may produce scientific
    notation (e.g. "2e+05 events/s") when the rate is large, so the regex
    must match both plain decimals and scientific notation.

    Returns float, or None if the pattern is not found.
    """
    matches = re.findall(r"([\d.]+(?:[eE][+-]?\d+)?)\s+events/s", stdout)
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


def append_functional_log(rows):
    """Append functional test results to functional_tests.log (TSV).

    Writes a header line the first time the file is created.
    Each row is a tuple of values that will be joined with tabs.
    Columns: date, git_hash, git_branch, cpu, test_name, events,
             ratio, sigma, baseline_ratio, baseline_sigma, n_sigma, verdict.
    """
    header = (
        "date\tgit_hash\tgit_branch\tcpu\ttest_name\tevents\t"
        "ratio\tsigma\tbaseline_ratio\tbaseline_sigma\tn_sigma\tverdict\n"
    )
    write_header = not os.path.isfile(FUNCTIONAL_LOG)
    with open(FUNCTIONAL_LOG, "a") as f:
        if write_header:
            f.write(header)
        for row in rows:
            f.write("\t".join(str(v) for v in row) + "\n")


def count_detected_and_simulated(filepath):
    """Count detected and simulated events in a UCCeBrA output file.

    Each line beginning with 'E' corresponds to one simulated event.
    Each line beginning with 'D' corresponds to one detected event
    (an event in which at least one detector registered energy).

    Args:
        filepath: Absolute path to the simulation output file.

    Returns:
        Tuple of (n_detected, n_simulated) as integers.
        Returns (0, 0) if the file cannot be read.
    """
    n_detected = 0
    n_simulated = 0
    try:
        with open(filepath, "r") as f:
            for line in f:
                if line.startswith("D"):
                    n_detected += 1
                elif line.startswith("E"):
                    n_simulated += 1
    except OSError:
        return 0, 0
    return n_detected, n_simulated


def compute_ratio(n_detected, n_simulated):
    """Compute the detection ratio and its Poisson uncertainty.

    ratio = n_detected / n_simulated
    sigma = sqrt(n_detected) / n_simulated

    The uncertainty follows from treating n_detected as a Poisson count:
    the standard deviation of a Poisson count N is sqrt(N), so the
    fractional uncertainty on the ratio is sqrt(N_detected) / N_simulated.

    Args:
        n_detected: Number of detected events (D lines in output).
        n_simulated: Number of simulated events (E lines in output).

    Returns:
        Tuple of (ratio, sigma) as floats. Returns (0.0, 0.0) if
        n_simulated is zero.
    """
    if n_simulated == 0:
        return 0.0, 0.0
    ratio = n_detected / n_simulated
    sigma = math.sqrt(n_detected) / n_simulated
    return ratio, sigma


# Maps functional test names to the corresponding benchmark variant label
# used as the key in baselines.json.
BASELINE_KEY = {
    "sources_cs137": "UCCeBrA_cs137",
    "sources_co60":  "UCCeBrA_co60",
}


# ---------------------------------------------------------------------------
# Smoke tests
# ---------------------------------------------------------------------------

SMOKE_EVENTS = 1000

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


# ---------------------------------------------------------------------------
# Functional (sources) tests
# ---------------------------------------------------------------------------

FUNCTIONAL_EVENTS = 100000

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
    """Run functional tests and compare detection ratios against benchmark baselines.

    For each scenario:
      1. Runs FUNCTIONAL_EVENTS events.
      2. Parses the output file to compute the detection ratio and its
         Poisson uncertainty (sqrt(detected) / simulated).
      3. Looks up the benchmark baseline ratio from baselines.json.
         If no baseline exists, prints [NO BASELINE] and skips comparison.
      4. Computes the number of standard deviations between the test ratio
         and the benchmark baseline:
           n_sigma = |ratio_test - ratio_baseline|
                     / sqrt(sigma_test^2 + sigma_baseline^2)
      5. Verdict:
           n_sigma < 2           -> [PASS]
           2 <= n_sigma < 3      -> [MARGINAL PASS]
           n_sigma >= 3          -> [FAIL]

    Exits 1 if any test fails (3 sigma or more) or if the simulation
    crashes. Marginal passes do not cause a non-zero exit.
    """
    print(f"\n=== test-{mode} ({FUNCTIONAL_EVENTS} events) ===")
    baselines = load_baselines()
    failures = 0
    log_rows = []
    git_hash, git_branch = get_git_info()
    cpu = get_cpu_info()
    today = datetime.date.today().isoformat()

    for (test_name, binary_name, macro_file,
         example_path, support_files, out_file) in FUNCTIONAL_CASES[mode]:
        binary = find_binary(binary_name)
        workdir = setup_workdir(test_name, example_path, support_files)
        write_base_macro(macro_file, example_path,
                         f"/Output/Filename {out_file}", workdir)
        wrapper = os.path.join(workdir, "run.mac")
        write_run_macro(macro_file, FUNCTIONAL_EVENTS, wrapper)
        stdout, stderr, returncode = run_sim(binary, wrapper, workdir)

        # Check that the simulation ran cleanly.
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

        # Compute detection ratio for this test run.
        n_detected, n_simulated = count_detected_and_simulated(output_path)
        ratio, sigma = compute_ratio(n_detected, n_simulated)

        # Look up benchmark baseline using the variant label mapping.
        baseline_key = BASELINE_KEY.get(test_name)
        baseline_entry = baselines.get(baseline_key) if baseline_key else None

        if baseline_entry is None or not isinstance(baseline_entry, dict):
            # No benchmark baseline established yet — cannot compare.
            print(f"[NO BASELINE] {test_name:<30} "
                  f"ratio={ratio:.4f}\xb1{sigma:.4f}  "
                  f"(run make test-benchmark to set baseline)")
            log_rows.append((
                today, git_hash, git_branch, cpu, test_name, FUNCTIONAL_EVENTS,
                f"{ratio:.6f}", f"{sigma:.6f}",
                "N/A", "N/A", "N/A", "NO BASELINE",
            ))
            continue

        ratio_base = baseline_entry["ratio"]
        sigma_base = baseline_entry["sigma"]

        # Combined uncertainty from both the test run and the baseline.
        combined_sigma = math.sqrt(sigma**2 + sigma_base**2)
        if combined_sigma == 0:
            n_sigma = 0.0
        else:
            n_sigma = abs(ratio - ratio_base) / combined_sigma

        # Format readout line.
        base_str = f"baseline={ratio_base:.4f}\xb1{sigma_base:.4f}"
        ratio_str = f"ratio={ratio:.4f}\xb1{sigma:.4f}"

        if n_sigma < 2:
            verdict = "[PASS]"
        elif n_sigma < 3:
            verdict = "[MARGINAL PASS]"
        else:
            verdict = "[FAIL]"
            failures += 1

        print(f"{verdict:<16} {test_name:<30} {ratio_str}  {base_str}  {n_sigma:.2f}\u03c3")

        log_rows.append((
            today, git_hash, git_branch, cpu, test_name, FUNCTIONAL_EVENTS,
            f"{ratio:.6f}", f"{sigma:.6f}",
            f"{ratio_base:.6f}", f"{sigma_base:.6f}",
            f"{n_sigma:.4f}", verdict.strip("[]"),
        ))

    append_functional_log(log_rows)

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
    """Run all scenarios and record events/sec, detection ratio, and sigma.

    Parses the output file after each run to compute the detection ratio
    (detected events / simulated events) and its Poisson uncertainty
    (sqrt(detected) / simulated). Stores ratio and sigma in baselines.json
    so the functional tests can compare against them.

    No pass/fail — this is for establishing baselines and tracking
    performance over time. Stores ratio and sigma in baselines.json.
    """
    git_hash, git_branch = get_git_info()
    cpu = get_cpu_info()

    print(f"\n=== Benchmark ({n_events} events, commit {git_hash}, branch {git_branch}) ===")
    print(f"CPU: {cpu}")
    print(f"{'Variant':<20} {'Events/sec':>12}  {'Ratio':>8}  {'Sigma':>8}")
    print("-" * 56)

    baselines = load_baselines()

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
            continue

        output_path = os.path.join(workdir, "output.out")
        n_detected, n_simulated = count_detected_and_simulated(output_path)
        ratio, sigma = compute_ratio(n_detected, n_simulated)

        print(f"  {variant:<20} {eps:>12.0f}  {ratio:>8.4f}  {sigma:>8.4f}")

        # Store ratio baseline for functional tests to compare against.
        baselines[variant] = {"ratio": ratio, "sigma": sigma}

    save_baselines(baselines)
    print(f"\nBaselines updated in tests/baselines.json")


# ---------------------------------------------------------------------------
# Baseline update
# ---------------------------------------------------------------------------

def update_baselines():
    """Re-run the benchmark and reset ratio baselines to observed values.

    Wipes ratio entries from baselines.json first, then runs the benchmark
    to establish fresh ratio+sigma baselines. If the benchmark fails
    mid-run, the original baselines are restored.

    Note: this runs 1,000,000 events per scenario (the benchmark default).
    Use --events to override if a faster update is needed.
    """
    print("Resetting all baselines (running benchmark)...")
    old_content = None
    if os.path.isfile(BASELINES_FILE):
        with open(BASELINES_FILE) as f:
            old_content = f.read()
    # Wipe existing ratio entries so benchmark sets them fresh.
    baselines = load_baselines()
    cleaned = {k: v for k, v in baselines.items() if not isinstance(v, dict)}
    save_baselines(cleaned)
    try:
        run_benchmark(1000000)
    except Exception:
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
        "--events", type=int, default=1000000,
        help="Event count for benchmark mode (default: 1000000)"
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
