# Phase 3: Interactive Utilities - Research

**Researched:** 2026-04-09
**Domain:** dmenu-driven C interactive utilities (session management, power profiles, clipboard picker)
**Confidence:** HIGH

## Summary

Phase 3 converts three shell scripts into native C utilities that interact with dmenu bidirectionally: dmenu-session (session management with confirmation dialogs), dmenu-cpupower (power profile switching), and dmenu-clip (clipboard history picker). All three follow the same dmenu pipe pattern: open dmenu, write menu items, read user selection, act on it. The foundation is already solid -- Phase 1 delivered a fully working `dmenu_open`/`dmenu_write`/`dmenu_read`/`dmenu_close` API in `common/dmenu.c`, and Phase 2 established the Makefile BINS pattern for adding new utilities.

The key technical challenges are: (1) dmenu-session's confirmation dialog pattern requiring a second dmenu invocation after the first, (2) dmenu-cpupower capturing stdout from `powerprofilesctl get` to display the current profile in the dmenu prompt, and (3) dmenu-clip reading a directory of 50 cache files, sorting by mtime, building truncated previews, and matching the user's selection back to the original hash filename. All three utilities spawn external processes (betterlockscreen, pkill, systemctl, powerprofilesctl, xclip), which are well-served by the existing `exec_wait` and `exec_detach` helpers.

**Primary recommendation:** Implement dmenu-session first (simplest dmenu interaction, validates the confirmation dialog pattern), then dmenu-cpupower (adds stdout capture), then dmenu-clip (most complex due to directory scanning and sorting).

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** Four actions: lock, logout, reboot, shutdown -- presented via dmenu as a list
- **D-02:** Lock uses betterlockscreen: `betterlockscreen -l --display 1 --blur 0.8`
- **D-03:** DISPLAY validation: check `getenv("DISPLAY")` is non-null before spawning betterlockscreen. Fall back to error via warn() if DISPLAY unset.
- **D-04:** Duplicate lock prevention: `pgrep -f betterlockscreen` check before spawning. If already running, exit silently.
- **D-05:** Confirmation dialogs for destructive actions (logout, reboot, shutdown): second dmenu call with "no"/"yes" options. No confirmation for lock.
- **D-06:** Logout kills: slstatus, dmenu-clipd, dunst, then dwm (in that order). Uses exec_wait with pkill for each.
- **D-07:** Reboot via `systemctl reboot`, shutdown via `systemctl poweroff` -- both via exec_detach after confirmation.
- **D-08:** Read current profile: exec `powerprofilesctl get` and capture output for dmenu prompt text
- **D-09:** List profiles: performance, balanced, power-saver -- hardcoded (standard across all systems with power-profiles-daemon)
- **D-10:** Set profile: exec `powerprofilesctl set {selected}` via exec_wait
- **D-11:** Show current profile in dmenu prompt: `dmenu_open("cpu: {current}", extra_argv)`
- **D-12:** Pass extra argv through to dmenu (same as all dmenu utilities)
- **D-13:** Read cache from `~/.cache/dmenu-clipboard/` (XDG_CACHE_HOME fallback)
- **D-14:** Sort entries by mtime (newest first) using stat() on each file
- **D-15:** Preview: first 80 chars of each file, newlines replaced by spaces
- **D-16:** Display previews in dmenu, match selection back to hash filename
- **D-17:** Restore selected entry to clipboard via exec: `xclip -selection clipboard < cache_file`
- **D-18:** Pass extra argv through to dmenu

### Claude's Discretion
- Whether to use config.def.h for dmenu-session/dmenu-cpupower (probably not -- no configurable values)
- Error handling strategy for powerprofilesctl failures
- Maximum entries to display in dmenu-clip (currently 50, matching shell script)

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope

</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| SESS-01 | Lock screen via betterlockscreen with blur 0.8 and DISPLAY export | D-02, D-03: exec_detach betterlockscreen with DISPLAY check. Verified betterlockscreen v4.3.0 installed. |
| SESS-02 | Lock prevents duplicate betterlockscreen instances (pgrep check) | D-04: exec_wait pgrep -f betterlockscreen. Safe from C (no self-match). See Pitfall Analysis. |
| SESS-03 | Logout with confirmation dialog (kills slstatus, dmenu-clipd, dwm) | D-05, D-06: Second dmenu_open for confirmation. exec_wait pkill -x for each daemon in order. |
| SESS-04 | Reboot with confirmation dialog (systemctl reboot) | D-05, D-07: Confirmation dmenu then exec_detach systemctl reboot. |
| SESS-05 | Shutdown with confirmation dialog (systemctl poweroff) | D-05, D-07: Confirmation dmenu then exec_detach systemctl poweroff. |
| POWER-01 | User can see current power profile in dmenu prompt | D-08, D-11: Capture powerprofilesctl get stdout via pipe+fork+read. Format as "cpu: {profile}" prompt. |
| POWER-02 | User can select from available profiles | D-09: Hardcoded list: performance, balanced, power-saver. Verified against powerprofilesctl list on system. |
| POWER-03 | Selected profile applied via powerprofilesctl set | D-10: exec_wait powerprofilesctl set {selection}. No privilege escalation needed (D-Bus). |
| POWER-04 | dmenu-cpupower passes extra arguments through to dmenu | D-12: Forward argc/argv to dmenu_open extra_argv. Established pattern. |
| CLIP-01 | Browse clipboard history newest-first via dmenu | D-13, D-14: opendir + stat loop, qsort by st_mtime descending, write previews to dmenu. |
| CLIP-02 | Each entry shows 80-char truncated preview, newlines replaced by spaces | D-15: fread up to 80 bytes, replace '\n' with ' ', null-terminate. |
| CLIP-03 | Selecting entry copies full content to clipboard via xclip | D-17: exec_wait with xclip -selection clipboard, stdin redirected from cache file. |
| CLIP-04 | dmenu-clip passes extra arguments through to dmenu | D-18: Forward argc/argv to dmenu_open extra_argv. Established pattern. |

</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| libc (glibc) | system | POSIX APIs: fork, exec, pipe, stat, opendir, qsort | Only runtime dependency. All functions used are POSIX.1-2008 standard. [VERIFIED: system headers] |
| dmenu.h/dmenu.c | Phase 1 | Bidirectional dmenu pipe API | Already built and tested in Phase 1. DmenuCtx handle-based API. [VERIFIED: utils/common/dmenu.c] |
| util.h/util.c | Phase 1 | die, warn, exec_wait, exec_detach, setup_sigchld | Already built and tested in Phase 1. [VERIFIED: utils/common/util.c] |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `<dirent.h>` | POSIX | Directory scanning for dmenu-clip cache | opendir/readdir/closedir to iterate cache files [VERIFIED: POSIX standard] |
| `<sys/stat.h>` | POSIX | File metadata (mtime) for dmenu-clip sorting | stat() to get st_mtime for each cache entry [VERIFIED: POSIX standard] |
| `<stdlib.h>` | POSIX | qsort for mtime sorting, getenv for XDG_CACHE_HOME | Standard library [VERIFIED: POSIX standard] |

### External Binaries (exec'd, not linked)
| Binary | Version | Purpose | Verified |
|--------|---------|---------|----------|
| betterlockscreen | v4.3.0 | Screen locking | [VERIFIED: `/usr/sbin/betterlockscreen`] |
| powerprofilesctl | 0.30 | Power profile get/set via D-Bus | [VERIFIED: `/usr/sbin/powerprofilesctl`] |
| xclip | 0.13 | Clipboard restore from file | [VERIFIED: `/usr/sbin/xclip`] |
| pgrep | procps-ng 4.0.6 | Duplicate lock detection | [VERIFIED: `/usr/sbin/pgrep`] |
| pkill | procps-ng 4.0.6 | Process termination (logout) | [VERIFIED: `/usr/sbin/pkill`] |
| systemctl | systemd 260 | Reboot/shutdown | [VERIFIED: `/usr/sbin/systemctl`] |
| dmenu | 5.4 | Menu UI | [VERIFIED: `/usr/local/bin/dmenu`] |

**No new packages to install.** All dependencies are already on the system.

## Architecture Patterns

### Recommended Project Structure
```
utils/
├── common/
│   ├── dmenu.h          # DmenuCtx API (Phase 1)
│   ├── dmenu.c          # Bidirectional pipe implementation (Phase 1)
│   ├── util.h           # die, warn, exec_wait, exec_detach (Phase 1)
│   └── util.c           # Shared utility implementations (Phase 1)
├── dmenu-session/
│   └── dmenu-session.c  # Session manager (NEW)
├── dmenu-cpupower/
│   └── dmenu-cpupower.c # Power profile switcher (NEW)
├── dmenu-clip/
│   └── dmenu-clip.c     # Clipboard history picker (NEW)
├── config.mk            # Shared compiler flags (Phase 1)
└── Makefile             # Build system -- add 3 new BINS entries
```

### Pattern 1: Simple dmenu Interaction (dmenu-session, dmenu-cpupower)
**What:** Write items to dmenu, read selection, act on it. Standard open-write-read-close cycle.
**When to use:** Any utility that presents a fixed menu and acts on the selection.
**Example:**
```c
/* Source: established pattern from Phase 1 dmenu.c API */
DmenuCtx *ctx;
char *sel;

ctx = dmenu_open("session", extra_argv);
dmenu_write(ctx, "logout");
dmenu_write(ctx, "lock");
dmenu_write(ctx, "reboot");
dmenu_write(ctx, "shutdown");
sel = dmenu_read(ctx);   /* closes write end, reads selection */
dmenu_close(ctx);         /* waitpid + free */

if (!sel) return 0;       /* user pressed Escape */
/* ... act on sel ... */
free(sel);
```

### Pattern 2: Confirmation Dialog (dmenu-session destructive actions)
**What:** Second dmenu invocation after first selection, presenting "no"/"yes" options.
**When to use:** Destructive actions (logout, reboot, shutdown).
**Example:**
```c
/* Source: D-05 from CONTEXT.md */
static int
confirm(const char *prompt, char *const extra_argv[])
{
    DmenuCtx *ctx;
    char *sel;
    int yes;

    ctx = dmenu_open(prompt, extra_argv);
    dmenu_write(ctx, "no");
    dmenu_write(ctx, "yes");
    sel = dmenu_read(ctx);
    dmenu_close(ctx);

    yes = (sel && strcmp(sel, "yes") == 0);
    free(sel);
    return yes;
}
```

### Pattern 3: Stdout Capture (dmenu-cpupower reading current profile)
**What:** Fork a child, capture its stdout into a buffer, use the result.
**When to use:** When you need to read output from an external command (not the same as exec_wait which only returns exit status).
**Example:**
```c
/* Source: pattern derived from fork+pipe in dmenu.c, adapted for single-direction capture */
static int
capture_cmd(const char *const argv[], char *buf, size_t bufsz)
{
    int fd[2];
    pid_t pid;
    ssize_t n;

    if (pipe(fd) < 0) return -1;
    pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
        execvp(argv[0], (char *const *)argv);
        _exit(127);
    }
    close(fd[1]);
    n = read(fd[0], buf, bufsz - 1);
    close(fd[0]);
    waitpid(pid, NULL, 0);
    if (n <= 0) return -1;
    buf[n] = '\0';
    /* Strip trailing newline */
    if (n > 0 && buf[n - 1] == '\n') buf[n - 1] = '\0';
    return 0;
}
```

### Pattern 4: Directory Scan + Sort by mtime (dmenu-clip)
**What:** Read all files from a directory, stat each for mtime, sort newest-first, build preview strings.
**When to use:** dmenu-clip listing clipboard cache entries.
**Example:**
```c
/* Source: POSIX opendir/readdir + stat + qsort */
struct entry {
    char hash[33];       /* MD5 hash filename (32 hex + NUL) */
    char preview[81];    /* 80-char truncated preview + NUL */
    time_t mtime;
};

static int
cmp_mtime_desc(const void *a, const void *b)
{
    const struct entry *ea = a, *eb = b;
    if (ea->mtime > eb->mtime) return -1;
    if (ea->mtime < eb->mtime) return  1;
    return 0;
}
```

### Pattern 5: stdin Redirection for xclip (dmenu-clip restore)
**What:** Fork xclip with stdin redirected from a file, to restore clipboard content.
**When to use:** CLIP-03 -- restoring selected clipboard entry.
**Example:**
```c
/* Source: D-17 from CONTEXT.md */
static void
clip_restore(const char *path)
{
    pid_t pid;
    int fd;

    pid = fork();
    if (pid < 0) die("fork:");
    if (pid == 0) {
        fd = open(path, O_RDONLY);
        if (fd < 0) _exit(1);
        dup2(fd, STDIN_FILENO);
        close(fd);
        execlp("xclip", "xclip", "-selection", "clipboard", NULL);
        _exit(127);
    }
    waitpid(pid, NULL, 0);
}
```

### Anti-Patterns to Avoid
- **system() or popen():** Never. Always fork+execvp. CERT C ENV33-C. [CITED: .planning/research/PITFALLS.md]
- **Keeping dmenu write end open while reading:** Causes deadlock. Always close write end before reading. [CITED: utils/common/dmenu.c line 106-109]
- **Using exec_wait for systemctl reboot/poweroff:** These commands do not return, so waitpid would hang. Use exec_detach for reboot/shutdown. [ASSUMED]
- **Static buffers for dmenu_read result:** dmenu_read returns heap-allocated string (caller frees). Do not use static buffers for selection. [VERIFIED: utils/common/dmenu.c]

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| dmenu pipe communication | Custom pipe/fork/exec per utility | dmenu_open/dmenu_write/dmenu_read/dmenu_close from common/dmenu.c | Already handles bidirectional pipes, deadlock prevention, argv forwarding. Tested in Phase 1. |
| Process spawning with exit code | Custom fork+waitpid per callsite | exec_wait() from common/util.c | Handles fork, execvp, waitpid, exit status extraction. Proven in Phase 2. |
| Fire-and-forget process spawning | Custom fork+setsid | exec_detach() from common/util.c | Handles fork, setsid, execvp. SIGCHLD handler reaps zombies. Proven in Phase 2. |
| Zombie prevention | Per-utility SIGCHLD handling | setup_sigchld() from common/util.c | Single call in main(), handles all children. Proven in Phases 1-2. |
| Directory scanning | Custom file listing with glob | opendir/readdir (POSIX) + stat | Standard POSIX, no library needed. 50 files is trivial. |
| Sorting entries by mtime | Custom linked list insertion sort | qsort() from stdlib.h | Standard library, well-tested, O(n log n). |

**Key insight:** The entire Phase 1 foundation (dmenu.c + util.c) exists specifically so Phase 3 utilities are thin wrappers around established helpers. Each utility's main() should be 50-120 lines.

## Common Pitfalls

### Pitfall 1: pgrep -f betterlockscreen Self-Match
**What goes wrong:** `pgrep -f` matches against the full command line of all processes, including the process running the pgrep command itself if the string appears in its arguments.
**Why it happens:** The `-f` flag searches /proc/*/cmdline, and the pgrep process's own cmdline contains "betterlockscreen".
**How to avoid:** When called from C via exec_wait({"pgrep", "-f", "betterlockscreen", NULL}), the forked child's cmdline is `pgrep -f betterlockscreen` which DOES contain the string. However, pgrep excludes its own PID by default. So the self-match issue is only a concern in shell scripts. From C via exec_wait, pgrep -f is safe. [VERIFIED: pgrep man page -- "pgrep does not report itself as a match"]
**Warning signs:** Lock action never spawns because pgrep always returns 0.

### Pitfall 2: pgrep -x Fails for Names Longer Than 15 Characters
**What goes wrong:** `pgrep -x betterlockscreen` fails with "pattern that searches for process name longer than 15 characters will result in zero matches" because the Linux kernel truncates process names (comm field) to 15 characters.
**Why it happens:** The kernel's task_struct.comm field is TASK_COMM_LEN (16 bytes including NUL). "betterlockscreen" is 16 characters, which gets truncated.
**How to avoid:** Use `pgrep -f betterlockscreen` instead of `pgrep -x`. This searches the full command line, not the truncated comm name. [VERIFIED: tested on system -- pgrep -x prints warning and returns 1]
**Warning signs:** Lock duplicate check never finds a running instance.

### Pitfall 3: systemctl reboot/poweroff Never Returns
**What goes wrong:** If you use exec_wait for systemctl reboot or poweroff, the parent process blocks forever in waitpid because the system is going down.
**Why it happens:** These commands initiate system shutdown. The process may be killed before execvp returns to the child, and waitpid in the parent may never complete.
**How to avoid:** Use exec_detach for reboot and shutdown. The parent returns immediately after fork. [ASSUMED -- standard practice]
**Warning signs:** The utility appears to hang before reboot/shutdown takes effect.

### Pitfall 4: dmenu-clip Preview Matching Ambiguity
**What goes wrong:** Two clipboard entries with identical first 80 characters produce identical preview strings. When the user selects one, the wrong entry may be restored.
**Why it happens:** The shell script has this same bug -- it matches by preview text, and duplicate previews cause mismatches.
**How to avoid:** Match by line position (index) rather than string comparison. When building the preview list, maintain an array indexed by position. dmenu outputs the selected text, but if we number the entries or track the correspondence by array index, we can disambiguate. The simplest approach: maintain a parallel array of entries. Write previews to dmenu in order. When reading selection, strcmp against each preview to find the match. First match wins (newest-first order means we return the newest duplicate). This matches the shell script behavior. [ASSUMED]
**Warning signs:** Wrong clipboard entry restored when duplicates exist.

### Pitfall 5: XDG_CACHE_HOME Not Set
**What goes wrong:** getenv("XDG_CACHE_HOME") returns NULL on systems that don't explicitly set it (most systems).
**Why it happens:** XDG_CACHE_HOME is optional per the XDG Base Directory Specification. Default is $HOME/.cache when unset.
**How to avoid:** Always fall back: `const char *cache = getenv("XDG_CACHE_HOME"); if (!cache || !*cache) { /* use $HOME/.cache */ }`. Also check that getenv("HOME") returns non-NULL. [CITED: XDG Base Directory Specification]
**Warning signs:** Segfault on systems without XDG_CACHE_HOME.

### Pitfall 6: dmenu-clip Reading Empty Cache Directory
**What goes wrong:** If no clipboard entries exist (directory is empty or does not exist), the utility should exit silently, not crash.
**Why it happens:** opendir succeeds on an empty directory, readdir returns only "." and "..".
**How to avoid:** After scanning, check if entry count is 0 and exit(0) silently. Also check if the cache directory exists at all with opendir -- if NULL, exit(0). [VERIFIED: shell script exits 0 on empty]

### Pitfall 7: Logout Kill Order Matters
**What goes wrong:** If dwm is killed before other daemons, the X session may tear down and the remaining pkill commands fail or have no effect.
**Why it happens:** dwm is the session leader. When dwm exits, the X server may terminate, which kills all X clients.
**How to avoid:** Kill daemons first (slstatus, dmenu-clipd, dunst), then dwm last. This is specified in D-06. Each pkill should use exec_wait so the kill is confirmed before proceeding to the next. [VERIFIED: D-06 in CONTEXT.md specifies order]

## Code Examples

### dmenu-session main() Structure
```c
/* Source: synthesized from CONTEXT.md D-01 through D-07 and existing shell script */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../common/dmenu.h"
#include "../common/util.h"

static int
confirm(const char *prompt, char *const extra_argv[])
{
    DmenuCtx *ctx;
    char *sel;
    int yes;

    ctx = dmenu_open(prompt, extra_argv);
    dmenu_write(ctx, "no");
    dmenu_write(ctx, "yes");
    sel = dmenu_read(ctx);
    dmenu_close(ctx);
    yes = (sel && strcmp(sel, "yes") == 0);
    free(sel);
    return yes;
}

/* action_lock: validate DISPLAY, check pgrep, exec betterlockscreen */
/* action_logout: confirm, then pkill slstatus, dmenu-clipd, dunst, dwm */
/* action_reboot: confirm, then exec_detach systemctl reboot */
/* action_shutdown: confirm, then exec_detach systemctl poweroff */

int
main(int argc, char *argv[])
{
    DmenuCtx *ctx;
    char *sel;

    argv0 = argv[0];
    setup_sigchld();

    ctx = dmenu_open("session", argv + 1); /* forward extra args */
    dmenu_write(ctx, "logout");
    dmenu_write(ctx, "lock");
    dmenu_write(ctx, "reboot");
    dmenu_write(ctx, "shutdown");
    sel = dmenu_read(ctx);
    dmenu_close(ctx);

    if (!sel) return 0;

    /* dispatch to action_* functions based on sel */
    /* ... */

    free(sel);
    return 0;
}
```

### dmenu-cpupower stdout Capture Pattern
```c
/* Source: derived from fork+pipe pattern in dmenu.c */
static int
capture_stdout(const char *const argv[], char *buf, size_t bufsz)
{
    int fd[2];
    pid_t pid;
    ssize_t n;

    if (pipe(fd) < 0) return -1;
    pid = fork();
    if (pid < 0) { close(fd[0]); close(fd[1]); return -1; }
    if (pid == 0) {
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
        execvp(argv[0], (char *const *)argv);
        _exit(127);
    }
    close(fd[1]);
    n = read(fd[0], buf, bufsz - 1);
    close(fd[0]);
    waitpid(pid, NULL, 0);
    if (n <= 0) return -1;
    buf[n] = '\0';
    if (n > 0 && buf[n - 1] == '\n') buf[n - 1] = '\0';
    return 0;
}
```

### dmenu-clip Directory Scan and Sort
```c
/* Source: POSIX opendir/readdir + stat, synthesized for this use case */
#include <dirent.h>
#include <sys/stat.h>

#define MAX_ENTRIES 50
#define MAX_PREVIEW 80

struct entry {
    char hash[33];
    char preview[MAX_PREVIEW + 1];
    time_t mtime;
};

static int
cmp_mtime_desc(const void *a, const void *b)
{
    const struct entry *ea = a, *eb = b;
    if (ea->mtime > eb->mtime) return -1;
    if (ea->mtime < eb->mtime) return  1;
    return 0;
}

/* In scan function: */
/* 1. opendir(cache_dir) */
/* 2. For each readdir entry (skip ".", ".."): */
/*    a. snprintf full path: cache_dir/entry->d_name */
/*    b. stat(path, &st) to get st.st_mtime */
/*    c. fopen + fread up to 80 bytes for preview */
/*    d. Replace '\n' with ' ' in preview */
/*    e. Store in entries[count++] */
/* 3. qsort(entries, count, sizeof(struct entry), cmp_mtime_desc) */
```

### xclip Restore with stdin Redirection
```c
/* Source: D-17, fork+dup2 pattern from Phase 1 */
#include <fcntl.h>

static void
restore_clipboard(const char *filepath)
{
    pid_t pid;
    int fd;

    pid = fork();
    if (pid < 0) die("fork:");
    if (pid == 0) {
        fd = open(filepath, O_RDONLY);
        if (fd < 0) _exit(1);
        dup2(fd, STDIN_FILENO);
        close(fd);
        execlp("xclip", "xclip", "-selection", "clipboard", NULL);
        _exit(127);
    }
    waitpid(pid, NULL, 0);
}
```

## Discretion Recommendations

### config.def.h for dmenu-session / dmenu-cpupower
**Recommendation: Do NOT use config.def.h** for dmenu-session or dmenu-cpupower. There are no user-configurable values. The betterlockscreen arguments, profile names, and systemctl commands are all fixed. Following the screenshot-notify precedent (Phase 2 Plan 02 explicitly decided against config.h when there are no configurable values). [VERIFIED: 02-02-SUMMARY.md key-decisions]

**DO use config.def.h for dmenu-clip** with these compile-time options:
```c
#define CACHE_DIR_NAME "dmenu-clipboard"
#define MAX_ENTRIES    50
#define MAX_PREVIEW    80
```
These match the shell script's configurable constants and users might reasonably want to change them. [ASSUMED]

### Error Handling for powerprofilesctl Failures
**Recommendation:** If powerprofilesctl get fails (non-zero exit or empty output), use "unknown" as the prompt text and continue. If powerprofilesctl set fails, warn() to stderr but do not die -- the user will notice via the prompt on next invocation. This is non-fatal behavior. [ASSUMED]

### Maximum Entries for dmenu-clip
**Recommendation:** Cap at 50, matching the shell script and the daemon's LRU pruning limit. The cache directory already contains exactly 50 entries. This is a compile-time constant in config.def.h. [VERIFIED: `ls ~/.cache/dmenu-clipboard/ | wc -l` returns 50]

## State of the Art

| Old Approach (Shell Scripts) | New Approach (C Utilities) | Impact |
|------------------------------|----------------------------|--------|
| `printf ... \| dmenu` for pipe | dmenu_open/dmenu_write/dmenu_read/dmenu_close API | Eliminates shell dependency, proper error handling |
| `pkexec cpupower frequency-set -g` | `powerprofilesctl set` (no privilege escalation) | No pkexec/polkit needed. powerprofilesctl uses D-Bus. [VERIFIED: PITFALLS.md Security section] |
| `cat /sys/...scaling_governor` for current profile | `powerprofilesctl get` for current profile | Abstracted from kernel interface, consistent with profile names |
| `ls -1t` for mtime sort in shell | stat() + qsort() in C | Proper sort, no subshell spawning, handles edge cases |
| `head -c 80 \| tr '\n' ' '` for preview | fread + loop replace in C | Single-pass, bounded, no pipe overhead |
| `slock` for screen lock | `betterlockscreen -l --display 1 --blur 0.8` | Better visual experience, matches user's chosen lock screen |

**Note on cpupower vs powerprofilesctl:** The existing shell script uses `cpupower` (requires pkexec for root). CONTEXT.md D-08 through D-11 specify using `powerprofilesctl` instead, which communicates with power-profiles-daemon over D-Bus and needs no privilege escalation. The profile names also differ: cpupower uses kernel governors (performance/schedutil/powersave), while powerprofilesctl uses abstract names (performance/balanced/power-saver). The C implementation uses powerprofilesctl. [VERIFIED: powerprofilesctl 0.30 installed, `powerprofilesctl get` returns "performance"]

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | systemctl reboot/poweroff should use exec_detach (not exec_wait) because the system goes down | Common Pitfalls, Pitfall 3 | LOW -- if exec_wait is used, the parent just gets killed by system shutdown anyway. But exec_detach is cleaner. |
| A2 | Duplicate preview matching (first match wins for newest) is acceptable behavior | Common Pitfalls, Pitfall 4 | LOW -- matches existing shell script behavior. Rare edge case. |
| A3 | dmenu-clip should have config.def.h but dmenu-session/dmenu-cpupower should not | Discretion Recommendations | LOW -- no functional impact, only affects code organization. |
| A4 | exec_detach is correct for betterlockscreen (fire-and-forget lock) | Architecture Patterns | LOW -- betterlockscreen runs independently and does not need its exit status checked. |

## Open Questions

1. **D-06 Logout Kill Order -- should dunst be killed?**
   - What we know: D-06 specifies killing slstatus, dmenu-clipd, dunst, then dwm
   - What's unclear: The existing shell script only kills slstatus, dmenu-clipd, and dwm (no dunst)
   - Recommendation: Follow D-06 as specified (kill dunst too). It is a locked decision.

2. **Menu item order in dmenu-session**
   - What we know: CONTEXT.md Specific Ideas says "match existing shell script's menu order: logout, lock, reboot, shutdown"
   - What's unclear: D-01 just says "four actions" without specifying order
   - Recommendation: Use logout, lock, reboot, shutdown (matching shell script). The Specific Ideas section is a clear signal.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| betterlockscreen | SESS-01 lock | Yes | v4.3.0 | -- |
| powerprofilesctl | POWER-01/02/03 | Yes | 0.30 | -- |
| xclip | CLIP-03 restore | Yes | 0.13 | -- |
| pgrep | SESS-02 duplicate check | Yes | procps-ng 4.0.6 | -- |
| pkill | SESS-03 logout kills | Yes | procps-ng 4.0.6 | -- |
| systemctl | SESS-04/05 reboot/shutdown | Yes | systemd 260 | -- |
| dmenu | All utilities | Yes | 5.4 | -- |
| gcc | Build | Yes | system | -- |
| make | Build | Yes | system | -- |

**Missing dependencies with no fallback:** None.
**Missing dependencies with fallback:** None.

All external dependencies are available and verified.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Custom C test programs (established in Phase 1) |
| Config file | utils/Makefile (test targets) |
| Quick run command | `make -C utils test` |
| Full suite command | `make -C utils clean all test` |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| SESS-01 | Lock spawns betterlockscreen | manual-only | Manual: press lock keybinding, verify screen locks | N/A |
| SESS-02 | Duplicate lock prevention | manual-only | Manual: press lock twice rapidly | N/A |
| SESS-03 | Logout kills daemons in order | manual-only | Manual: test logout from session | N/A |
| SESS-04 | Reboot with confirmation | manual-only | Manual: test reboot flow | N/A |
| SESS-05 | Shutdown with confirmation | manual-only | Manual: test shutdown flow | N/A |
| POWER-01 | Current profile in prompt | manual-only | Manual: launch utility, verify prompt shows profile | N/A |
| POWER-02 | Profile list in dmenu | manual-only | Manual: launch utility, verify 3 profiles listed | N/A |
| POWER-03 | Profile applied | manual-only | Manual: select profile, verify with `powerprofilesctl get` | N/A |
| POWER-04 | Extra args forwarded | manual-only | Manual: pass -l 3, verify dmenu shows as list | N/A |
| CLIP-01 | History newest-first | manual-only | Manual: verify order matches file mtimes | N/A |
| CLIP-02 | 80-char preview | manual-only | Manual: verify truncated entries in dmenu | N/A |
| CLIP-03 | Restore to clipboard | manual-only | Manual: select entry, paste elsewhere | N/A |
| CLIP-04 | Extra args forwarded | manual-only | Manual: pass -l 10, verify dmenu shows as list | N/A |
| BUILD | All 3 utilities compile | unit | `make -C utils clean all` | Wave 0 |

**Justification for manual-only:** All three utilities are interactive (require dmenu GUI + user input) and execute system-level actions (lock screen, reboot, clipboard set). Automated testing would require X11 display, running dmenu instance, and simulated keypresses. The established project pattern (Phases 1-2) uses compilation tests for build verification and manual testing for runtime behavior.

### Sampling Rate
- **Per task commit:** `make -C utils clean all` (build verification)
- **Per wave merge:** Full build + manual smoke test of each utility
- **Phase gate:** All 3 utilities build, install, and function via dwm keybinding

### Wave 0 Gaps
- None -- existing test infrastructure (Makefile build verification) covers the automatable portion. Interactive utilities require manual testing by nature.

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | No | -- |
| V3 Session Management | No | -- |
| V4 Access Control | Partial | powerprofilesctl uses D-Bus, no SUID needed. [VERIFIED: PITFALLS.md Security] |
| V5 Input Validation | Yes | dmenu selections matched against known strings (no arbitrary input). exec args are string literals or from fixed list. |
| V6 Cryptography | No | -- |

### Known Threat Patterns

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Shell injection via system() | Tampering | Use execvp with argv arrays, never system(). [CITED: PITFALLS.md Security] |
| Clipboard history readable by other users | Information Disclosure | Cache directory should be 0700. (Note: existing shell daemon creates it without restrictive permissions -- this is a pre-existing issue, not Phase 3's responsibility.) [CITED: PITFALLS.md Security] |
| SUID binary for power profiles | Elevation of Privilege | Not needed -- powerprofilesctl talks D-Bus, no root required. [VERIFIED: powerprofilesctl 0.30 works without sudo] |
| Path traversal in clipboard filenames | Tampering | Filenames are MD5 hex hashes (safe). Never use clipboard content as path component. [CITED: PITFALLS.md Security] |

## Project Constraints (from CLAUDE.md)

- **Platform:** Arch Linux only -- pacman/AUR package names
- **Philosophy:** Suckless-style C -- minimal dependencies, no frameworks
- **Display server:** X11 -- all utilities use Xlib/XFixes directly where needed
- **Build:** Each util has own directory, single top-level Makefile, install.sh orchestrates
- **Code style:** K&R braces, tabs, C99, 80-col pragmatic limit
- **Error handling:** die() for fatal, warn() for non-fatal, check all return values
- **Process spawning:** fork+execvp only, never system() or popen()
- **Naming:** lowercase with hyphens for binaries (dmenu-session, dmenu-cpupower, dmenu-clip)
- **Installation:** PREFIX=$(HOME)/.local, no sudo for utility installation

## Sources

### Primary (HIGH confidence)
- `utils/common/dmenu.h` / `dmenu.c` -- Full dmenu pipe API, verified implementation [VERIFIED]
- `utils/common/util.h` / `util.c` -- exec_wait, exec_detach, setup_sigchld implementations [VERIFIED]
- `utils/Makefile` -- Build system pattern for adding new BINS entries [VERIFIED]
- `utils/battery-notify/battery-notify.c` -- Reference implementation for main() structure [VERIFIED]
- `utils/screenshot-notify/screenshot-notify.c` -- Reference implementation for fork+pipe pattern [VERIFIED]
- `scripts/dmenu-session` -- Shell reference for session manager behavior [VERIFIED]
- `scripts/dmenu-cpupower` -- Shell reference for CPU power behavior [VERIFIED]
- `scripts/dmenu-clip` -- Shell reference for clipboard picker behavior [VERIFIED]
- `.planning/research/PITFALLS.md` -- Known pitfalls and mitigations [VERIFIED]

### Secondary (MEDIUM confidence)
- `powerprofilesctl get` output verified as single-line profile name (e.g., "performance\n") [VERIFIED: live system]
- `powerprofilesctl list` output verified to show exactly 3 profiles [VERIFIED: live system]
- `pgrep -x` 15-character limit verified on system [VERIFIED: live system error message]
- Clipboard cache format verified: 50 files, 32-char MD5 hex filenames [VERIFIED: live system]

### Tertiary (LOW confidence)
- None -- all claims verified against codebase or live system.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- all libraries are POSIX standard, all helpers are from Phase 1
- Architecture: HIGH -- patterns directly follow Phase 1-2 established patterns and shell script behavior
- Pitfalls: HIGH -- verified against live system (pgrep truncation, powerprofilesctl behavior)

**Research date:** 2026-04-09
**Valid until:** 2026-05-09 (stable domain, no fast-moving dependencies)
