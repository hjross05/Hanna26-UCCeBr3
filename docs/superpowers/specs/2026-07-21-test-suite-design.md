# UCCeBrA Test Suite Design

**Date:** 2026-07-21
**Status:** Approved

---

## 1. Overview

A testing suite for UCCeBrA, modelled directly on the UCGretina `tests/benchmark.py`.
Provides automated regression testing: detects build breakage, output-format
regressions, and statistically significant changes in detection rates.

Physics correctness is explicitly out of scope for automated testing; it requires
human domain expert review.

---

## 2. Goals

- Mirror the structure and conventions of the UCGretina test suite as closely as
  possible.
- Replace the old split-file suite (`test_smoke.py`, `test_functional.py`,
  `test_benchmark.py`) with a single driver script.
- Require no third-party Python dependencies (standard library only).

---

## 3. Files Created or Modified

### New files

| Path | Purpose |
|------|---------|
| `tests/benchmark.py` | Single test driver script |
| `tests/baselines.json` | Auto-created on first run; git-tracked |
| `docs/superpowers/specs/2026-07-21-test-suite-design.md` | This spec |

### Modified files

| Path | Change |
|------|--------|
| `GNUmakefile` | Update `test`, `test-smoke`, `test-functional`, `test-benchmark` targets |
| `AGENTS.md` | Rewrite testing section |

### Already correct

| Path | Status |
|------|--------|
| `.gitignore` | `git_hash`, `tests/tmp/`, and `*.log` already excluded — no changes needed |

---

## 4. Directory Structure

```
tests/
├── benchmark.py        # Single test driver
├── baselines.json      # Line-count baselines; git-tracked; auto-created on first run
├── benchmark.log       # Events/sec log; gitignored (covered by *.log rule)
└── tmp/                # Per-test working directories; gitignored (already in .gitignore)
```

---

## 5. Make Targets

The four `.PHONY` targets already declared in `GNUmakefile` are updated to:

| Target | Invocation | Purpose |
|--------|-----------|---------|
| `make test` | `python3 tests/benchmark.py --mode smoke && python3 tests/benchmark.py --mode sources` | Full suite (smoke + functional) |
| `make test-smoke` | `python3 tests/benchmark.py --mode smoke` | Quick sanity: binary runs, no fatal, events/sec > 0 |
| `make test-functional` | `python3 tests/benchmark.py --mode sources` | Line-count regression against baselines |
| `make test-benchmark` | `python3 tests/benchmark.py --mode benchmark` | Events/sec timing; logged to `benchmark.log` |

---

## 6. Test Cases

### 6.1 Smoke tests (`--mode smoke`, 100 events)

Pass criteria: exit code 0, no fatal error strings in stdout + stderr, end-of-run
`events/s` line present in stdout, events/sec > 0.

| Test name | Binary | Macro | Support files copied |
|-----------|--------|-------|----------------------|
| `smoke_cs137` | `UCCeBrA` | `examples/cs137/cs137_simple.mac` | _(none)_ |
| `smoke_co60` | `UCCeBrA` | `examples/co60/co60.mac` | `demonstrator.geo`, `bricks.geo` |

### 6.2 Functional tests (`--mode sources`, 1000 events)

Pass criteria: all smoke criteria, plus output file `output.out` exists and line
count is within `2 * sqrt(baseline)` of the stored baseline. If no baseline exists,
one is set automatically and the test passes with `[BASELINE SET]`.

| Test name | Binary | Macro | Support files copied |
|-----------|--------|-------|----------------------|
| `sources_cs137` | `UCCeBrA` | `examples/cs137/cs137_simple.mac` | _(none)_ |
| `sources_co60` | `UCCeBrA` | `examples/co60/co60.mac` | `demonstrator.geo`, `bricks.geo` |

### 6.3 Benchmark tests (`--mode benchmark`, 10,000 events by default)

No pass/fail — measures events/sec only. Results appended to `benchmark.log` (TSV).

| Variant label | Binary | Macro |
|---------------|--------|-------|
| `UCCeBrA_cs137` | `UCCeBrA` | `examples/cs137/cs137_simple.mac` |
| `UCCeBrA_co60` | `UCCeBrA` | `examples/co60/co60.mac` |

---

## 7. Script Structure (`tests/benchmark.py`)

A direct port of UCGretina's `tests/benchmark.py`, adapted for UCCeBrA's single
binary and two example scenarios.

### Command-line interface

```
python3 tests/benchmark.py --mode {smoke,sources,benchmark}
                            [--events N]           # benchmark mode only; default 10000
                            [--update-baselines]   # reset all baselines to current values
```

### Infrastructure functions

All ported directly from UCGretina's benchmark.py:

| Function | Purpose |
|----------|---------|
| `find_binary(name)` | Locate binary via `$G4WORKDIR/bin/$G4SYSTEM/<name>`; exit on missing |
| `setup_workdir(test_name, example_path, support_files)` | Create `tests/tmp/<name>/`, wipe if exists, copy support files |
| `write_base_macro(base_macro_path, example_path, output_command, workdir)` | Strip `/run/beamOn` and `/Output/Filename` from original macro; append output command |
| `write_run_macro(base_macro_path, n_events, wrapper_path)` | Write 2-line wrapper: `/control/execute <base>` then `/run/beamOn N` |
| `run_sim(binary, macro_path, workdir)` | `subprocess.run`; returns `(stdout, stderr, returncode)` |
| `parse_events_per_sec(stdout)` | Regex for `NNN events/s`; takes last match to skip progress lines |
| `check_fatal(stdout, stderr)` | Scans for `Fatal Exception`, `Segmentation fault`, `FatalException`, `G4Exception : Fatal` |
| `count_lines(filepath)` | `wc -l` via subprocess (Linux/macOS) |
| `load_baselines()` / `save_baselines(data)` | Read/write `baselines.json` with `_meta` provenance |
| `check_baseline(name, observed, baselines)` | `|observed - baseline| ≤ 2√baseline`; auto-sets if absent |
| `get_git_info()` | 6-char hash + branch name |
| `get_cpu_info()` | `/proc/cpuinfo` (Linux), `sysctl` (macOS), hostname fallback |
| `append_benchmark_log(rows)` | TSV append to `benchmark.log`; writes header if new file |

---

## 8. Macro Transformation

Each test runs in an isolated `tests/tmp/<test_name>/` directory. The original
example macros are never modified. The transformation is:

1. `write_base_macro` reads the original `.mac`, strips `/Output/Filename` and
   `/run/beamOn` lines, and writes a base macro to the working directory with the
   test's `/Output/Filename output.out` command appended.
2. `write_run_macro` writes a 2-line wrapper:
   ```
   /control/execute func_<test_name>.mac
   /run/beamOn N
   ```
3. The simulation is run with the wrapper as its argument and the working directory
   as `cwd`.

---

## 9. `baselines.json` Format

```json
{
  "_meta": {
    "git_hash": "a1b2c3",
    "git_branch": "main",
    "cpu": "Intel(R) Core(TM) i7-14700"
  },
  "sources_cs137": 142,
  "sources_co60": 2500
}
```

Baseline values are hardware-independent line counts, not timings. The `_meta` key
records provenance only and is stripped before comparisons. Baselines are
auto-created on first run; `--update-baselines` resets all of them.

---

## 10. `benchmark.log` Format

TSV with columns: `date`, `git_hash`, `git_branch`, `cpu`, `variant`, `events`,
`events_per_sec`. Header written once when the file is first created. Hardware-
dependent; gitignored.

---

## 11. AGENTS.md Changes

The testing section will be rewritten to:

- Replace all references to `test_smoke.py`, `test_functional.py`,
  `test_benchmark.py` with the four `make test*` targets.
- Describe `baselines.json` (git-tracked, hardware-independent) and `benchmark.log`
  (gitignored, hardware-dependent).
- Retain the existing caveat: "it compiles and the example runs is a necessary
  condition, not a sufficient one — physics correctness requires human review."

---

## 12. Out of Scope

- Output format validation (D/C/E record parsing, energy bounds, detector ID checks)
- Physics correctness verification
- Integration with CI systems
- Parallel test execution
- Windows support
