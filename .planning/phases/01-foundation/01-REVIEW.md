---
phase: 01-foundation
reviewed: 2026-04-09T00:00:00Z
depth: standard
files_reviewed: 9
files_reviewed_list:
  - install.sh
  - utils/common/dmenu.c
  - utils/common/dmenu.h
  - utils/common/util.c
  - utils/common/util.h
  - utils/config.mk
  - utils/Makefile
  - utils/tests/test-dmenu.c
  - utils/tests/test-util.c
findings:
  critical: 1
  warning: 3
  info: 2
  total: 6
status: issues_found
---

# Phase 01: Code Review Report

**Reviewed:** 2026-04-09T00:00:00Z
**Depth:** standard
**Files Reviewed:** 9
**Status:** issues_found

## Summary

Reviewed the foundational C utility library (dmenu abstraction layer and util helpers), build system, install script, and test suite. The code follows suckless conventions well -- clean, minimal, and POSIX-oriented. One critical issue was found: `fdopen` return values are not checked in `dmenu_open`, which would cause a NULL pointer dereference crash if the call fails. Three warnings address resource leaks on error paths and an unchecked `sigaction` return value. Two informational items note a magic number and dead code.

## Critical Issues

### CR-01: Unchecked fdopen return values in dmenu_open cause NULL dereference

**File:** `utils/common/dmenu.c:68-69`
**Issue:** Both `fdopen(to_dmenu[1], "w")` and `fdopen(from_dmenu[0], "r")` can return NULL on failure (e.g., if the file descriptor limit is hit). The return values are stored directly in `ctx->write` and `ctx->read` without any NULL check. Subsequent calls to `fprintf(ctx->write, ...)` in `dmenu_write` or `fgets(buf, 1024, ctx->read)` in `dmenu_read` would dereference a NULL `FILE *`, causing a segfault.
**Fix:**
```c
ctx->write = fdopen(to_dmenu[1], "w");
if (!ctx->write)
	die("fdopen:");
ctx->read = fdopen(from_dmenu[0], "r");
if (!ctx->read)
	die("fdopen:");
```

## Warnings

### WR-01: Pipe file descriptor leak when second pipe() call fails

**File:** `utils/common/dmenu.c:42-43`
**Issue:** If `pipe(from_dmenu)` fails on line 42, `die()` is called. At that point, `to_dmenu[0]` and `to_dmenu[1]` from the successful first `pipe()` call on line 40 are leaked. The `argv` allocation from line 29 is also leaked. Since `die()` exits the process, the OS will reclaim these resources, but this violates the project's own convention of guarding resources carefully. If `die()` is ever changed to a recoverable error path, this becomes a real leak.
**Fix:**
```c
if (pipe(from_dmenu) < 0) {
	close(to_dmenu[0]);
	close(to_dmenu[1]);
	free(argv);
	die("pipe:");
}
```

### WR-02: Unchecked sigaction return value

**File:** `utils/common/util.c:121`
**Issue:** `sigaction(SIGCHLD, &sa, NULL)` can fail (returns -1) if the signal number is invalid or if there is a system error. The return value is silently ignored. If this call fails, `exec_detach()` will produce zombie processes because the SIGCHLD handler was never installed. The caller (`setup_sigchld`) has no way to know the setup failed.
**Fix:**
```c
if (sigaction(SIGCHLD, &sa, NULL) < 0)
	die("sigaction:");
```

### WR-03: Missing SIGALRM handler in exec_detach test causes silent test abort

**File:** `utils/tests/test-util.c:194`
**Issue:** The `test_exec_detach_nonblocking` test uses `alarm(2)` as a timeout mechanism, but no SIGALRM handler is installed. If `exec_detach` blocks and the alarm fires, the default SIGALRM action (process termination) kills the entire test suite silently with no diagnostic output. This affects test reliability -- a blocked `exec_detach` would cause total test failure with no indication of which test failed or why.
**Fix:** Install a handler that sets a flag, or use the existing `fail()` mechanism:
```c
static volatile sig_atomic_t alarm_fired = 0;
static void alarm_handler(int sig) { (void)sig; alarm_fired = 1; }

/* In test_exec_detach_nonblocking, before alarm(2): */
signal(SIGALRM, alarm_handler);
alarm(2);
waitpid(pid, &status, 0);
alarm(0);

if (alarm_fired) {
	fail("exec_detach_nonblocking", "timed out -- exec_detach blocked");
	return;
}
```

## Info

### IN-01: Magic number 1024 for dmenu read buffer

**File:** `utils/common/dmenu.c:94,98`
**Issue:** The buffer size 1024 is used as a raw literal in two places: `malloc(1024)` and `fgets(buf, 1024, ctx->read)`. If one is changed without the other, a buffer overflow or truncation bug is introduced.
**Fix:** Define a constant at the top of the file:
```c
#define DMENU_BUFSZ 1024
```
Then use `DMENU_BUFSZ` in both locations.

### IN-02: Dead code in test_setup_sigchld_reaps

**File:** `utils/tests/test-util.c:230-231`
**Issue:** `ret = 0; (void)ret;` appears to be leftover code that serves no purpose. The variable `ret` is assigned and immediately cast to void.
**Fix:** Remove the two lines:
```c
	/* Restore default SIGCHLD for remaining tests */
	signal(SIGCHLD, SIG_DFL);
```

---

_Reviewed: 2026-04-09T00:00:00Z_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
