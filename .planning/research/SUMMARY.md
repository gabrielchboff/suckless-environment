# Project Research Summary

**Project:** Suckless Environment v2 — Native C Utilities
**Domain:** Suckless C desktop utilities (X11, process spawning, sysfs, D-Bus, dmenu integration)
**Researched:** 2026-04-09
**Confidence:** HIGH

## Executive Summary

This project converts six shell scripts into native C utilities that integrate with a suckless dwm environment. Experts building in this space follow the suckless pattern: single-purpose binaries, explicit per-utility Makefiles, no runtime configuration, fork/execvp for process spawning, and zero new system dependencies where possible. The recommended approach is to build a small shared `common/` library (die, warn, pscanf, exec helpers, dmenu pipe helpers) first, then implement utilities in order of increasing complexity — starting with one-shot utilities (battery-notify, screenshot-notify), then interactive dmenu utilities (dmenu-session, dmenu-cpupower, dmenu-clip), and finally the event-driven daemon (dmenu-clipd). This ordering validates patterns at each step before tackling the most complex piece.

The key risks are concentrated in dmenu-clipd. X11 clipboard ownership requires both monitoring changes via XFixes events AND serving data to requestors via the SelectionRequest protocol — omitting the latter means clipboard data vanishes when the source application closes. The other pitfalls (zombie processes, dmenu pipe deadlock, signal handler safety) are well-understood and easily mitigated by establishing correct patterns in the first utility and copying them forward. Every library dependency is already installed on this system (libX11, libXfixes, libsystemd); zero new packages are needed.

The recommended stack is deliberately minimal: libXfixes for event-driven clipboard monitoring, sd-bus (via libsystemd) for D-Bus notifications and power profile management, Linux sysfs for battery reading, and inline FNV-1a for content hashing — replacing the shell scripts' dependency on xclip polling, libnotify, and md5sum. The result is six self-contained binaries under ~300 lines of C each, with no new runtime dependencies beyond what the existing environment already provides.

## Key Findings

### Recommended Stack

All build dependencies are already installed on this Arch system. No `pacman -S` required. The stack stays strictly within the suckless convention of explicit Makefile flags and zero dependency bloat: libX11 + libXfixes for the clipboard daemon event loop, libsystemd (sd-bus) for D-Bus power profile queries and freedesktop notifications, Linux sysfs for battery data, and POSIX fork/execvp for all process spawning. FNV-1a replaces OpenSSL MD5 for clipboard deduplication — it is 10 inline lines, dependency-free, and sufficient for content hashing where cryptographic strength is irrelevant.

**Core technologies:**
- libX11 + libXfixes (`-lX11 -lXfixes`): clipboard event detection via `XFixesSelectSelectionInput` — eliminates the 0.5s poll loop; already installed as dwm dependency
- libsystemd sd-bus (`-lsystemd`): D-Bus client for power-profiles-daemon (read/write `ActiveProfile`) and `org.freedesktop.Notifications.Notify` — replaces libnotify's 15-library dependency chain; already installed
- Linux sysfs (`/sys/class/power_supply/BAT1/`): battery capacity and status — zero dependencies, stable kernel ABI; this machine uses BAT1 not BAT0
- POSIX fork/execvp: process spawning for dmenu, betterlockscreen, maim, xclip, systemctl, powerprofilesctl — no shell interpreter involved
- FNV-1a (inline, ~10 lines): clipboard content deduplication — replaces OpenSSL MD5, zero new `-l` flags
- Per-utility Makefiles with `config.mk`: matches dwm/dmenu/slstatus build convention exactly

### Expected Features

The six utilities must match or exceed the existing shell scripts for v1. The flagship improvement is dmenu-clipd's XFixes event-driven loop replacing the 0.5s xclip poll. All other features are low-complexity and well-understood.

**Must have (table stakes, v1):**
- dmenu-clipd: XFixes event-driven monitoring, text-only storage, hash dedup, max 50 entries with LRU pruning, graceful SIGTERM shutdown, single-instance enforcement via flock
- dmenu-clip: newest-first listing, 80-char truncated preview, xclip-based clipboard restore, dmenu arg passthrough
- dmenu-session: lock (betterlockscreen), logout, reboot, shutdown with confirmation dialogs for destructive actions
- dmenu-cpupower: list profiles, show current in prompt, set via powerprofilesctl, dmenu arg passthrough
- battery-notify: one-shot sysfs read, notify at <=20% discharging, no re-alert spam (state file)
- screenshot-notify: area capture via maim -s, copy to clipboard, dunst notification on success, silent on cancel

**Should have (v1.x, after all six are stable):**
- battery-notify: two-tier alerts (warning at 20%, critical at 5% with persistent notification)
- screenshot-notify: multiple capture modes (area, fullscreen, active window) via argument
- dmenu-clipd: atomic writes (write to `.tmp`, then `rename()`)
- dmenu-session: duplicate betterlockscreen prevention (pgrep check before spawn)

**Defer (v2+):**
- dmenu-clip: direct Xlib clipboard ownership (removes xclip dependency; complex SelectionRequest event loop)
- battery-notify: systemd .timer + .service unit files for periodic execution
- dmenu-session: suspend/hibernate support
- Any Wayland support (explicitly out of scope per PROJECT.md)

### Architecture Approach

The project divides cleanly into three component classifications. The `common/` library (util.c + dmenu.c) is compiled into each binary that needs it — no shared library, static linkage only. Interactive utilities (dmenu-session, dmenu-cpupower, dmenu-clip) use the fork+dup2+exec dmenu pipe pattern: write menu items to stdin, read selection from stdout. The daemon (dmenu-clipd) runs an XFixes event loop and communicates with dmenu-clip exclusively through the filesystem cache at `~/.cache/dmenu-clipboard/`. One-shot utilities (battery-notify, screenshot-notify) read state, act, and exit.

**Major components:**
1. `common/util.{c,h}` — die(), warn(), pscanf(), exec_wait(), exec_detach(); used by all 6 utilities; modeled on slstatus/util.c
2. `common/dmenu.{c,h}` — dmenu_open(), dmenu_select() pipe helpers; used by the 3 interactive utilities
3. `dmenu-clipd` — XFixes event loop, XConvertSelection clipboard retrieval, FNV-1a hashing, file cache writes, SelectionRequest serving, flock single-instance guard
4. `dmenu-clip` — reads cache dir sorted by mtime, pipes previews to dmenu, restores via xclip
5. `dmenu-session` — two-level dmenu interaction (menu + confirmation), forks betterlockscreen/systemctl/pkill
6. `dmenu-cpupower` — reads powerprofilesctl output, pipes profiles to dmenu, execs powerprofilesctl set
7. `battery-notify` — pscanf sysfs read, threshold check, state file to prevent re-alert spam, execs notify-send
8. `screenshot-notify` — forks maim|xclip pipeline, checks exit status, execs notify-send on success only

### Critical Pitfalls

1. **X11 clipboard SelectionRequest not served** — dmenu-clipd must respond to `SelectionRequest` events (not just monitor via XFixes) or clipboard data vanishes when the source app closes. Implement the full ownership+serving protocol: `XSetSelectionOwner` + `XChangeProperty` + `SelectionNotify` response + TARGETS atom support.

2. **Zombie processes from fire-and-forget spawning** — every utility spawns children. Install a SIGCHLD handler calling `waitpid(-1, NULL, WNOHANG)` in a loop for fire-and-forget children, or use `waitpid()` directly for commands where exit status matters. Establish this pattern in the first utility built and copy it forward.

3. **dmenu bidirectional pipe deadlock** — using `popen()` only gives one direction; attempting bidirectional communication deadlocks if the write side fills the kernel buffer. Always use fork+dup2+exec: write all menu items to dmenu's stdin, close the write end, then read selection from dmenu's stdout.

4. **DISPLAY not propagated to betterlockscreen** — use `execvp()` (inherits environment) not `execve()` with a custom `envp`. Validate `getenv("DISPLAY")` is non-null before spawning betterlockscreen; fall back to `slock` or a dunst error notification on failure.

5. **Battery re-alert spam** — a one-shot binary run from a cron timer will notify every minute when battery stays below threshold. Write a state flag file (`/tmp/battery-notified`) when alerting; check for it on each run; clear it when charge rises above threshold.

## Implications for Roadmap

Based on the architecture recommended build order and the pitfall dependency structure, a 4-phase approach is appropriate.

### Phase 1: Foundation — common/ library and build system

**Rationale:** All six utilities depend on common/util.c for die/warn/pscanf/exec helpers, and three depend on common/dmenu.c for the dmenu pipe pattern. Building this first means every subsequent phase writes less boilerplate. It also establishes the Makefile structure that all utilities inherit and validates the slstatus pattern extraction.

**Delivers:** `common/util.{c,h}`, `common/dmenu.{c,h}`, per-utility `config.mk` + `Makefile` skeletons, install.sh integration stubs.

**Addresses:** Build system (STACK.md), shared code analysis (ARCHITECTURE.md).

**Avoids:** Divergent patterns across utilities, anti-pattern of `system()` vs fork/execvp (PITFALLS.md Pitfalls 2 and 3).

### Phase 2: One-shot utilities — battery-notify and screenshot-notify

**Rationale:** These are the simplest utilities (no dmenu interaction, no X11, no daemon lifecycle). battery-notify is ~60-80 lines; screenshot-notify is ~40 lines. Building them first validates the build system, exec_wait/exec_detach patterns, and sysfs reading approach before touching anything more complex. Both establish the SIGCHLD zombie-reaping pattern that all subsequent utilities inherit.

**Delivers:** Working `battery-notify` (sysfs read + threshold check + state file + notify-send) and `screenshot-notify` (maim|xclip pipeline + exit-status check + notify-send).

**Uses:** common/util only; exec notify-send for notifications (zero new complexity vs sd-bus direct call for v1).

**Implements:** sysfs direct read pattern, exec-only action pattern.

**Avoids:** Sysfs stale reads (open/read/close per pscanf pattern), re-alert spam (state file), screenshot cancel notification (check WEXITSTATUS before notifying).

### Phase 3: Interactive dmenu utilities — dmenu-session, dmenu-cpupower, dmenu-clip

**Rationale:** These three share the dmenu pipe pattern from common/dmenu.c. Building them together in sequence keeps context: dmenu-session is simplest (hardcoded items), dmenu-cpupower adds reading command output, dmenu-clip adds directory traversal and file I/O. The confirmation-dialog pattern in dmenu-session (second dmenu call) is the most novel piece.

**Delivers:** Working `dmenu-session` (lock/logout/reboot/shutdown with confirmation), `dmenu-cpupower` (list/show/set power profiles), `dmenu-clip` (clipboard history picker that reads dmenu-clipd's cache).

**Implements:** dmenu fork+dup2+exec pipe pattern, argv passthrough design.

**Avoids:** Bidirectional pipe deadlock (use fork+dup2+exec, never popen for write+read), DISPLAY not propagated (validate getenv before betterlockscreen exec), dmenu missing final newline on menu items.

### Phase 4: Clipboard daemon — dmenu-clipd

**Rationale:** This is architecturally distinct from the other five — long-running daemon, X11 event loop, X11 selection protocol ownership, flock-based single-instance enforcement. It depends on the cache directory contract that dmenu-clip (Phase 3) already reads. Building it last means the rest of the system is working and the daemon can be integrated into a running environment for immediate end-to-end validation.

**Delivers:** Working `dmenu-clipd` daemon: XFixes event-driven clipboard monitoring, full SelectionRequest serving (TARGETS + UTF8_STRING), FNV-1a content hashing, LRU pruning at 50 entries, flock single-instance guard, SIGTERM graceful shutdown.

**Uses:** libX11 + libXfixes exclusively; common/util only from shared code.

**Implements:** X11 XFixes event-driven pattern, file-based IPC with dmenu-clip.

**Avoids:** Clipboard data loss on app exit (serve SelectionRequest), XNextEvent blocking SIGTERM (ConnectionNumber+select or XPending check), duplicate instances (flock), zombie processes (SIGCHLD handler).

### Phase Ordering Rationale

- common/ must precede all utilities — it is the foundation with no dependencies.
- One-shot utilities (Phase 2) have no user-facing interaction complexity; they are the fastest path to a working binary and validate build system and exec patterns first.
- Interactive utilities (Phase 3) share the dmenu pipe pattern; building them in sequence lets each build on lessons from the previous one.
- dmenu-clipd (Phase 4) is architecturally unique and isolated as the final phase so its X11 selection complexity does not block the other five utilities from becoming usable.
- The cache directory interface between dmenu-clipd (Phase 4) and dmenu-clip (Phase 3) is one-directional: clip reads, clipd writes. dmenu-clip can be tested with a manually populated cache before the daemon exists.

### Research Flags

Phases likely needing deeper research during planning:
- **Phase 4 (dmenu-clipd):** X11 selection protocol (SelectionRequest + TARGETS handling) is well-documented but intricate. Verify the exact `XConvertSelection` to `SelectionNotify` to `XGetWindowProperty` sequence in clipnotify and scm source before writing code. The INCR protocol for payloads >256KB is a known gap acceptable to skip for MVP with a documented limitation.

Phases with standard patterns (skip research-phase):
- **Phase 1 (common/):** Directly modeled on slstatus/util.c already in the codebase. Pattern is completely known.
- **Phase 2 (one-shot utilities):** sysfs reading is identical to slstatus/components/battery.c. fork/exec is standard POSIX. No novel territory.
- **Phase 3 (interactive utilities):** dmenu pipe pattern is documented with working code examples. powerprofilesctl CLI interface verified via busctl. No research needed.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | All libraries verified installed on system; versions confirmed; D-Bus interface verified via busctl introspect; sysfs paths verified against BAT1 on this machine |
| Features | HIGH | Cross-referenced against clipmenu, scm, lowbat, dlogout; feature set matches or exceeds existing shell scripts; anti-features clearly identified |
| Architecture | HIGH | Patterns sourced from existing codebase (slstatus/util.c, dwm/config.h) and verified C reference implementations (clipnotify); build order matches dependency graph |
| Pitfalls | HIGH | Each pitfall verified against official specs (ICCCM, kernel docs, CERT C, signal-safety man page) and issue trackers; recovery strategies are low-effort |

**Overall confidence:** HIGH

### Gaps to Address

- **INCR protocol for large clipboard entries:** Required for clipboard data exceeding 256KB. Acceptable to skip for MVP — document as a known limitation at the SelectionRequest handler in dmenu-clipd. No action needed during planning.
- **Battery device detection:** This machine uses BAT1. Code should use `#define BATTERY_NAME "BAT1"` in config.mk as a compile-time override. Scanning `/sys/class/power_supply/` for type=="Battery" is the robust approach but is a v1.x improvement. Approach is clear; no research gap.
- **powerprofilesctl availability check:** dmenu-cpupower should verify power-profiles-daemon is running before setting a profile. Check powerprofilesctl list exit status before attempting set. Implementation detail for Phase 3 planning.
- **notify-send vs sd-bus direct call for notifications:** STACK.md recommends sd-bus direct call; ARCHITECTURE.md recommends exec notify-send for simplicity. Both work. Resolve during Phase 2: pick exec notify-send for v1 (simpler, consistent with suckless exec-everything philosophy) and apply consistently to both battery-notify and screenshot-notify.

## Sources

### Primary (HIGH confidence)

- Existing codebase: `slstatus/util.c`, `slstatus/components/battery.c`, `dmenu/Makefile`, `scripts/*` — patterns verified by direct inspection
- [clipnotify source](https://github.com/cdown/clipnotify) — reference C implementation for XFixes clipboard monitoring (~80 lines)
- [sd-bus official docs](https://www.freedesktop.org/software/systemd/man/latest/sd-bus.html) — systemd D-Bus client API
- [power-profiles-daemon D-Bus API](https://freedesktop-team.pages.debian.net/power-profiles-daemon/gdbus-org.freedesktop.UPower.PowerProfiles.html) — verified via busctl introspect on this system
- [Desktop Notifications Specification](https://specifications.freedesktop.org/notification/notification-spec-latest.html) — Notify method signature
- [Linux power supply sysfs](https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-class-power) — battery sysfs ABI
- [signal-safety(7) Linux man page](https://man7.org/linux/man-pages/man7/signal-safety.7.html) — async-signal-safe function list
- [CERT C ENV33-C](https://wiki.sei.cmu.edu/confluence/pages/viewpage.action?pageId=87152177) — security rationale for exec over system()

### Secondary (MEDIUM confidence)

- [scm - Simple clipboard manager](https://github.com/thimc/scm) — suckless-style C clipboard manager; flock pattern reference; omits SelectionRequest (known gap)
- [xclipd - X11 clipboard daemon](https://github.com/jhunt/xclipd) — reference for full clipboard daemon with XFixes
- [lowbat - Minimal battery monitor daemon](https://www.tjkeller.xyz/projects/lowbat/) — C battery daemon, ~150 lines
- [betterlockscreen DISPLAY issue #291](https://github.com/betterlockscreen/betterlockscreen/issues/291) — documented DISPLAY detection failure
- [X11 clipboard management foibles](https://jameshunt.us/writings/x11-clipboard-management-foibles/) — clipboard loss on application exit

### Tertiary (informational)

- [clipmenu](https://github.com/cdown/clipmenu) — shell-based clipboard manager; feature set reference only
- [powernotd](https://github.com/Laeri/powernotd) — Rust battery daemon; multi-threshold pattern reference
- [dlogout session manager reference](https://dwm.suckless.narkive.com/lAj0CWrE/dev-dlogout-i-wrote-a-basic-logout-shutdown-menu-for-use-with) — C session manager reference

---
*Research completed: 2026-04-09*
*Ready for roadmap: yes*
