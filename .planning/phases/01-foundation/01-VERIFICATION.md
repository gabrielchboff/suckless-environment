---
phase: 01-foundation
verified: 2026-04-09T17:30:00Z
status: human_needed
score: 9/11 must-haves verified
overrides_applied: 0
gaps:
  - truth: "All 6 utility subdirectories exist per D-05"
    status: failed
    reason: "SUMMARY claims directories exist but only utils/common/ and utils/tests/ are present"
    artifacts:
      - path: "utils/dmenu-session/"
        issue: "Directory does not exist"
      - path: "utils/dmenu-cpupower/"
        issue: "Directory does not exist"
      - path: "utils/dmenu-clip/"
        issue: "Directory does not exist"
      - path: "utils/dmenu-clipd/"
        issue: "Directory does not exist"
      - path: "utils/battery-notify/"
        issue: "Directory does not exist"
      - path: "utils/screenshot-notify/"
        issue: "Directory does not exist"
    missing:
      - "Run: mkdir -p utils/dmenu-session utils/dmenu-cpupower utils/dmenu-clip utils/dmenu-clipd utils/battery-notify utils/screenshot-notify"
human_verification:
  - test: "Run install.sh end-to-end on a clean system"
    expected: "suckless tools build, C utilities build, scripts install, no errors"
    why_human: "install.sh runs sudo pacman and requires interactive package confirmation; also builds suckless tools from source which may fail on missing deps"
  - test: "Verify dmenu_read returns NULL on Escape key press"
    expected: "User opens a dmenu pipe, presses Escape, dmenu_read returns NULL"
    why_human: "Requires interactive X11 display and manual Escape keypress; automated test can only verify NULL return in non-interactive mode"
---

# Phase 1: Foundation Verification Report

**Phase Goal:** Every subsequent utility can be built against a shared library with a consistent Makefile convention, and install.sh compiles and installs everything
**Verified:** 2026-04-09T17:30:00Z
**Status:** human_needed
**Re-verification:** No -- initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | make -C utils compiles common/util.o and common/dmenu.o without errors or warnings | VERIFIED | `make -C utils clean all` exits 0, no warnings in output. Both .o files produced. |
| 2 | util.h exports die, warn, pscanf, exec_wait, exec_detach, setup_sigchld, LEN macro, and argv0 | VERIFIED | util.h lines 7-16: LEN macro, extern argv0, all 6 function declarations present |
| 3 | dmenu.h exports DmenuCtx struct, dmenu_open, dmenu_write, dmenu_read, dmenu_close | VERIFIED | dmenu.h lines 8-26: typedef struct DmenuCtx with write/read/pid members, all 4 function declarations |
| 4 | util.c implements all six public functions using POSIX fork/execvp/waitpid/sigaction | VERIFIED | util.c 123 lines: verr (static helper), warn, die, pscanf, exec_wait (fork+execvp+waitpid), exec_detach (fork+setsid+execvp), setup_sigchld (sigaction with SA_RESTART+SA_NOCLDSTOP) |
| 5 | config.mk uses PREFIX = $(HOME)/.local and -std=c99 flags matching suckless convention | VERIFIED | config.mk line 5: `PREFIX = $(HOME)/.local`, line 8: `-std=c99 -pedantic -Wall -Wextra -Wno-unused-parameter -Os -D_DEFAULT_SOURCE` |
| 6 | dmenu.c implements the full two-step pipe API: dmenu_open, dmenu_write, dmenu_read, dmenu_close | VERIFIED | dmenu.c 141 lines: dmenu_open (fork+dup2+exec, argv construction, pipe creation), dmenu_write (fprintf+fflush), dmenu_read (fclose write end, fgets, strip newline, NULL on empty), dmenu_close (fclose both, waitpid, free) |
| 7 | dmenu.c uses fork+dup2+exec internally, never popen | VERIFIED | grep confirms: fork(), dup2(), execvp() present. `grep popen dmenu.c` returns 0 matches. `grep system dmenu.c` returns 0 matches (except system headers). |
| 8 | dmenu_read returns NULL when user cancels (presses Escape) | VERIFIED | dmenu.c lines 115-127: fgets returns NULL on EOF/cancel -> returns NULL. Empty string after strip -> returns NULL. test-dmenu confirms NULL returned in non-interactive mode. |
| 9 | install.sh has a new section that runs make -C utils and copies binaries to ~/.local/bin | VERIFIED | install.sh lines 45-51: `echo "==> Building C utilities"`, `make -C "$REPO_DIR/utils" clean all`, `make -C "$REPO_DIR/utils" install`. No sudo on these lines. Section is between suckless builds (line 44) and scripts section (line 55). |
| 10 | A test program can link against common/dmenu.o and common/util.o and compile | VERIFIED | `make -C utils test` exits 0. test-dmenu links against both .o files (Makefile line 48-49). 11/11 tests pass including runtime pipe integration. |
| 11 | All 6 utility subdirectories exist per D-05 | FAILED | Only utils/common/ and utils/tests/ exist. Missing: dmenu-session/, dmenu-cpupower/, dmenu-clip/, dmenu-clipd/, battery-notify/, screenshot-notify/. SUMMARY 01-01 claims "All 6 utility subdirectories created" -- this is false. |

**Score:** 9/11 truths verified (1 failed, 1 needs human verification beyond what automated checks cover)

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `utils/config.mk` | Shared compiler/linker configuration | VERIFIED | 12 lines. Contains PREFIX, CFLAGS with c99, CC=cc. |
| `utils/Makefile` | Top-level build system for all utilities | VERIFIED | 57 lines. `include config.mk`, COMMON_OBJ, DMENU_OBJ, explicit rules, install/uninstall/test targets. |
| `utils/common/util.h` | Public API declarations for shared utility library | VERIFIED | 18 lines. All 6 functions declared, LEN macro, extern argv0, include guard. |
| `utils/common/util.c` | Implementation of shared utility functions | VERIFIED | 123 lines (min_lines: 80 met). All functions implemented. No popen/system. Static verr helper. |
| `utils/common/dmenu.h` | Public API declarations for dmenu pipe helpers | VERIFIED | 28 lines. DmenuCtx struct, all 4 functions declared with doc comments. |
| `utils/common/dmenu.c` | dmenu bidirectional pipe implementation | VERIFIED | 141 lines (min_lines: 80 met). Full implementation with fork+dup2+exec, deadlock-safe read ordering. |
| `utils/tests/test-util.c` | Compilation and linkage test for util.c | VERIFIED | 253 lines. Tests all 10 behaviors (die exits, die errno, warn continues, warn errno, pscanf reads, pscanf nofile, exec_wait true/false, exec_detach nonblocking, sigchld reaps). |
| `utils/tests/test-dmenu.c` | Compilation and API test for dmenu helpers | VERIFIED | 166 lines. Struct member checks, symbol resolution, pipe integration. Contains dmenu_open. |
| `install.sh` | Updated installer with utils build and install | VERIFIED | Lines 45-51 contain make -C utils commands. No sudo on utils lines. Preserves existing suckless and scripts sections. |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| utils/Makefile | utils/config.mk | include directive | WIRED | Line 4: `include config.mk` |
| utils/Makefile | utils/common/util.o | COMMON_OBJ variable | WIRED | Line 7: `COMMON_OBJ = common/util.o`, line 16: explicit rule `common/util.o: common/util.c common/util.h` |
| utils/common/util.c | utils/common/util.h | include directive | WIRED | Line 11: `#include "util.h"` |
| utils/common/dmenu.c | utils/common/dmenu.h | include directive | WIRED | Line 8: `#include "dmenu.h"` |
| utils/common/dmenu.c | utils/common/util.h | include for die() on fork failure | WIRED | Line 9: `#include "util.h"`. die() called on fork/pipe/malloc failures. |
| install.sh | utils/Makefile | make -C utils command | WIRED | Line 48: `make -C "$REPO_DIR/utils" clean all`, Line 51: `make -C "$REPO_DIR/utils" install` |

### Data-Flow Trace (Level 4)

Not applicable -- Phase 1 produces library code and build infrastructure, not components that render dynamic data.

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| Build compiles without errors/warnings | `make -C utils clean all` | Exit 0, no warnings | PASS |
| test-util passes all 10 tests | `make -C utils test` | 10 passed, 0 failed | PASS |
| test-dmenu passes all 11 tests | `make -C utils test` | 11/11 passed, including runtime pipe integration | PASS |
| No popen/system in common code | `grep popen/system utils/common/*.c` | 0 matches | PASS |
| install.sh has utils section without sudo | `grep "make -C.*utils" install.sh` | 2 matches, neither contains sudo | PASS |
| Utility subdirectories exist | `ls -d utils/dmenu-session/` etc. | 6 of 6 missing | FAIL |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|------------|-------------|--------|----------|
| FOUND-01 | 01-01 | Shared utility library (die, warn, pscanf, exec_wait, exec_detach) modeled on slstatus/util.c | SATISFIED | util.h/util.c implement all functions. setup_sigchld also included per D-03. test-util validates every function. Fresh implementation per D-02 (not copied from slstatus). |
| FOUND-02 | 01-02 | Shared dmenu pipe helpers (dmenu_open, dmenu_select) using fork+dup2+exec pattern | SATISFIED | dmenu.h/dmenu.c implement full 4-function API (dmenu_open, dmenu_write, dmenu_read, dmenu_close). Note: requirement text says "dmenu_select" but user decision D-08 refined to dmenu_read as part of a 4-function handle-based API. RESEARCH.md FOUND-02 row documents this evolution. Behavioral intent (open pipe, write items, read selection) is fully met. |
| FOUND-03 | 01-01 | Per-utility Makefile with config.mk convention matching dwm/dmenu/slstatus | SATISFIED | User decision D-06 changed architecture to single top-level Makefile at utils/ (not per-utility). config.mk convention is implemented matching suckless style. RESEARCH.md FOUND-03 row documents: "D-06 changes this -- single top-level Makefile at utils/, not per-utility Makefiles. config.mk pattern still used but shared at utils/ level." |
| FOUND-04 | 01-02 | Updated install.sh compiles all C utilities and installs binaries to ~/.local/bin | SATISFIED | install.sh lines 45-51 call `make -C "$REPO_DIR/utils" clean all` and `make -C "$REPO_DIR/utils" install`. No sudo. Makefile install target creates ~/.local/bin and copies BINS. Currently BINS is empty (correct for Phase 1 -- no user-facing binaries). |

No orphaned requirements -- all 4 FOUND-* IDs mapped to Phase 1 in REQUIREMENTS.md traceability table, all appear in plan frontmatter (FOUND-01/03 in 01-01, FOUND-02/04 in 01-02).

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| utils/tests/test-dmenu.c | 93 | `system("command -v dmenu >/dev/null 2>&1")` | Info | Uses system() in test-only code for dmenu binary detection. Acceptable in test context, not in production library code. |
| utils/Makefile | 11 | `BINS =` (empty) | Info | Expected for Phase 1. Phase 2+ will populate. Not a stub -- this is intentional scaffolding. |

No blockers or warnings found. No TODO/FIXME/PLACEHOLDER markers in any production code. No popen/system in library code.

### Human Verification Required

### 1. End-to-end install.sh run

**Test:** Run `./install.sh` on a system with all suckless tools present
**Expected:** Script completes without errors. Suckless tools build (dwm, st, dmenu, slstatus), C utilities build (make -C utils clean all), install runs (make -C utils install), scripts copy to ~/.local/bin.
**Why human:** install.sh runs `sudo pacman` interactively and builds suckless tools from source. Requires real system with X11, build deps, and sudo access.

### 2. dmenu_read cancel behavior with Escape key

**Test:** Write a small C program that calls dmenu_open, dmenu_write several items, dmenu_read, and prints the result. Run it, press Escape in the dmenu window.
**Expected:** dmenu_read returns NULL. Program prints "(null)" or similar.
**Why human:** Requires interactive X11 display and manual Escape keypress. Automated test verifies NULL return in non-interactive mode but cannot simulate Escape key press.

### Gaps Summary

**1 gap found: missing utility subdirectories**

Plan 01 Task 1 explicitly listed `mkdir -p utils/common utils/dmenu-session utils/dmenu-cpupower utils/dmenu-clip utils/dmenu-clipd utils/battery-notify utils/screenshot-notify`. Summary 01-01 claims "All 6 utility subdirectories created under utils/" but only `utils/common/` and `utils/tests/` exist. The 6 utility-specific directories were never created.

**Impact assessment:** LOW. These are empty placeholder directories for later phases. Their absence does not block Phase 1's goal (shared library + build system + install.sh). Later phases will need to create them when adding utility source files. However, this is a clear discrepancy between SUMMARY claims and reality.

**Root cause:** The executor likely forgot the `mkdir -p` step or it was rolled back. The SUMMARY passed self-check without verifying directory existence.

---

*Verified: 2026-04-09T17:30:00Z*
*Verifier: Claude (gsd-verifier)*
