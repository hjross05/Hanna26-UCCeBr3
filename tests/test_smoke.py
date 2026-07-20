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
# co60.mac references demonstrator.geo and bricks.geo by relative path,
# so the simulation must be run from the co60 example directory.
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
        # Run from tmpdir — cs137_simple.mac has no relative file references
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
