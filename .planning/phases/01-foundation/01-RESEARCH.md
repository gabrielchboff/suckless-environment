# Phase 1: Foundation - Research

**Researched:** 2026-04-09
**Domain:** C shared library, POSIX make build system, fork/exec process management, dmenu pipe IPC
**Confidence:** HIGH

## Summary

Phase 1 establishes the shared C utility library (`utils/common/`) and build system that all subsequent phases compile against. The core deliverables are: (1) `common/util.c` with die(), warn(), pscanf(), exec_wait(), exec_detach(), and a SIGCHLD zombie reaper setup function; (2) `common/dmenu.c` with a two-step dmenu pipe API using fork+dup2+exec; (3) a single top-level Makefile at `utils/` with a shared `config.mk`; and (4) install.sh extended to compile and install all utilities.

This phase is purely foundational -- no user-facing binaries are produced. The patterns established here (error handling, process spawning, dmenu interaction, build conventions) will be replicated by every subsequent utility. Getting these right prevents cascading rework in later phases.

**Primary recommendation:** Build the shared library code and Makefile first, then validate with minimal test programs that exercise each function. The entire phase is standard POSIX C with no external dependencies beyond libc. Follow the suckless conventions already established in slstatus/util.c and dwm/Makefile but respect the locked decision that `utils/` has a single top-level Makefile (D-06), not per-directory Makefiles.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** NEVER modify suckless source directories (dwm/, dmenu/, st/, slstatus/). These are upstream-patched and must remain untouched.
- **D-02:** common/util.c is a fresh standalone implementation inspired by suckless patterns. Same function names (die, warn, pscanf) but independent code. Not a copy-paste from slstatus.
- **D-03:** common/util.c provides: die(), warn(), pscanf(), exec_wait() (fork+execvp+waitpid for commands where exit status matters), exec_detach() (fork+execvp for fire-and-forget spawning), and a SIGCHLD zombie reaper setup function.
- **D-04:** All new C utilities live under `utils/` at repo root -- sibling to dwm/, dmenu/, etc.
- **D-05:** Directory structure: `utils/common/`, `utils/dmenu-session/`, `utils/dmenu-cpupower/`, `utils/dmenu-clip/`, `utils/dmenu-clipd/`, `utils/battery-notify/`, `utils/screenshot-notify/`
- **D-06:** A single top-level Makefile at `utils/` builds all utilities in one `make` command. Individual utility directories contain their .c source files but no standalone Makefiles.
- **D-07:** install.sh calls `make -C utils` to build, then installs binaries to ~/.local/bin.
- **D-08:** Two-step API design: dmenu_open(prompt, argv) returns a handle, dmenu_write()/dmenu_read() operate on it, dmenu_close() cleans up. This gives callers control over when to write items and read selection.
- **D-09:** Uses fork+dup2+exec internally (never popen) to avoid bidirectional pipe deadlock.
- **D-10:** dmenu_read() returns NULL on cancel (user pressed Escape) or empty selection.

### Claude's Discretion
- Zombie reaping strategy (SIGCHLD handler vs per-call waitpid) -- Claude picks per utility
- config.mk variable names and flag organization
- Header file organization within common/

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| FOUND-01 | Shared utility library (die, warn, pscanf, exec_wait, exec_detach) modeled on slstatus/util.c | Verified slstatus/util.c pattern (die/warn/verr/pscanf). Fresh implementation per D-02. exec_wait and exec_detach are new additions using standard POSIX fork/execvp/waitpid. SIGCHLD reaper setup function also required per D-03. See Architecture Patterns section. |
| FOUND-02 | Shared dmenu pipe helpers (dmenu_open, dmenu_select) using fork+dup2+exec pattern | Verified dmenu pipe pattern from ARCHITECTURE.md research. Two-step API per D-08: handle-based with dmenu_open/dmenu_write/dmenu_read/dmenu_close. fork+dup2+exec per D-09. See dmenu Pipe Pattern section. |
| FOUND-03 | Per-utility Makefile with config.mk convention matching dwm/dmenu/slstatus | NOTE: D-06 changes this -- single top-level Makefile at utils/, not per-utility Makefiles. config.mk pattern still used but shared at utils/ level. Verified dwm/Makefile and slstatus/Makefile patterns for reference. See Build System section. |
| FOUND-04 | Updated install.sh compiles all C utilities and installs binaries to ~/.local/bin | install.sh currently builds suckless tools to /usr/local/bin with sudo. Per D-07, new utils install to ~/.local/bin (no sudo needed). install.sh calls `make -C utils` then copies binaries. See install.sh Integration section. |
</phase_requirements>

## Project Constraints (from CLAUDE.md)

- **Platform:** Arch Linux only -- pacman/AUR package names
- **Philosophy:** Suckless-style C -- minimal dependencies, no frameworks
- **Display server:** X11 -- all utilities use Xlib/XFixes directly where needed
- **Language standard:** C99 (`-std=c99`)
- **Code style:** K&R braces, tab indentation, 80-col target, no trailing whitespace
- **Include order:** stdlib -> POSIX -> X11 -> local headers
- **Error handling:** die() for fatal, warn() for non-fatal, NULL return on error
- **Build pattern:** Makefile + config.mk
- **Comment header:** `/* See LICENSE file for copyright and license details. */` in every file
- **No suckless modifications:** dwm/, dmenu/, st/, slstatus/ are read-only references
- **GSD workflow:** Do not make direct repo edits outside a GSD workflow unless explicitly asked

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| GCC | 15.2.1 | C compiler | Already installed, Arch default [VERIFIED: `cc --version` on this system] |
| GNU Make | 4.4.1 | Build system | Already installed, suckless convention [VERIFIED: `make --version` on this system] |
| libc (glibc) | system | POSIX functions (fork, execvp, waitpid, pipe, dup2, signal) | No external dep -- standard POSIX C [VERIFIED: system libc] |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| pkg-config | 2.5.1 | Compiler/linker flag discovery | When needed for future phases linking X11/systemd [VERIFIED: `pkg-config --version` on this system] |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| GNU Make | CMake/Meson | Alien to suckless ecosystem, over-engineered for single-file utilities |
| GCC | clang | Either works, but GCC is already installed and matches existing builds |

**No installation needed.** Phase 1 uses only libc. No `-l` flags required for the foundation library itself.

## Architecture Patterns

### Recommended Project Structure

```
utils/
├── config.mk              # Shared compiler/linker config (suckless convention)
├── Makefile               # Single top-level Makefile builds everything
├── common/
│   ├── util.h             # die(), warn(), pscanf(), exec_wait(), exec_detach(), setup_sigchld()
│   ├── util.c             # Implementation
│   ├── dmenu.h            # DmenuCtx handle, dmenu_open(), dmenu_write(), dmenu_read(), dmenu_close()
│   └── dmenu.c            # Implementation using fork+dup2+exec
├── dmenu-session/
│   └── dmenu-session.c    # (future phase)
├── dmenu-cpupower/
│   └── dmenu-cpupower.c   # (future phase)
├── dmenu-clip/
│   └── dmenu-clip.c       # (future phase)
├── dmenu-clipd/
│   └── dmenu-clipd.c      # (future phase)
├── battery-notify/
│   └── battery-notify.c   # (future phase)
└── screenshot-notify/
    └── screenshot-notify.c # (future phase)
```

### Pattern 1: Error Handling (die/warn/verr)

**What:** Variadic error functions that print to stderr with optional errno context. The slstatus pattern uses a format string ending in `:` to trigger perror() output.

**When to use:** Every utility. die() for fatal errors (exits 1), warn() for non-fatal (continues).

**Reference pattern (slstatus/util.c -- DO NOT COPY, rewrite independently per D-02):**
```c
/* Source: slstatus/util.c lines 13-46, read-only reference */
static void
verr(const char *fmt, va_list ap)
{
    vfprintf(stderr, fmt, ap);
    if (fmt[0] && fmt[strlen(fmt) - 1] == ':') {
        fputc(' ', stderr);
        perror(NULL);
    } else {
        fputc('\n', stderr);
    }
}

void warn(const char *fmt, ...) { /* va_start, verr, va_end */ }
void die(const char *fmt, ...)  { /* va_start, verr, va_end, exit(1) */ }
```

**Implementation notes for the fresh rewrite:**
- Same API contract: `die(fmt, ...)` and `warn(fmt, ...)` with `:` suffix triggering errno output
- Include program name in output (use global `argv0` or pass at init)
- Fresh implementation means rewriting the verr logic independently -- same behavior, different code [VERIFIED: D-02 explicitly states "independent code"]

### Pattern 2: pscanf (sysfs Reader)

**What:** Open a file, scanf from it, close immediately. Used for reading sysfs pseudo-files.

**When to use:** battery-notify (reads /sys/class/power_supply/BAT1/capacity and status).

**Reference (slstatus/util.c lines 125-141):**
```c
int
pscanf(const char *path, const char *fmt, ...)
{
    FILE *fp;
    va_list ap;
    int n;

    if (!(fp = fopen(path, "r"))) {
        warn("fopen '%s':", path);
        return -1;
    }
    va_start(ap, fmt);
    n = vfscanf(fp, fmt, ap);
    va_end(ap);
    fclose(fp);

    return (n == EOF) ? -1 : n;
}
```

**Critical detail:** Always open-read-close per call. Never keep sysfs file descriptors open -- sysfs is virtual and subsequent reads without reopening return stale data. [VERIFIED: PITFALLS.md Pitfall 6]

### Pattern 3: exec_wait and exec_detach

**What:** Two process spawning helpers. exec_wait forks, execs, and waits for exit status. exec_detach forks, execs, and returns immediately (fire-and-forget).

**When to use:** exec_wait for commands where exit status matters (powerprofilesctl, maim). exec_detach for commands that should outlive the caller (betterlockscreen, notify-send).

```c
/* Source: ARCHITECTURE.md research, POSIX standard */

/* Fork, exec, wait. Returns child exit status or -1 on signal death. */
static int
exec_wait(const char *const argv[])
{
    pid_t pid;
    int status;

    pid = fork();
    if (pid < 0)
        die("fork:");
    if (pid == 0) {
        execvp(argv[0], (char *const *)argv);
        _exit(127);
    }
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

/* Fork, exec, do not wait. Child is detached. */
static void
exec_detach(const char *const argv[])
{
    pid_t pid;

    pid = fork();
    if (pid < 0)
        die("fork:");
    if (pid == 0) {
        setsid();
        execvp(argv[0], (char *const *)argv);
        _exit(127);
    }
    /* Parent returns immediately. SIGCHLD handler reaps zombie. */
}
```

**Key detail:** exec_detach calls `setsid()` in the child to detach from the controlling terminal. This prevents the child from receiving signals intended for the parent. The parent does NOT call waitpid -- zombie reaping is handled by the SIGCHLD handler. [VERIFIED: POSIX standard, ARCHITECTURE.md pattern]

### Pattern 4: SIGCHLD Zombie Reaper

**What:** A signal handler that reaps all terminated children in a non-blocking loop. A setup function installs it.

**When to use:** Any utility that calls exec_detach(). Install once at program start.

```c
/* Source: POSIX signal handling, PITFALLS.md Pitfall 2 */
#include <signal.h>
#include <sys/wait.h>

static void
sigchld_handler(int sig)
{
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

void
setup_sigchld(void)
{
    struct sigaction sa;

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);
}
```

**Async-signal-safety:** The handler ONLY calls waitpid() which is async-signal-safe per POSIX. No printf, no malloc, no other functions. [VERIFIED: signal-safety(7) man page, PITFALLS.md Pitfall 5]

### Pattern 5: dmenu Pipe API (Two-Step Handle)

**What:** A handle-based API for bidirectional communication with dmenu: open a pipe, write menu items, read selection, close/cleanup. Uses fork+dup2+exec internally per D-09.

**When to use:** dmenu-session, dmenu-cpupower, dmenu-clip.

**API design per D-08:**
```c
/* Source: CONTEXT.md D-08, D-09, D-10, ARCHITECTURE.md pipe pattern */

typedef struct {
    FILE *write;    /* Parent writes menu items here */
    FILE *read;     /* Parent reads selection from here */
    pid_t pid;      /* dmenu child PID */
} DmenuCtx;

/* Open dmenu with given prompt. Extra args forwarded to dmenu.
 * Returns handle for write/read operations. */
DmenuCtx *dmenu_open(const char *prompt, char *const extra_argv[]);

/* Write a single menu item (appends newline). */
void dmenu_write(DmenuCtx *ctx, const char *item);

/* Close write end (signals EOF to dmenu), read user selection.
 * Returns allocated string (caller frees) or NULL on cancel/Escape. */
char *dmenu_read(DmenuCtx *ctx);

/* Wait for dmenu to exit, free resources. */
void dmenu_close(DmenuCtx *ctx);
```

**Internal implementation notes:**
1. dmenu_open: create two pipes (to_dmenu, from_dmenu), fork, child does dup2+exec
2. dmenu_write: fprintf + fflush to the write pipe
3. dmenu_read: fclose write end (EOF to dmenu), fgets from read end, strip trailing newline
4. dmenu_close: waitpid on child, fclose read end, free context
5. dmenu_read returns NULL on cancel per D-10 (fgets returns NULL when dmenu exits without selection)

**argv construction:** The prompt is passed as `-p prompt`. Extra argv from the caller (dmenu styling args from dwm's config.h) are appended. The function builds the full argv array: `{"dmenu", "-p", prompt, extra_argv[0], extra_argv[1], ..., NULL}`.

### Pattern 6: Single Top-Level Makefile (D-06)

**What:** One Makefile at `utils/` that includes `config.mk` and builds all utilities. Each utility directory has only `.c` source files.

**Reference patterns from suckless (dwm/Makefile, slstatus/Makefile):**
```makefile
# Source: Verified from dwm/Makefile and slstatus/Makefile in this repo

# utils/config.mk
VERSION = 1.0
CC = cc
CFLAGS = -std=c99 -pedantic -Wall -Wextra -Os -D_DEFAULT_SOURCE
LDFLAGS = -s
PREFIX = $(HOME)/.local

# utils/Makefile
include config.mk

# Common objects
COMMON_OBJ = common/util.o common/dmenu.o

# Utility binaries and their source objects
UTILS = dmenu-session dmenu-cpupower dmenu-clip dmenu-clipd \
        battery-notify screenshot-notify

# Per-utility objects (each utility has one .c file)
# dmenu-session needs common/util.o + common/dmenu.o
# battery-notify needs only common/util.o
# dmenu-clipd needs common/util.o + X11 libs

all: $(UTILS)

# Implicit rule for .c -> .o
.c.o:
	$(CC) -o $@ -c $(CFLAGS) $<

# Common library objects
common/util.o: common/util.c common/util.h
common/dmenu.o: common/dmenu.c common/dmenu.h common/util.h

# Per-utility link rules (each specifies its own objects and libs)
dmenu-session: dmenu-session/dmenu-session.o $(COMMON_OBJ)
	$(CC) -o $@ $(LDFLAGS) $^

# ... etc for each utility

clean:
	rm -f $(COMMON_OBJ) ...

install: all
	mkdir -p "$(DESTDIR)$(PREFIX)/bin"
	# cp each binary to $(PREFIX)/bin
```

**Key difference from suckless originals:** suckless tools install to /usr/local/bin with sudo. Per D-07, new utils install to ~/.local/bin without sudo. The PREFIX in config.mk reflects this.

### Anti-Patterns to Avoid

- **Using system() for anything:** Spawns /bin/sh, shell injection risk, violates the entire project goal of eliminating shell dependencies. Use exec_wait/exec_detach. [VERIFIED: PITFALLS.md, CERT C ENV33-C]
- **Using popen() for dmenu:** Only unidirectional -- cannot write stdin AND read stdout. Use fork+dup2+exec per D-09. [VERIFIED: PITFALLS.md Pitfall 3]
- **Copying slstatus/util.c verbatim:** Violates D-02. Same API, fresh implementation.
- **Per-utility Makefiles:** Violates D-06. Single top-level Makefile only.
- **Building a shared library (.so):** Overkill for 5 functions. Compile to .o, link statically into each binary. Suckless convention. [VERIFIED: ARCHITECTURE.md]
- **Non-async-signal-safe functions in signal handlers:** Only waitpid() and _exit() are safe. No printf, no malloc, no syslog. [VERIFIED: signal-safety(7), PITFALLS.md Pitfall 5]

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Error formatting with errno | Custom fprintf+strerror wrapper | verr() pattern from suckless (`:` suffix convention) | Consistent with ecosystem, handles errno formatting edge cases |
| Process spawning | Ad-hoc fork/exec per call site | exec_wait() and exec_detach() in common/util.c | DRY, correct waitpid/setsid handling, prevents zombie leaks |
| dmenu bidirectional pipe | Raw pipe/fork/dup2 in each utility | DmenuCtx handle API in common/dmenu.c | Pipe lifecycle and cleanup is error-prone, centralize once |
| Zombie reaping | Per-process waitpid or SIG_IGN | setup_sigchld() with SA_RESTART handler | Correct async-signal-safe pattern, covers all fire-and-forget children |

**Key insight:** The entire point of Phase 1 is building the "don't hand-roll" layer. Every subsequent phase calls these functions instead of reimplementing fork/exec/pipe logic.

## Common Pitfalls

### Pitfall 1: Bidirectional Pipe Deadlock with dmenu

**What goes wrong:** Parent writes to dmenu stdin and reads from dmenu stdout through two pipes. If the parent writes more data than the pipe buffer can hold (~64KB on Linux) without reading, both processes deadlock -- parent blocked on write, dmenu blocked on write to full stdout pipe.
**Why it happens:** The kernel pipe buffer is finite. popen() only handles one direction. Manual two-pipe management requires careful sequencing.
**How to avoid:** Write all items to the dmenu write pipe, close the write end (signals EOF to dmenu), THEN read the selection from the read pipe. For Phase 1 utilities, menu data is always small (<1KB), so the write completes before the buffer fills. The dmenu_read() function in common/dmenu.c must close the write end before attempting to read.
**Warning signs:** dmenu never appears, utility hangs on launch, strace shows blocked write().
[VERIFIED: PITFALLS.md Pitfall 3]

### Pitfall 2: Zombie Processes from exec_detach

**What goes wrong:** exec_detach forks a child and returns without waiting. The child eventually exits but the parent never calls waitpid(). The child becomes a zombie (Z state in ps). Over time, zombies accumulate and waste PID table entries.
**Why it happens:** C programs must explicitly reap children, unlike shell scripts where the shell does it automatically.
**How to avoid:** Install a SIGCHLD handler at program start using setup_sigchld(). The handler calls waitpid(-1, NULL, WNOHANG) in a loop to reap all terminated children.
**Warning signs:** `ps aux | grep Z` shows defunct processes after running utilities.
[VERIFIED: PITFALLS.md Pitfall 2]

### Pitfall 3: Stale sysfs Reads

**What goes wrong:** Keeping a file descriptor to /sys/class/power_supply/BAT1/capacity open and reading multiple times returns stale data because the file position is at EOF after the first read.
**Why it happens:** Sysfs is virtual -- content is generated on read. After consuming it, subsequent reads without seek or reopen get nothing.
**How to avoid:** pscanf() opens, reads, and closes in one call. Never cache sysfs file descriptors.
**Warning signs:** Battery percentage always reads the same value regardless of actual charge.
[VERIFIED: PITFALLS.md Pitfall 6, slstatus/util.c pscanf pattern]

### Pitfall 4: Signal Handler Safety Violations

**What goes wrong:** A SIGCHLD handler that calls printf(), malloc(), or any non-async-signal-safe function can corrupt memory or deadlock. The handler interrupts the main program at an arbitrary point -- if the main program is inside malloc() when the signal fires, calling malloc() in the handler corrupts the heap.
**Why it happens:** Programmers add logging to signal handlers for debugging, not realizing the reentrancy constraint.
**How to avoid:** The SIGCHLD handler must ONLY call waitpid() (async-signal-safe). Use `volatile sig_atomic_t` flags for other signal-driven state changes. The setup_sigchld() function establishes the safe pattern once.
**Warning signs:** Intermittent crashes, deadlocks under load, ASan reports in signal context.
[VERIFIED: signal-safety(7) Linux man page, PITFALLS.md Pitfall 5]

### Pitfall 5: Makefile Implicit Rule Conflicts with Subdirectory Sources

**What goes wrong:** A single top-level Makefile with source files in subdirectories (common/util.c, dmenu-session/dmenu-session.c) does not work with the default `.c.o` implicit rule because Make looks for the .c file relative to the Makefile location but the .o target includes the subdirectory path.
**Why it happens:** POSIX Make's implicit rules match based on file stem. `common/util.o` requires `common/util.c` but the default `.c.o` rule expects the source in the same directory as the target.
**How to avoid:** Write explicit dependency and compilation rules for each subdirectory. Or use a VPATH/vpath directive. Or use pattern rules: `common/%.o: common/%.c`. GNU Make supports this; POSIX Make does not. Since this is Arch Linux with GNU Make 4.4.1, GNU extensions are safe.
**Warning signs:** `make: *** No rule to make target 'common/util.o'` errors.
[ASSUMED -- standard GNU Make behavior, not verified against specific GNU Make docs in this session]

## Code Examples

### Example 1: Complete util.h Header

```c
/* Source: Synthesized from D-03, slstatus/util.h reference, suckless conventions */
/* See LICENSE file for copyright and license details. */
#ifndef UTIL_H
#define UTIL_H

#include <stdarg.h>

#define LEN(x) (sizeof(x) / sizeof((x)[0]))

extern char *argv0;

void die(const char *fmt, ...);
void warn(const char *fmt, ...);
int pscanf(const char *path, const char *fmt, ...);
int exec_wait(const char *const argv[]);
void exec_detach(const char *const argv[]);
void setup_sigchld(void);

#endif /* UTIL_H */
```

### Example 2: Complete dmenu.h Header

```c
/* Source: Synthesized from D-08, D-09, D-10, ARCHITECTURE.md */
/* See LICENSE file for copyright and license details. */
#ifndef DMENU_H
#define DMENU_H

#include <stdio.h>
#include <sys/types.h>

typedef struct {
	FILE *write;   /* parent writes menu items here */
	FILE *read;    /* parent reads selection from here */
	pid_t pid;     /* dmenu child PID */
} DmenuCtx;

DmenuCtx *dmenu_open(const char *prompt, char *const extra_argv[]);
void dmenu_write(DmenuCtx *ctx, const char *item);
char *dmenu_read(DmenuCtx *ctx);
void dmenu_close(DmenuCtx *ctx);

#endif /* DMENU_H */
```

### Example 3: config.mk Reference

```makefile
# Source: Verified from dwm/config.mk and slstatus/config.mk in this repo
# utils/config.mk -- shared build configuration for all utilities

VERSION = 1.0

# paths
PREFIX = $(HOME)/.local

# flags
CFLAGS  = -std=c99 -pedantic -Wall -Wextra -Wno-unused-parameter -Os -D_DEFAULT_SOURCE
LDFLAGS = -s

# compiler
CC = cc
```

### Example 4: Top-Level Makefile Skeleton

```makefile
# Source: Adapted from slstatus/Makefile pattern, adjusted for D-06
# utils/Makefile -- builds all utilities

include config.mk

# Common objects (linked into utility binaries, not standalone)
COMMON_OBJ = common/util.o
DMENU_OBJ  = common/dmenu.o

# Utility binaries -- populated as phases add utilities
# Phase 2+: BINS += battery-notify/battery-notify screenshot-notify/screenshot-notify
# Phase 3+: BINS += dmenu-session/dmenu-session dmenu-cpupower/dmenu-cpupower dmenu-clip/dmenu-clip
# Phase 4+: BINS += dmenu-clipd/dmenu-clipd
BINS =

all: $(COMMON_OBJ) $(DMENU_OBJ) $(BINS)

# Compilation rules for subdirectories (GNU Make pattern rules)
common/%.o: common/%.c common/%.h
	$(CC) -o $@ -c $(CFLAGS) $<

# Generic pattern for utility .c -> .o
%/%.o: %/%.c
	$(CC) -o $@ -c $(CFLAGS) -Icommon $<

clean:
	rm -f $(COMMON_OBJ) $(DMENU_OBJ)
	rm -f $(BINS) $(BINS:=.o)

install: all
	mkdir -p "$(DESTDIR)$(PREFIX)/bin"
	@for bin in $(BINS); do \
		cp -f "$$bin" "$(DESTDIR)$(PREFIX)/bin/$$(basename $$bin)"; \
		chmod 755 "$(DESTDIR)$(PREFIX)/bin/$$(basename $$bin)"; \
	done

uninstall:
	@for bin in $(BINS); do \
		rm -f "$(DESTDIR)$(PREFIX)/bin/$$(basename $$bin)"; \
	done

.PHONY: all clean install uninstall
```

**Note on BINS:** Phase 1 builds only common objects. BINS is empty until Phase 2 adds the first utility. The Makefile structure supports incremental addition without restructuring. Test programs can be added temporarily to validate the library.

### Example 5: install.sh Extension

```sh
# Source: Existing install.sh in repo, extended per D-07
# Add after existing suckless tool builds:

echo "==> Building C utilities"
make -C "$REPO_DIR/utils" clean all

echo "==> Installing C utilities"
make -C "$REPO_DIR/utils" install
```

**Key difference from suckless section:** No `sudo`. New utilities install to ~/.local/bin (PREFIX = $(HOME)/.local in config.mk), not /usr/local/bin. The existing suckless tools still install with sudo to /usr/local/bin -- these are separate concerns.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Shell scripts for desktop utilities | Native C programs | This project (v2) | Eliminates shell interpreter runtime dependency |
| popen() for bidirectional pipe | fork+dup2+exec pattern | Always was correct | Prevents deadlock on bidirectional IPC |
| system() for spawning | fork+execvp | CERT C ENV33-C (2006+) | Eliminates shell injection risk |
| Polling clipboard with xclip | XFixes event-driven (future phase) | XFixes extension available since X11R6.8 (2004) | Zero CPU when idle |

**Deprecated/outdated:**
- `signal()` function: Use `sigaction()` instead. signal() has unspecified behavior for handler reinstallation across systems. sigaction() is POSIX-portable and explicitly controllable. [VERIFIED: POSIX.1-2008 recommendation]

## Assumptions Log

> List all claims tagged `[ASSUMED]` in this research.

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | GNU Make pattern rules `common/%.o: common/%.c` work correctly for subdirectory compilation in this Makefile structure | Pitfall 5, Code Example 4 | LOW -- may need explicit rules per target instead of pattern rules. Easily tested and fixed during implementation. |

**All other claims verified** against the existing codebase (slstatus/util.c, dwm/Makefile, install.sh), POSIX standards, or the prior research documents (STACK.md, ARCHITECTURE.md, PITFALLS.md).

## Open Questions

1. **Header include path strategy**
   - What we know: Utilities include common headers via relative paths (e.g., `#include "../common/util.h"` from dmenu-session/). The single top-level Makefile could also use `-Icommon` in CFLAGS.
   - What's unclear: Whether to use relative includes or `-I` flag for common headers.
   - Recommendation: Use `-Icommon` in CFLAGS so source files use `#include "util.h"` and `#include "dmenu.h"` without path prefixes. This is cleaner and matches the suckless pattern where util.h is included as `#include "util.h"`. If the planner prefers relative paths, that works too -- it is within Claude's discretion per "Header file organization within common/".

2. **BINS list management across phases**
   - What we know: Phase 1 produces no binaries (common objects only). Phase 2+ adds binaries to the BINS variable.
   - What's unclear: Whether to define all BINS now (they will fail to build since sources do not exist) or add them per phase.
   - Recommendation: Start with BINS empty. Each subsequent phase adds its entries. The Makefile only needs the common object rules to be correct in Phase 1. A test program can validate the library without being in BINS.

3. **dmenu_read() return type**
   - What we know: D-10 says returns NULL on cancel. D-08 says dmenu_read() operates on a handle.
   - What's unclear: Whether dmenu_read() returns a heap-allocated string (caller frees) or writes into a caller-provided buffer.
   - Recommendation: Return a heap-allocated string (malloc + fgets, return pointer). Caller calls free(). This is the simplest API -- the selection length is unpredictable and callers should not have to guess buffer sizes. The suckless tools do use static buffers, but for a selection string that the caller acts on once and discards, malloc is cleaner.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| GCC (cc) | C compilation | Yes | 15.2.1 | clang (not needed) |
| GNU Make | Build system | Yes | 4.4.1 | -- |
| pkg-config | Flag discovery | Yes | 2.5.1 | Manual flags (not needed for Phase 1) |
| libc headers | POSIX functions | Yes | system | -- |
| dmenu binary | Runtime test of dmenu.c | Yes | built from source | -- |

**Missing dependencies with no fallback:** None.

**Missing dependencies with fallback:** None.

Phase 1 has zero external dependencies beyond the C toolchain and POSIX libc, all of which are already verified present.

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Manual C test programs (no test framework -- suckless convention) |
| Config file | None -- test programs are standalone .c files |
| Quick run command | `make -C utils && cc -o /tmp/test-util tests/test-util.c utils/common/util.o && /tmp/test-util` |
| Full suite command | `make -C utils && sh tests/run-all.sh` |

**Rationale:** Suckless projects do not use test frameworks (no Unity, no CUnit, no cmocka). Tests are standalone C programs that call the functions and assert expected behavior with simple `if` checks. A shell script orchestrates running them and checking exit codes.

### Phase Requirements -> Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| FOUND-01 | die/warn/pscanf/exec_wait/exec_detach compile and work | unit | `cc -o /tmp/test-util test-util.c common/util.o && /tmp/test-util` | No -- Wave 0 |
| FOUND-02 | dmenu_open/dmenu_write/dmenu_read/dmenu_close compile and work | integration | `cc -o /tmp/test-dmenu test-dmenu.c common/util.o common/dmenu.o && /tmp/test-dmenu` | No -- Wave 0 |
| FOUND-03 | `make` in utils/ produces common objects using config.mk convention | smoke | `make -C utils clean all && test -f utils/common/util.o && test -f utils/common/dmenu.o` | No -- Wave 0 |
| FOUND-04 | install.sh compiles all utilities and installs binaries to ~/.local/bin | smoke | `./install.sh` (manual verification that utils section runs) | No -- Wave 0 |

### Sampling Rate

- **Per task commit:** `make -C utils clean all` (quick build test)
- **Per wave merge:** Full test program compilation and execution
- **Phase gate:** All 4 success criteria pass before verification

### Wave 0 Gaps

- [ ] `utils/tests/test-util.c` -- test program for FOUND-01: calls die(), warn(), pscanf(), exec_wait(), exec_detach()
- [ ] `utils/tests/test-dmenu.c` -- test program for FOUND-02: opens dmenu pipe, writes items, reads selection
- [ ] `utils/tests/run-all.sh` -- shell script to compile and run all tests, report pass/fail
- [ ] Build system itself (Makefile + config.mk) must be created before tests can run

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | No | N/A -- no auth in foundation library |
| V3 Session Management | No | N/A -- no sessions |
| V4 Access Control | No | N/A -- no access control |
| V5 Input Validation | Yes | pscanf validates format strings; dmenu_read validates return value |
| V6 Cryptography | No | N/A -- no crypto in foundation |

### Known Threat Patterns for C/POSIX

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Shell injection via system() | Tampering | Never use system(). exec_wait/exec_detach use execvp with explicit argv arrays -- no shell interpretation. |
| Buffer overflow in format strings | Tampering | Use vsnprintf with explicit size. pscanf uses vfscanf with format bounds. |
| Signal handler reentrancy | Denial of Service | Only async-signal-safe functions in handlers (waitpid only). |
| Zombie process accumulation | Denial of Service | setup_sigchld() with WNOHANG reaping loop. |

## Sources

### Primary (HIGH confidence)
- slstatus/util.c, slstatus/util.h -- die/warn/pscanf pattern reference (direct codebase observation)
- slstatus/Makefile, slstatus/config.mk -- suckless build convention reference (direct codebase observation)
- dwm/Makefile, dwm/config.mk -- suckless build convention reference (direct codebase observation)
- dmenu/Makefile, dmenu/config.mk -- suckless build convention reference (direct codebase observation)
- install.sh -- current installer, integration point (direct codebase observation)
- dwm-start -- utility launch context (direct codebase observation)
- .planning/research/ARCHITECTURE.md -- dmenu pipe pattern, project structure, shared code analysis
- .planning/research/PITFALLS.md -- zombie processes, pipe deadlock, signal safety, sysfs reads
- .planning/research/STACK.md -- library dependencies and build flags
- POSIX.1-2008 -- fork, execvp, waitpid, pipe, dup2, sigaction specification

### Secondary (MEDIUM confidence)
- GCC 15.2.1, GNU Make 4.4.1, pkg-config 2.5.1 -- verified on this system via command-line probes
- /usr/include/X11/Xlib.h, /usr/include/X11/extensions/Xfixes.h, /usr/include/systemd/sd-bus.h -- verified present on this system

### Tertiary (LOW confidence)
- GNU Make pattern rule behavior for subdirectory targets -- based on training knowledge, not verified against current docs

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- verified GCC, Make, and libc are present and functional on this system
- Architecture: HIGH -- all patterns derived from existing suckless code in the repo and verified research documents
- Pitfalls: HIGH -- all pitfalls verified against PITFALLS.md research, POSIX standards, and codebase observation
- Build system: HIGH for config.mk/Makefile convention, MEDIUM for GNU Make pattern rules with subdirectories (A1)

**Research date:** 2026-04-09
**Valid until:** 2026-05-09 (stable domain -- POSIX C and Make do not change)
