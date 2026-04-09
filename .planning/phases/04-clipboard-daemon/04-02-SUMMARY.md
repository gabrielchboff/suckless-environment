---
phase: 04-clipboard-daemon
plan: 02
subsystem: clipboard
tags: [makefile, build-system, x11-linking, compilation]

# Dependency graph
requires:
  - phase: 04-clipboard-daemon
    plan: 01
    provides: "dmenu-clipd.c, config.def.h, config.h source files"
  - phase: 01-foundation
    provides: "common/util.o, config.mk, utils/Makefile structure"
provides:
  - "Build rules for dmenu-clipd with X11 library linking"
  - "Compiled dmenu-clipd binary linked against libX11 and libXfixes"
affects: [install.sh, dwm-start]

# Tech tracking
tech-stack:
  added: []
  patterns: [per-target-library-flags]

key-files:
  created: []
  modified:
    - utils/Makefile
    - .gitignore

key-decisions:
  - "X11_LIBS variable scoped to dmenu-clipd link rule only -- no other utility links X11"
  - "Updated .gitignore to exclude all compiled utility binaries (were previously untracked)"

patterns-established:
  - "Per-target library flag pattern: $(X11_LIBS) appended after $^ in link rule for target-specific dependencies"

requirements-completed: [CLIPD-01, CLIPD-02, CLIPD-03, CLIPD-04, CLIPD-05]

# Metrics
duration: 2min
completed: 2026-04-09
status: checkpoint-pending
---

# Phase 4 Plan 2: Clipboard Daemon Build Integration Summary

**Makefile integration for dmenu-clipd with X11_LIBS (-lX11 -lXfixes) scoped linking and zero-warning compilation of all 6 utilities**

## Status: CHECKPOINT PENDING

Task 1 (build integration) is complete. Task 2 (human verification of end-to-end clipboard workflow) requires user interaction in a live X11 session.

## Performance

- **Duration:** 2 min
- **Started:** 2026-04-09T20:05:03Z
- **Tasks completed:** 1 of 2
- **Files modified:** 2

## Accomplishments

- Added X11_LIBS variable (-lX11 -lXfixes) to utils/Makefile, scoped exclusively to dmenu-clipd
- Added dmenu-clipd to BINS list (now 6 total utilities)
- Added link rule using $(COMMON_OBJ) + $(X11_LIBS) without $(DMENU_OBJ) -- daemon does not use dmenu
- Added compile rule with correct dependencies (dmenu-clipd.c, config.h, util.h)
- Added clean target for dmenu-clipd object files
- Updated .gitignore to exclude all compiled utility binaries
- Verified zero-warning build of all 6 utilities
- Verified dmenu-clipd links libX11.so and libXfixes.so
- Verified other utilities (dmenu-clip) do NOT link X11

## Task Commits

1. **Task 1: Add dmenu-clipd build rules to utils/Makefile** - `9ef59a3` (feat)

## Files Modified

- `utils/Makefile` - Added X11_LIBS variable, dmenu-clipd to BINS, link/compile/clean rules
- `.gitignore` - Added utils/ compiled binary paths to prevent untracked files

## Decisions Made

- X11_LIBS as a dedicated variable rather than inline flags: cleaner, documents the dependency, easy to extend
- Updated .gitignore for all utils/ binaries (not just dmenu-clipd) since they were all untracked generated files

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Added compiled binaries to .gitignore**
- **Found during:** Task 1 commit preparation
- **Issue:** All 6 compiled utility binaries appeared as untracked files -- they were never in .gitignore
- **Fix:** Added all utils/ binary paths to root .gitignore
- **Files modified:** .gitignore
- **Commit:** 9ef59a3

## Verification Results

All acceptance criteria verified:
- BINS list includes dmenu-clipd/dmenu-clipd (6 total utilities)
- X11_LIBS variable defined as -lX11 -lXfixes
- dmenu-clipd link rule uses $(COMMON_OBJ) and $(X11_LIBS) but NOT $(DMENU_OBJ)
- dmenu-clipd compile rule depends on dmenu-clipd.c, config.h, and common/util.h
- clean target includes rm -f dmenu-clipd/*.o
- `make -C utils clean all` succeeds with ZERO warnings
- All 6 binaries exist after build
- No other utility's link rule references X11_LIBS
- ldd confirms dmenu-clipd links libX11.so.6 and libXfixes.so.3
- ldd confirms dmenu-clip does NOT link X11

## Threat Model Compliance

- T-04-08 (Build flags): Mitigated -- config.mk enforces -Wall -Wextra -pedantic, zero-warning build verified, -Os optimization, -s strips symbols
- T-04-09 (Binary installation): Accepted -- installs to user-writable ~/.local/bin via `make install`, no SUID/SGID

## Known Stubs

None.

---
*Phase: 04-clipboard-daemon*
*Completed: 2026-04-09 (Task 1 only -- Task 2 checkpoint pending)*
