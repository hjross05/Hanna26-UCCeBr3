# tests/test_functional.py
#
# Functional tests for UCCeBrA output format, event counts, and physics sanity.
# Tests both the cs137_simple (single detector, 662 keV direct gun) and
# co60 (9-detector array, radioactive decay) scenarios at 1,000 events.
#
# Each class runs the simulation once in setUpClass and parses the output.
# All tests in the class share the parsed result — the binary is only
# invoked once per scenario.

import os
import unittest
import tempfile

from tests.output_parser import find_binary, patch_mac, run_simulation, parse_output

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CS137_MAC = os.path.join(REPO_ROOT, "examples", "cs137", "cs137_simple.mac")
CO60_MAC  = os.path.join(REPO_ROOT, "examples", "co60", "co60.mac")
# co60.mac references geometry files by relative path; must run from this directory
CO60_CWD  = os.path.join(REPO_ROOT, "examples", "co60")

FUNCTIONAL_EVENTS = 1000

# cs137_simple emits a monoenergetic 662 keV gamma (Cs-137 photopeak).
# No deposited energy can exceed the source energy.
CS137_MAX_ENERGY_KEV = 662.0
CS137_DETECTOR_ID    = 1       # single detector setup — only detector ID 1 is valid
CS137_EMITTED_ENERGY = 662.0
CS137_ENERGY_TOL     = 0.01    # keV tolerance for emitted energy check

# co60 emits a 1173/1332 keV cascade. Max possible deposited energy is 1332 keV.
CO60_MAX_ENERGY_KEV  = 1332.0
CO60_MIN_DETECTOR_ID = 1
CO60_MAX_DETECTOR_ID = 9       # demonstrator array has 9 detectors


def _validate_raw_line_format(line):
    """
    Return True if a raw output line has a valid record type:
      - starts with 'D' (detected event header)
      - starts with 'C' (hit record)
      - starts with 'E' (emitted gamma header)
      - starts with exactly 5 spaces (gamma sub-record)
      - is blank
    Any other prefix indicates a malformed or unexpected line.
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
        """D-lines must have 2 parseable fields: int NDetsHit, int event_id."""
        for ev in self.events:
            # n_dets_hit is parsed from the D-line; value 0 means no D-line was present
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
                    self.assertIsInstance(g[key], float,
                        f"Event {ev['event_id']}: gamma field '{key}' is not float")

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
        """Deposited energy must be > 0 and <= 662.0 keV (cs137 source energy)."""
        for ev in self.events:
            for hit in ev["hits"]:
                self.assertGreater(hit["edep"], 0.0,
                    f"Event {ev['event_id']}: non-positive energy {hit['edep']} keV")
                self.assertLessEqual(hit["edep"], CS137_MAX_ENERGY_KEV,
                    f"Event {ev['event_id']}: energy {hit['edep']} keV exceeds "
                    f"source energy {CS137_MAX_ENERGY_KEV} keV")

    def test_detector_id(self):
        """All hits must be on detector 1 (cs137_simple is a single-detector setup)."""
        for ev in self.events:
            for hit in ev["hits"]:
                self.assertEqual(hit["det_id"], CS137_DETECTOR_ID,
                    f"Event {ev['event_id']}: unexpected detector ID {hit['det_id']}")

    def test_fep_flag(self):
        """FEP (full-energy peak) flag must be 0 or 1."""
        for ev in self.events:
            for hit in ev["hits"]:
                self.assertIn(hit["fep"], (0, 1),
                    f"Event {ev['event_id']}: invalid FEP flag {hit['fep']}")

    def test_global_time(self):
        """GlobalTime must be >= 0 (time cannot be negative)."""
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
                    msg=(
                        f"Event {ev['event_id']}: emitted energy {g['energy']} keV "
                        f"not within {CS137_ENERGY_TOL} keV of {CS137_EMITTED_ENERGY} keV"
                    )
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

    def test_line_format(self):
        """Every line must be a valid record type: D, C, E, sub-record, or blank."""
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
        """Deposited energy must be > 0 and <= 1332.0 keV (co60 max gamma energy)."""
        for ev in self.events:
            for hit in ev["hits"]:
                self.assertGreater(hit["edep"], 0.0,
                    f"Event {ev['event_id']}: non-positive energy {hit['edep']} keV")
                self.assertLessEqual(hit["edep"], CO60_MAX_ENERGY_KEV,
                    f"Event {ev['event_id']}: energy {hit['edep']} keV exceeds "
                    f"max co60 gamma energy {CO60_MAX_ENERGY_KEV} keV")

    def test_detector_id_range(self):
        """All detector IDs must be in range 1-9 (demonstrator array has 9 detectors)."""
        for ev in self.events:
            for hit in ev["hits"]:
                self.assertGreaterEqual(hit["det_id"], CO60_MIN_DETECTOR_ID,
                    f"Event {ev['event_id']}: detector ID {hit['det_id']} < 1")
                self.assertLessEqual(hit["det_id"], CO60_MAX_DETECTOR_ID,
                    f"Event {ev['event_id']}: detector ID {hit['det_id']} > 9")

    def test_fep_flag(self):
        """FEP (full-energy peak) flag must be 0 or 1."""
        for ev in self.events:
            for hit in ev["hits"]:
                self.assertIn(hit["fep"], (0, 1),
                    f"Event {ev['event_id']}: invalid FEP flag {hit['fep']}")

    def test_global_time(self):
        """GlobalTime must be >= 0 (time cannot be negative)."""
        for ev in self.events:
            for hit in ev["hits"]:
                self.assertGreaterEqual(hit["global_time"], 0.0,
                    f"Event {ev['event_id']}: negative GlobalTime {hit['global_time']}")


if __name__ == "__main__":
    unittest.main()
