# Design: AGENTS.md for UCCeBrA

**Date:** 2026-07-20
**Status:** Approved for implementation

## Context

UCCeBrA is a Geant4-based Monte Carlo simulation written in C++, used for nuclear
physics experiments. It has no existing AGENTS.md, CLAUDE.md, or contributor
guidelines. The codebase is physics-sensitive: a change that compiles cleanly can
produce physically incorrect simulation output with no visible error. A testing suite
is planned but not yet present.

The AGENTS.md must serve both AI agents and human contributors. It is written so a
human can read it naturally from top to bottom, and so an AI agent can locate the
relevant section quickly by heading.

## Audience

- Primary AI agent: OpenCode with the Superpowers plugin, Claude Sonnet 4.6 via API
- Human contributors: physicists and developers joining the project
- Physics domain knowledge assumed: none

## Document Structure

1. Project Overview
2. Geant4 Primer
3. Repository Layout
4. Build System
5. Code Conventions
6. Danger Zones
7. Python Post-Processing
8. OpenCode / Superpowers Guidance

## Design Decisions

| Decision | Rationale |
|----------|-----------|
| Reference Manual structure | Physics domain gap requires grounding, not just a task cookbook |
| Prose-first writing | Serves human contributors; bullet-only docs are harder to read as narrative |
| Physics warnings repeated | Risk is high enough to warrant redundancy across sections |
| No physics knowledge assumed | Audience includes physicists who are not software experts |
| Examples described as illustrative | They show how the code can be used for real scenarios |
| Readability prioritised | Codebase users are physicists first; readable code reduces errors |
| Comments required on changes | Necessary when no test suite exists yet |
