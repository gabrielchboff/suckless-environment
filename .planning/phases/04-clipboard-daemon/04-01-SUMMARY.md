---
phase: 04-clipboard-daemon
plan: 01
subsystem: clipboard
tags: [xfixes, x11, clipboard, daemon, fnv1a, flock, select, sigterm]

# Dependency graph
requires:
  - phase: 01-foundation
    provides: "common/util.o (die, warn, argv0), config.mk, utils/Makefile structure"
  - phase: 03-interactive-utilities
    provides: "dmenu-clip cache format contract (CACHE_DIR_NAME, hash-named files, mtime ordering)"
provides:
  - "dmenu-clipd.c -- event-driven X11 clipboard daemon"
  - "config.def.h/config.h for dmenu-clipd compile-time configuration"
  - "FNV-1a 64-bit content hashing for cache filenames"
  - "XFixes-based clipboard monitoring (zero CPU when idle)"
affects: [04-02-PLAN, install.sh, dwm-start]

# Tech tracking
tech-stack:
  added: [libX11, libXfixes, XFixes extension, FNV-1a hash]
  patterns: [select()+ConnectionNumber event loop, flock single-instance, volatile sig_atomic_t signal flag]

key-files:
  created:
    - utils/dmenu-clipd/dmenu-clipd.c
    - utils/dmenu-clipd/config.def.h
    - utils/dmenu-clipd/config.h
  modified: []

key-decisions:
  - "FNV-1a 64-bit chosen over MD5 -- zero dependencies, inline ~10 lines, 16-char hex filenames"
  - "select() with 1s timeout for SIGTERM responsiveness instead of bare XNextEvent"
  - "UTF8_STRING with STRING fallback for maximum clipboard text compatibility"

patterns-established:
  - "XFixes event-driven pattern: register XFixesSelectSelectionInput, handle via xfixes_event_base + XFixesSelectionNotify"
  - "Interruptible event loop: select(ConnectionNumber(dpy)) + XPending drain before select"
  - "flock advisory lock for daemon single-instance enforcement"

requirements-completed: [CLIPD-01, CLIPD-02, CLIPD-03, CLIPD-04, CLIPD-05]

# Metrics
duration: 2min
completed: 2026-04-09
---

# Phase 4 Plan 1: Clipboard Daemon Implementation Summary

**Event-driven X11 clipboard daemon using XFixes, FNV-1a dedup, flock single-instance, and select()-based interruptible event loop**

## Performance

- **Duration:** 2 min
- **Started:** 2026-04-09T20:00:34Z
- **Completed:** 2026-04-09T20:02:33Z
- **Tasks:** 2
- **Files created:** 3

## Accomplishments
- Complete dmenu-clipd.c daemon (405 lines) replacing shell script's 0.5s xclip polling with XFixes events
- Cache format fully compatible with dmenu-clip: same directory, hash-named files, mtime ordering
- All 5 CLIPD requirements addressed: XFixes monitoring, FNV-1a dedup, LRU pruning, flock locking, graceful SIGTERM shutdown

## Task Commits

Each task was committed atomically:

1. **Task 1: Create config.def.h and config.h for dmenu-clipd** - `c4e85b7` (feat)
2. **Task 2: Implement dmenu-clipd.c -- complete clipboard daemon** - `08807c7` (feat)

## Files Created/Modified
- `utils/dmenu-clipd/config.def.h` - Compile-time config: CACHE_DIR_NAME="dmenu-clipboard", MAX_ENTRIES=50
- `utils/dmenu-clipd/config.h` - User copy of config.def.h (suckless pattern)
- `utils/dmenu-clipd/dmenu-clipd.c` - Complete clipboard daemon: XFixes events, FNV-1a hash, flock, select() loop, SIGTERM handling, LRU pruning

## Decisions Made
- FNV-1a 64-bit over MD5: zero dependencies, 10 lines inline, 16-char hex filenames with negligible collision probability for 50 entries
- select() with 1-second timeout: standard POSIX pattern for interruptible daemon loops, ensures SIGTERM response within 1s
- UTF8_STRING with STRING fallback: handles both modern and legacy X11 applications
- Bounded SelectionNotify wait (50 retries at 10ms = 500ms max): prevents infinite hang if clipboard owner disappears
- XA_STRING stored in static Atom variable (not XInternAtom'd): it is predefined in Xatom.h

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- dmenu-clipd.c source complete, ready for Makefile integration in Plan 02
- Plan 02 will add build rules to utils/Makefile with -lX11 -lXfixes link flags
- Plan 02 will update install.sh and dwm-start to use the compiled binary

---
*Phase: 04-clipboard-daemon*
*Completed: 2026-04-09*
