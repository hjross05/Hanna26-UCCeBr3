# Detection Ratio Tests Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the line-count regression in the test suite with a physics-meaningful detection ratio (detected/simulated) with Poisson uncertainty, stored by the benchmark run and compared against in the functional tests.

**Architecture:** Two new infrastructure functions (`count_detected_and_simulated`, `compute_ratio`) are added to `benchmark.py`. The benchmark mode parses its output file and stores ratio+sigma in `baselines.json`. The functional/sources mode parses its output file, computes its own ratio+sigma, and compares against the benchmark baseline using combined uncertainty. The `check_baseline` and `count_lines` functions are removed.

**Tech Stack:** Python 3 standard library only. All changes confined to `tests/benchmark.py` and `tests/baselines.json`.

## Global Constraints

- Python 3 standard library only — no third-party packages.
- `baselines.json` stores ratio baseline entries as `{"ratio": float, "sigma": float}` dicts keyed by the benchmark variant labels (`UCCeBrA_cs137`, `UCCeBrA_co60`). Functional tests look up baselines using those same keys via `BASELINE_KEY`.
- Detection ratio: `ratio = n_detected / n_simulated`, `sigma = sqrt(n_detected) / n_simulated`.
- `n_detected` = number of lines beginning with `D` in `output.out`.
- `n_simulated` = number of lines beginning with `E` in `output.out`.
- Pass/fail thresholds: `n_sigma < 2` → PASS, `2 <= n_sigma < 3` → MARGINAL PASS, `n_sigma >= 3` → FAIL.
- Combined uncertainty: `n_sigma = |ratio_test - ratio_baseline| / sqrt(sigma_test^2 + sigma_baseline^2)`.
- If no benchmark baseline exists for a scenario, print `[NO BASELINE]` and do not fail.
- Default benchmark event count: 1,000,000 (was 10,000). `--events` flag still overrides.
- `update_baselines` re-runs the benchmark (not the functional tests) to reset baselines.
- Smoke tests are unchanged.
- `check_baseline` and `count_lines` functions are removed (no longer used).
- `benchmark.log` gains two new columns: `ratio` and `sigma` (after `events_per_sec`).

---

## Baseline key mapping

The benchmark runs under variant labels (`UCCeBrA_cs137`, `UCCeBrA_co60`). The functional tests run under test names (`sources_cs137`, `sources_co60`). The baseline lookup maps functional test names to the corresponding benchmark variant label:

```python
BASELINE_KEY = {
    "sources_cs137": "UCCeBrA_cs137",
    "sources_co60":  "UCCeBrA_co60",
}
```

This dict is defined as a module-level constant.

---

## Task 1: Add `count_detected_and_simulated` and `compute_ratio` infrastructure functions; remove `check_baseline` and `count_lines`

**Files:**
- Modify: `tests/benchmark.py`

**Interfaces:**
- Removes: `count_lines(filepath)` (lines 162–167), `check_baseline(name, observed, baselines)` (lines 205–228)
- Produces:
  - `count_detected_and_simulated(filepath: str) -> tuple[int, int]` — returns `(n_detected, n_simulated)`
  - `compute_ratio(n_detected: int, n_simulated: int) -> tuple[float, float]` — returns `(ratio, sigma)`
  - `BASELINE_KEY: dict[str, str]` — module-level constant mapping functional test names to benchmark variant labels

- [ ] **Step 1: Remove `count_lines` and `check_baseline` from `tests/benchmark.py`**

Delete the `count_lines` function (lines 162–167):
```python
def count_lines(filepath):
    """Return the number of lines in a file using wc -l."""
    result = subprocess.run(["wc", "-l", filepath], capture_output=True, text=True)
    if result.returncode != 0:
        return 0
    return int(result.stdout.strip().split()[0])
```

Delete the `check_baseline` function (lines 205–228):
```python
def check_baseline(name, observed, baselines):
    """Compare an observed line count against a stored baseline.
    ...
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
```

- [ ] **Step 2: Add `count_detected_and_simulated`, `compute_ratio`, and `BASELINE_KEY` in their place**

Insert the following after `append_benchmark_log` and before the smoke test section:

```python
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
```

- [ ] **Step 3: Verify syntax**

```bash
python3 -c "import ast; ast.parse(open('tests/benchmark.py').read()); print('OK')"
```

Expected: `OK`

- [ ] **Step 4: Verify the two removed functions are gone and the two new ones are present**

```bash
python3 -c "
import sys; sys.path.insert(0, '.')
import tests.benchmark as b
assert not hasattr(b, 'count_lines'), 'count_lines should be removed'
assert not hasattr(b, 'check_baseline'), 'check_baseline should be removed'
assert hasattr(b, 'count_detected_and_simulated'), 'missing count_detected_and_simulated'
assert hasattr(b, 'compute_ratio'), 'missing compute_ratio'
assert hasattr(b, 'BASELINE_KEY'), 'missing BASELINE_KEY'
print('OK')
"
```

Expected: `OK`

- [ ] **Step 5: Spot-check the new functions with known values**

```bash
python3 -c "
import sys; sys.path.insert(0, '.')
import tests.benchmark as b, math

# compute_ratio: 10 detected out of 100 simulated
ratio, sigma = b.compute_ratio(10, 100)
assert abs(ratio - 0.1) < 1e-10, f'ratio={ratio}'
assert abs(sigma - math.sqrt(10)/100) < 1e-10, f'sigma={sigma}'

# compute_ratio: zero simulated
ratio, sigma = b.compute_ratio(0, 0)
assert ratio == 0.0 and sigma == 0.0

print('OK')
"
```

Expected: `OK`

- [ ] **Step 6: Commit**

```bash
cd /home/nuc/Hanna26-UCCeBr3
git add tests/benchmark.py
git commit -m "add count_detected_and_simulated and compute_ratio; remove count_lines and check_baseline"
```

---

## Task 2: Update `run_benchmark` — ratio output, baselines storage, default event count; update `append_benchmark_log`; update `main` default

**Files:**
- Modify: `tests/benchmark.py`

**Interfaces:**
- Consumes: `count_detected_and_simulated`, `compute_ratio` from Task 1
- Modifies: `run_benchmark(n_events)`, `append_benchmark_log(rows)`, `main()`
- `baselines.json` entries change from `int` to `{"ratio": float, "sigma": float}`

- [ ] **Step 1: Update `append_benchmark_log` to add `ratio` and `sigma` columns**

Replace the current `append_benchmark_log` function:

```python
def append_benchmark_log(rows):
    """Append timing rows to benchmark.log (TSV).

    Writes a header line the first time the file is created.
    Each row is a tuple of values that will be joined with tabs.
    Columns: date, git_hash, git_branch, cpu, variant, events,
             events_per_sec, ratio, sigma.
    """
    header = "date\tgit_hash\tgit_branch\tcpu\tvariant\tevents\tevents_per_sec\tratio\tsigma\n"
    write_header = not os.path.isfile(BENCHMARK_LOG)
    with open(BENCHMARK_LOG, "a") as f:
        if write_header:
            f.write(header)
        for row in rows:
            f.write("\t".join(str(v) for v in row) + "\n")
```

- [ ] **Step 2: Replace `run_benchmark` with the updated version**

Replace the current `run_benchmark` function with:

```python
def run_benchmark(n_events):
    """Run all scenarios and record events/sec, detection ratio, and sigma.

    Parses the output file after each run to compute the detection ratio
    (detected events / simulated events) and its Poisson uncertainty
    (sqrt(detected) / simulated). Stores ratio and sigma in baselines.json
    so the functional tests can compare against them.

    No pass/fail — this is for establishing baselines and tracking
    performance over time. Results are appended to benchmark.log (TSV,
    gitignored).
    """
    git_hash, git_branch = get_git_info()
    cpu = get_cpu_info()
    today = datetime.date.today().isoformat()

    print(f"\n=== Benchmark ({n_events} events, commit {git_hash}, branch {git_branch}) ===")
    print(f"CPU: {cpu}")
    print(f"{'Variant':<20} {'Events/sec':>12}  {'Ratio':>8}  {'Sigma':>8}")
    print("-" * 56)

    rows = []
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
            rows.append((today, git_hash, git_branch, cpu, variant,
                         n_events, "ERROR", "ERROR", "ERROR"))
            continue

        output_path = os.path.join(workdir, "output.out")
        n_detected, n_simulated = count_detected_and_simulated(output_path)
        ratio, sigma = compute_ratio(n_detected, n_simulated)

        print(f"  {variant:<20} {eps:>12.0f}  {ratio:>8.4f}  {sigma:>8.4f}")

        # Store ratio baseline for functional tests to compare against.
        baselines[variant] = {"ratio": ratio, "sigma": sigma}

        rows.append((today, git_hash, git_branch, cpu, variant,
                     n_events, f"{eps:.0f}", f"{ratio:.6f}", f"{sigma:.6f}"))

    save_baselines(baselines)
    append_benchmark_log(rows)
    print(f"\nBaselines updated in tests/baselines.json")
    print(f"Results appended to benchmark.log")
```

- [ ] **Step 3: Update the `--events` default in `main()` from 10000 to 1000000**

In `main()`, change:
```python
    parser.add_argument(
        "--events", type=int, default=10000,
        help="Event count for benchmark mode (default: 10000)"
    )
```
to:
```python
    parser.add_argument(
        "--events", type=int, default=1000000,
        help="Event count for benchmark mode (default: 1000000)"
    )
```

Also update the module docstring at the top of the file — change:
```
  make test-benchmark  # events/sec timing (10000 events)
```
to:
```
  make test-benchmark  # events/sec timing, detection ratio (1000000 events)
```

- [ ] **Step 4: Verify syntax**

```bash
python3 -c "import ast; ast.parse(open('tests/benchmark.py').read()); print('OK')"
```

Expected: `OK`

- [ ] **Step 5: Verify the default event count is updated**

```bash
python3 tests/benchmark.py --help | grep -A1 "events"
```

Expected output includes `default: 1000000`.

- [ ] **Step 6: Commit**

```bash
cd /home/nuc/Hanna26-UCCeBr3
git add tests/benchmark.py
git commit -m "benchmark: add detection ratio output, store in baselines.json, default 1M events"
```

---

## Task 3: Update `run_functional` — ratio comparison against benchmark baseline

**Files:**
- Modify: `tests/benchmark.py`

**Interfaces:**
- Consumes: `count_detected_and_simulated`, `compute_ratio`, `BASELINE_KEY` from Task 1; updated `baselines.json` format from Task 2
- Modifies: `run_functional(mode)`

- [ ] **Step 1: Replace `run_functional` with the updated version**

Replace the current `run_functional` function with:

```python
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

    if failures:
        print(f"\n{failures} FAILED")
        sys.exit(1)
    else:
        print(f"\nAll {mode} tests passed.\n")
```

- [ ] **Step 2: Verify syntax**

```bash
python3 -c "import ast; ast.parse(open('tests/benchmark.py').read()); print('OK')"
```

Expected: `OK`

- [ ] **Step 3: Verify [NO BASELINE] path works**

```bash
python3 -c "
import json
f = 'tests/baselines.json'
data = json.load(open(f))
cleaned = {k: v for k, v in data.items() if not isinstance(v, dict)}
json.dump(cleaned, open(f, 'w'), indent=2)
print('wiped ratio entries')
"
python3 tests/benchmark.py --mode sources 2>&1 | grep -E "NO BASELINE|PASS|FAIL"
git checkout tests/baselines.json
```

Expected: both scenarios print `[NO BASELINE]` and exit 0.

- [ ] **Step 4: Commit**

```bash
cd /home/nuc/Hanna26-UCCeBr3
git add tests/benchmark.py
git commit -m "functional tests: replace line-count check with detection ratio vs benchmark baseline"
```

---

## Task 4: Update `update_baselines` and run end-to-end verification

**Files:**
- Modify: `tests/benchmark.py`
- Modify: `tests/baselines.json` (populated by running the benchmark)

**Interfaces:**
- Modifies: `update_baselines()`

- [ ] **Step 1: Update `update_baselines` to re-run the benchmark instead of functional tests**

Replace the current `update_baselines` function with:

```python
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
```

- [ ] **Step 2: Verify syntax**

```bash
python3 -c "import ast; ast.parse(open('tests/benchmark.py').read()); print('OK')"
```

Expected: `OK`

- [ ] **Step 3: Run the benchmark to establish ratio baselines (1,000,000 events)**

```bash
cd /home/nuc/Hanna26-UCCeBr3
python3 tests/benchmark.py --mode benchmark
```

Expected: table with `Variant`, `Events/sec`, `Ratio`, `Sigma` for both scenarios. `tests/baselines.json` updated.

- [ ] **Step 4: Run the functional tests and confirm sigma readout**

```bash
python3 tests/benchmark.py --mode sources
```

Expected: both scenarios print `[PASS]` with ratio, baseline, and sigma. Example:
```
[PASS]           sources_cs137                  ratio=0.1430±0.0119  baseline=0.1418±0.0004  0.95σ
```

- [ ] **Step 5: Run the full suite**

```bash
python3 tests/benchmark.py --mode smoke && python3 tests/benchmark.py --mode sources
```

Expected: smoke tests unchanged, functional tests pass with sigma readout.

- [ ] **Step 6: Commit everything**

```bash
cd /home/nuc/Hanna26-UCCeBr3
git add tests/benchmark.py tests/baselines.json
git commit -m "update_baselines: re-run benchmark; add ratio baselines from 1M event run"
```
