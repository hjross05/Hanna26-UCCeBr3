# $Id: GNUmakefile,v 1.1 1999/01/07 16:05:40 gunter Exp $
# --------------------------------------------------------------
# GNUmakefile for examples module.  Gabriele Cosmo, 06/04/98.
# --------------------------------------------------------------

name := UCCeBrA

G4TARGET := $(name)
G4EXLIB := true

ifndef G4INSTALL
  G4INSTALL = ../../..
endif

# Collect the git branch and commit hash.
GIT_HASH := $(shell git describe --always --abbrev=6 --exclude '*')
GIT_BRANCH := $(shell git rev-parse --abbrev-ref HEAD)
ifneq ("$(wildcard git_hash)","")
PREV_GIT_HASH := $(shell cat git_hash)
else
PREV_GIT_HASH := ""
endif

.PHONY: all test test-smoke test-functional test-benchmark
all: include/Git_Hash.hh lib bin

# Run the full test suite (smoke + functional + benchmark).
# Requires the binary to be built first: make && make test
test:
	python3 tests/run_tests.py

# Run smoke tests only — quick check that the binary runs and produces output.
test-smoke:
	python3 tests/test_smoke.py

# Run functional tests only — output format, event counts, physics sanity.
test-functional:
	python3 tests/test_functional.py

# Run benchmark tests only — event rate logging and event count verification.
test-benchmark:
	python3 tests/test_benchmark.py

include/Git_Hash.hh: FORCE
ifneq ($(PREV_GIT_HASH),$(GIT_HASH))
	echo $(GIT_HASH) > git_hash
	echo "#ifndef Git_Hash_h" > include/Git_Hash.hh
	echo "#define Git_Hash_h" >> include/Git_Hash.hh
	echo "#define GIT_HASH \"$(GIT_HASH)\"" >> include/Git_Hash.hh
	echo "#define GIT_BRANCH \"$(GIT_BRANCH)\"" >> include/Git_Hash.hh
	echo "#endif" >> include/Git_Hash.hh
endif

FORCE:

include $(G4INSTALL)/config/binmake.gmk
