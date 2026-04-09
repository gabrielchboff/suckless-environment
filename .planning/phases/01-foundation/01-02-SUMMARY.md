---
phase: 01-foundation
plan: 02
subsystem: dmenu-pipe
tags: [c, posix, fork, dup2, exec, pipe, dmenu, makefile]

requires:
  - phase: 01-foundation-01
    provides: dmenu.h header with DmenuCtx struct and function declarations, util.h/util.c shared library, config.mk build flags, Makefile skeleton
provides:
  - Full dmenu bidirectional pipe implementation (dmenu_open, dmenu_write, dmenu_read, dmenu_close)
  - test-dmenu compilation and API linkage validation program
  - install.sh integration for C utilities build and install
affects: [phase-2, phase-3, phase-4]

tech-stack:
  added: []
  patterns: [fork-dup2-exec-bidirectional-pipe, close-write-before-read-deadlock-prevention, handle-based-resource-api]

key-files:
  created:
    - utils/tests/test-dmenu.c
  modified:
    - utils/common/dmenu.c
    - utils/Makefile
    - install.sh

key-decisions:
  - "1024-byte malloc buffer for dmenu_read (heap-allocated, caller frees)"
  - "Close write end before reading in dmenu_read to prevent pipe deadlock"
  - "No sudo for utils install -- PREFIX=$(HOME)/.local is user-writable"

patterns-established:
  - "Handle-based API: open returns context, operations use context, close frees context"
  - "Pipe deadlock prevention: always close write end before reading from child"
  - "argv construction: count extras, malloc array, populate, free after fork"

requirements-completed: [FOUND-02, FOUND-04]

duration: 2min
completed: 2026-04-09
---

# Plan 01-02 Summary: dmenu Pipe Library + Install Integration

**Bidirectional dmenu pipe API using fork+dup2+exec with deadlock-safe read ordering and install.sh build pipeline**

## Performance

- **Duration:** 2 min
- **Started:** 2026-04-09T16:59:56Z
- **Completed:** 2026-04-09T17:02:25Z
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments
- Full dmenu.c implementation (124 lines) replacing stub with fork+dup2+exec bidirectional pipe
- test-dmenu.c validation program passing 11/11 tests (compile-time, link-time, runtime)
- install.sh extended with C utilities build and install section (no sudo, user-local)

## Task Commits

Each task was committed atomically:

1. **Task 1 (RED): Failing test for dmenu pipe API** - `52c7de6` (test)
2. **Task 1 (GREEN): Implement dmenu bidirectional pipe library** - `5cddd35` (feat)
3. **Task 2: Update install.sh with C utilities build and install** - `5def557` (feat)

_Task 1 followed TDD RED-GREEN cycle. No refactor needed._

## Files Created/Modified
- `utils/common/dmenu.c` - Full bidirectional pipe implementation (dmenu_open, dmenu_write, dmenu_read, dmenu_close)
- `utils/tests/test-dmenu.c` - Compilation, linkage, and runtime API validation test
- `utils/Makefile` - Added test-dmenu target, updated test and clean rules
- `install.sh` - Added C utilities build and install section after suckless tools

## Decisions Made
- Used 1024-byte heap-allocated buffer in dmenu_read (caller frees) -- selection length is unpredictable, malloc is cleaner than static buffer
- dmenu_read closes write end before reading to prevent deadlock per RESEARCH.md Pitfall 1
- install.sh uses `make -C "$REPO_DIR/utils"` (not `cd` + `make`) for consistency
- No sudo on utils install lines -- PREFIX=$(HOME)/.local is user-writable

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None - both tasks completed without issues. dmenu binary was available on the system, so the runtime integration test ran and passed (dmenu_read returned NULL as expected in non-interactive mode).

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Foundation layer complete: util.c + dmenu.c compiled and tested
- All subsequent phases can link against common/util.o and common/dmenu.o
- install.sh pipeline ready: `make -C utils clean all` builds everything, `make -C utils install` deploys to ~/.local/bin
- BINS variable in Makefile is empty -- Phase 2+ utilities will add their entries and automatically get built and installed

## Self-Check: PASSED

All files verified present. All 3 commits verified in git log. Build and test suite passes (11/11 test-dmenu, 10/10 test-util).

---
*Phase: 01-foundation*
*Completed: 2026-04-09*
