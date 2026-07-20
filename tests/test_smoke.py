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
#
# Test working directories are created under tests/tmp/ and left in place
# after the run so the user can inspect the macro file, geometry files,
# and output file.

import os
import sys
import unittest

# Ensure the repository root is on sys.path so this file can be run directly
# with 'python3 tests/test_smoke.py' as well as via the module form.
_REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if _REPO_ROOT not in sys.path:
    sys.path.insert(0, _REPO_ROOT)

from tests.output_parser import (
    find_binary, patch_mac, run_simulation,
    prepare_test_dir, copy_co60_geometry
)

# Paths to the example macro files, relative to repo root
REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CS137_MAC = os.path.join(REPO_ROOT, "examples", "cs137", "cs137_simple.mac")
CO60_MAC  = os.path.join(REPO_ROOT, "examples", "co60", "co60.mac")

SMOKE_EVENTS = 1000


class SmokeTestCs137(unittest.TestCase):
    """
    Smoke tests for the cs137_simple scenario.
    Runs the simulation once in tests/tmp/smoke_cs137/ and checks exit code
    and output file. The directory persists after the run for inspection.
    """

    @classmethod
    def setUpClass(cls):
        cls.binary  = find_binary()
        # Use a named subdirectory inside the repo to avoid macOS temp dir issues
        cls.testdir = prepare_test_dir("smoke_cs137")
        cls.outfile = os.path.join(cls.testdir, "smoke_cs137.out")
        cls.macfile = patch_mac(CS137_MAC, cls.outfile, SMOKE_EVENTS, dir=cls.testdir)
        cls.returncode, cls.stdout, cls.stderr, _ = run_simulation(
            cls.binary, cls.macfile, cwd=cls.testdir
        )

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
            f"Output file not found: {self.outfile}\nstdout: {self.stdout[:500]}"
        )
        self.assertGreater(
            os.path.getsize(self.outfile), 0,
            "Output file exists but is empty."
        )


class SmokeTestCo60(unittest.TestCase):
    """
    Smoke tests for the co60 scenario.
    co60.mac references demonstrator.geo and bricks.geo by relative path.
    These are copied into tests/tmp/smoke_co60/ and the binary is run from
    there so all inputs are visible in one place.
    """

    @classmethod
    def setUpClass(cls):
        cls.binary  = find_binary()
        cls.testdir = prepare_test_dir("smoke_co60")
        # Copy geometry files so the binary finds them by relative path
        copy_co60_geometry(cls.testdir)
        cls.outfile = os.path.join(cls.testdir, "smoke_co60.out")
        cls.macfile = patch_mac(CO60_MAC, cls.outfile, SMOKE_EVENTS, dir=cls.testdir)
        cls.returncode, cls.stdout, cls.stderr, _ = run_simulation(
            cls.binary, cls.macfile, cwd=cls.testdir
        )

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
            f"Output file not found: {self.outfile}\nstdout: {self.stdout[:500]}"
        )
        self.assertGreater(
            os.path.getsize(self.outfile), 0,
            "Output file exists but is empty."
        )


if __name__ == "__main__":
    unittest.main()
