# Design: UCCeBrA Testing Suite

**Date:** 2026-07-20
**Status:** Approved for implementation

## Context

UCCeBrA has no automated test infrastructure. The simulation binary accepts a single
macro file argument and writes ASCII output event-by-event as the simulation runs.
The testing suite must work without extra Python package installs (plain `unittest`)
and requires the binary to be compiled before running.

## Decisions

| Decision | Rationale |
|----------|-----------|
| `unittest` (no pytest) | No additional installs required |
| Requires built binary | Tests exercise real simulation behaviour |
| Macros generated at runtime | Derived from existing examples; no static test macros added to repo |
| 1,000 events for smoke/functional | Fast enough for TDD cycles |
| 10,000 events for benchmark | More stable rate measurement |
| Event rate → `benchmark.log` (gitignored) | Hardware-dependent; not meaningful to track in git |
| Event count → `event-counts.json` (tracked) | Hardware-independent ground truth |
| Detailed processor info in both files | Context for interpreting results |
| Cross-platform (Linux/macOS/Windows) | Uses platform module + OS-specific tools, no extra installs |

## Repository Structure

```
tests/
├── __init__.py                    # Makes tests/ a Python package
├── run_tests.py                   # Entry point: discovers and runs all suites
├── output_parser.py               # Shared utility: mac patching, output parsing, binary discovery
├── test_smoke.py                  # Smoke tests (1,000 events, cs137_simple + co60)
├── test_functional.py             # Functional tests (1,000 events, cs137_simple + co60)
├── test_benchmark.py              # Benchmark tests (10,000 events, cs137_simple + co60)
├── benchmark.log                  # Event rate log — appended each run, gitignored
└── benchmarks/
    └── event-counts.json          # Persistent event count ground truth, tracked by git
```

`.gitignore` additions: `tests/benchmark.log`, `tests/*.out`, `tests/tmp/`,
`tests/ucce_test_*.mac`

## Module: `output_parser.py`

Shared by all test files. Provides:

- **`find_binary()`** — locates `bin/Linux-g++/UCCeBrA` relative to repo root;
  raises `FileNotFoundError` with a clear message if not found
- **`get_git_hash()`** — returns short git hash for benchmark log entries
- **`get_processor_info()`** — returns dict with keys `cpu` (str), `cores` (int),
  `mhz` (float), `os` (str); cross-platform via `/proc/cpuinfo`, `sysctl`, `wmic`
- **`patch_mac(source_mac, output_filepath, n_events)`** — reads source `.mac`,
  replaces `/run/beamOn` and `/Output/Filename` (absolute path), writes to a
  `NamedTemporaryFile`, returns its path
- **`run_simulation(binary_path, macro_path, cwd)`** — runs binary as subprocess,
  returns `(returncode, stdout, stderr, elapsed_seconds)`
- **`parse_output(filepath)`** — parses UCCeBrA ASCII output; returns list of event
  dicts, each with: `event_id` (int), `n_dets_hit` (int), `hits` (list of dicts:
  `det_id`, `edep`, `x`, `y`, `z`, `fep`, `global_time`), `n_emitted` (int),
  `emitted_gammas` (list of dicts: `energy`, `x`, `y`, `z`, `phi`, `theta`)

## Smoke Tests (`test_smoke.py`)

Event count: 1,000. One `TestCase` class per scenario with `setUpClass` running the
simulation once, shared across all tests in the class.

| Test | Scenario | Assertion |
|------|----------|-----------|
| `test_binary_exits_cleanly` | cs137_simple | return code == 0 |
| `test_output_file_created_and_nonempty` | cs137_simple | output file exists and size > 0 |
| `test_binary_exits_cleanly` | co60 | return code == 0 |
| `test_output_file_created_and_nonempty` | co60 | output file exists and size > 0 |

Note: co60 tests must be run with `cwd=examples/co60/` because `co60.mac` references
`demonstrator.geo` and `bricks.geo` by relative path.

## Functional Tests (`test_functional.py`)

Event count: 1,000. Separate `TestCase` classes for cs137 and co60.

**cs137_simple checks:**

| Test | Assertion |
|------|-----------|
| `test_line_format` | Every line is D, C, E, gamma sub-record, or blank |
| `test_d_line_fields` | D-lines have 2 parseable fields (int, int) |
| `test_c_line_fields` | C-lines have 7 parseable fields (int, float×5, int, float) |
| `test_e_line_fields` | E-lines have 2 parseable fields (int, int) |
| `test_gamma_subrecord_fields` | Gamma sub-records have 6 parseable float fields |
| `test_event_count` | Unique event IDs == 1,000 |
| `test_d_c_line_consistency` | NDetsHit on each D-line matches C-lines that follow |
| `test_e_gamma_consistency` | NEmittedGammas on each E-line matches sub-records |
| `test_energy_bounds` | All C-line Edep > 0.0 and ≤ 662.0 keV |
| `test_detector_id` | All C-line DetID == 1 (single detector) |
| `test_fep_flag` | All FEP values are 0 or 1 |
| `test_global_time` | All GlobalTime ≥ 0.0 |
| `test_emitted_energy` | All emitted gamma energies == 662.0 ± 0.01 keV |

**co60 checks:**

| Test | Assertion |
|------|-----------|
| `test_line_format` | Every line is D, C, E, gamma sub-record, or blank |
| `test_event_count` | Unique event IDs == 1,000 |
| `test_d_c_line_consistency` | NDetsHit on each D-line matches C-lines that follow |
| `test_e_gamma_consistency` | NEmittedGammas on each E-line matches sub-records |
| `test_energy_bounds` | All C-line Edep > 0.0 and ≤ 1332.0 keV |
| `test_detector_id_range` | All C-line DetID in range 1–9 |
| `test_fep_flag` | All FEP values are 0 or 1 |
| `test_global_time` | All GlobalTime ≥ 0.0 |

## Benchmark Tests (`test_benchmark.py`)

Event count: 10,000. One `TestCase` class with two tests.

| Test | Scenario | Assertions | Side effects |
|------|----------|------------|--------------|
| `test_cs137_benchmark` | cs137_simple | event count == JSON expected | Appends to `benchmark.log`; updates `event-counts.json` |
| `test_co60_benchmark` | co60 | event count == JSON expected | Appends to `benchmark.log`; updates `event-counts.json` |

If `event-counts.json` does not exist, or a scenario is not yet recorded, the test
creates/updates the file with the current run's results and passes. On subsequent
runs the test asserts the event count matches the stored expected value.

**`benchmark.log` format (one line per run, appended):**
```
2026-07-20 14:32:01 | cs137_simple | 10000 events | 47832 events/s | bin: cc60a8d | CPU: Intel Core i7-9700K | cores: 8 | MHz: 3600.0 | OS: Linux 5.15.0
```

**`event-counts.json` format:**
```json
{
  "cs137_simple": {
    "expected_events": 10000,
    "cpu": "Intel Core i7-9700K",
    "cores": 8,
    "mhz": 3600.0,
    "os": "Linux 5.15.0"
  },
  "co60": {
    "expected_events": 10000,
    "cpu": "Intel Core i7-9700K",
    "cores": 8,
    "mhz": 3600.0,
    "os": "Linux 5.15.0"
  }
}
```

## `run_tests.py`

Discovers and runs all three suites in order (smoke → functional → benchmark).
Prints a human-readable summary. Individual suites also runnable directly.

## Output Format Reference (from `src/EventAction.cc`)

The UCCeBrA ASCII output format:

- **D-line:** `D<NDetsHit><event_id>` — detected event header (only if Nhits > 0)
- **C-line:** `C<DetID><Edep_keV><X_mm><Y_mm><Z_mm><FEP><GlobalTime>` — per-detector hit
- **E-line:** `E<NEmittedGammas><event_id>` — emitted gamma header
- **Gamma sub-record:** `     <Energy_keV><X><Y><Z><Phi_rad><Theta_rad>` — 5 leading spaces

Column positions are positional (whitespace-delimited). The sort scripts parse by
`words[N]` after split(). Any change to `EventAction.cc` output format must be
coordinated with both the sort scripts and these tests.
