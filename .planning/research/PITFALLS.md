# Pitfalls Research

**Domain:** Suckless C desktop utilities (X11, process spawning, sysfs, dmenu integration)
**Researched:** 2026-04-09
**Confidence:** HIGH (verified against existing suckless source, kernel docs, X11 protocol docs)

## Critical Pitfalls

### Pitfall 1: X11 clipboard daemon must handle SelectionRequest to actually own data

**What goes wrong:**
A clipboard daemon that only *monitors* clipboard changes via XFixes (XFixesSelectionNotifyEvent) but never *claims ownership* and *responds to SelectionRequest* events cannot persist clipboard data across application exits. When the original copying application closes, the clipboard data vanishes. The current shell-based dmenu-clipd avoids this by shelling out to xclip (which handles the protocol), but a pure C replacement that only watches events without serving data will silently lose clipboard contents when source applications close.

**Why it happens:**
X11 clipboard is not a buffer -- it is a broker protocol. The X server holds no data. When app B wants to paste, the server asks the current selection owner (app A) to convert and send the data. If app A is gone and no clipboard manager has claimed ownership, the data is gone. Developers used to Win32/macOS clipboard semantics expect a centralized buffer and skip the ownership/serving half of the protocol.

**How to avoid:**
The clipboard daemon (dmenu-clipd in C) must:
1. Watch for XFixesSelectionNotify events to detect ownership changes
2. Request clipboard content from the current owner via XConvertSelection()
3. Store the received data locally (file cache under ~/.cache/dmenu-clipboard/)
4. Claim selection ownership with XSetSelectionOwner()
5. Respond to SelectionRequest events by writing data to the requestor's property via XChangeProperty() and sending SelectionNotify back
6. Support the TARGETS atom (return UTF8_STRING, STRING at minimum) -- ICCCM mandates this and clipboard managers that skip it break interop with many apps

**Warning signs:**
- Clipboard empties after closing the app that copied
- Paste fails in apps that first query TARGETS before requesting data
- xclip -o returns empty after source app exits

**Phase to address:**
Clipboard daemon conversion phase (dmenu-clipd to C). This is the single most complex utility in the project.

---

### Pitfall 2: Zombie processes from fire-and-forget child spawning

**What goes wrong:**
Every utility spawns external processes: betterlockscreen, powerprofilesctl, maim, xclip, systemctl, dunstify. Using fork()+exec() without waitpid() creates zombie processes. Using system()/popen() without checking exit status and properly closing creates zombies and hides failures. In a session that runs for days, zombie accumulation from the clipboard daemon alone (spawning on every clipboard event) can exhaust PID space.

**Why it happens:**
Shell scripts handle child reaping automatically (the shell calls wait()). In C, the programmer is responsible. The suckless ethos of minimal code tempts developers to skip error handling on child processes. The dmenu-session utility spawns pkill, systemctl, betterlockscreen -- if any of these are launched via fork() without a wait path, they become zombies.

**How to avoid:**
Two patterns depending on whether you need the child's exit status:

*Need exit status (dmenu-cpupower checking if powerprofilesctl succeeded):*
```c
pid_t pid = fork();
if (pid == 0) { execvp(cmd, args); _exit(127); }
if (pid > 0) waitpid(pid, &status, 0);
```

*Fire-and-forget (betterlockscreen, dunstify):*
Install a SIGCHLD handler that reaps in a loop:
```c
static void sigchld(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}
/* In main: */
struct sigaction sa = { .sa_handler = sigchld, .sa_flags = SA_RESTART | SA_NOCLDSTOP };
sigaction(SIGCHLD, &sa, NULL);
```

Never call malloc(), printf(), or other non-async-signal-safe functions inside the SIGCHLD handler. Only waitpid() and write() are safe there.

**Warning signs:**
- `ps aux | grep Z` shows zombie (defunct) processes
- PID numbers climbing unusually fast
- Child process failures silently ignored

**Phase to address:**
Every phase that converts a script to C. Establish the spawn+reap pattern in the first utility converted, then reuse it.

---

### Pitfall 3: Bidirectional pipe deadlock with dmenu

**What goes wrong:**
Three utilities (dmenu-clip, dmenu-cpupower, dmenu-session) need to write a menu to dmenu's stdin and read the user's selection from dmenu's stdout. Using a single popen() call only gives you one direction. Attempting bidirectional communication with two pipes in a single process causes deadlock: the parent blocks writing to dmenu's stdin (pipe buffer full, dmenu hasn't read yet because it's waiting for EOF), while dmenu blocks writing to stdout (pipe buffer full, parent hasn't read yet because it's still writing).

**Why it happens:**
popen() opens a unidirectional pipe. The shell scripts use `printf ... | dmenu` which lets the shell handle pipe management. In C, you must manage both pipes yourself. If you write more than ~64KB to dmenu's stdin before reading its stdout, the kernel pipe buffer fills and both sides deadlock.

**How to avoid:**
For these utilities, the menu data is always small (3-10 items). The safe pattern:
1. fork()
2. In child: dup2 pipe read end to stdin, dup2 pipe write end to stdout, execvp("dmenu", ...)
3. In parent: write all menu items to the write pipe, close the write end (signals EOF to dmenu), then read from the read pipe

This works because the menu data is tiny (under 1KB). For dmenu-clip with longer previews, ensure the total data stays well under the pipe buffer size (64KB on Linux), or write and close before reading.

Alternatively, use popen("dmenu ...", "w") for the write side but that loses stdout. The cleanest suckless pattern is the fork+dup2+exec approach.

**Warning signs:**
- Utility hangs when launched, dmenu never appears
- Works with 3 items but hangs with 50
- strace shows both processes in write() syscall

**Phase to address:**
First dmenu-interactive utility conversion (dmenu-session is simplest, do it first to establish the pattern).

---

### Pitfall 4: DISPLAY environment variable not available in spawned context

**What goes wrong:**
betterlockscreen requires DISPLAY to connect to the X server. If dmenu-session (in C) spawns betterlockscreen without ensuring DISPLAY is set in the child's environment, or if the utility is somehow launched from a context where DISPLAY is not inherited (e.g., a keybinding daemon that sanitizes environment), the lock screen silently fails. The existing PROJECT.md even notes "DISPLAY export" as a specific concern for betterlockscreen.

**Why it happens:**
Shell scripts inherit the entire environment automatically. C programs using execvp() also inherit the environment, but execve() with a custom envp array does not. Additionally, some process launchers (systemd, dbus-activated services) strip environment variables. The betterlockscreen GitHub issue #291 documents exactly this failure mode.

**How to avoid:**
- Use execvp() (not execve() with a custom environment) so the child inherits the parent's full environment including DISPLAY
- Before exec, verify DISPLAY is set: `if (!getenv("DISPLAY")) { /* error */ }`
- For betterlockscreen specifically, the C wrapper should validate DISPLAY exists before fork+exec, and die with a clear error rather than silently failing
- In dwm-start (which stays shell), ensure DISPLAY is exported before launching any C utilities

**Warning signs:**
- Lock screen command returns immediately without locking
- betterlockscreen logs "cannot open display" to stderr (but stderr might not be visible from a dwm keybinding)
- Works from terminal but not from dwm keybinding

**Phase to address:**
dmenu-session conversion phase (where betterlockscreen is spawned). Also relevant for screenshot-notify (maim needs DISPLAY).

---

### Pitfall 5: Signal handler async-signal-safety violations

**What goes wrong:**
A SIGCHLD handler that calls printf(), malloc(), syslog(), or any non-async-signal-safe function can corrupt memory, deadlock on internal locks, or cause undefined behavior. This is especially dangerous in the clipboard daemon which runs indefinitely and receives SIGCHLD frequently (from spawned processes if any), SIGTERM for shutdown, and potentially SIGUSR1 for refresh.

**Why it happens:**
Signal handlers interrupt the main program at arbitrary points. If main() is inside malloc() when a signal fires and the handler also calls malloc(), the heap's internal data structures get corrupted. printf() holds an internal lock -- if interrupted while holding it, a printf() in the handler deadlocks. C programmers who are competent but not systems programmers routinely put logging in signal handlers.

**How to avoid:**
- Signal handlers should only set a volatile sig_atomic_t flag and return
- The main event loop checks the flag and does the actual work
- For SIGCHLD specifically: the waitpid() loop is the one exception -- waitpid() is async-signal-safe
- Follow the slstatus pattern already in the codebase: `static volatile sig_atomic_t done;` with a handler that just sets `done = 1`

**Warning signs:**
- Intermittent crashes that cannot be reproduced
- Deadlocks that only happen under load
- Valgrind/ASan reporting heap corruption in signal context

**Phase to address:**
Every phase. Establish the pattern in the first utility, copy it forward.

---

### Pitfall 6: Sysfs battery reads returning stale or partial data

**What goes wrong:**
Reading /sys/class/power_supply/BAT0/capacity with fopen+fscanf works most of the time, but sysfs files are virtual -- the kernel generates their content on read. If you open the file, read it, then read it again without closing and reopening, you get the same stale data because the file position is at EOF. Additionally, some battery attributes (energy_now, power_now) can momentarily read as 0 during ACPI updates, causing divide-by-zero or false "battery dead" alerts.

**Why it happens:**
Sysfs is not a real filesystem. Each read triggers a kernel callback. But once you've consumed the file's virtual content, subsequent reads without seeking back to position 0 return nothing. The slstatus battery.c code handles this correctly by using pscanf() which opens, reads, and closes the file each time. But a one-shot utility like battery-notify might cache a file descriptor and re-read it, getting stale data.

**How to avoid:**
- Follow the slstatus pattern: open, read, close for each sample. Do not keep sysfs file descriptors open across reads
- Check for 0 values on energy_now/power_now and treat them as transient errors rather than real values
- For battery-notify (one-shot): open /sys/class/power_supply/BAT0/capacity, read the integer, close immediately. If the value is 0 and the battery status is not "Full", retry once after a short delay
- Verify the battery device exists: check access("/sys/class/power_supply/BAT0", F_OK) and fail gracefully on desktops without batteries

**Warning signs:**
- Battery percentage always reads the same value
- False 0% readings triggering unnecessary notifications
- Works on first run but returns stale data on subsequent checks (if file descriptor reused)

**Phase to address:**
Battery-notify conversion phase.

---

### Pitfall 7: Not preventing duplicate daemon instances

**What goes wrong:**
The clipboard daemon (dmenu-clipd) runs persistently. If dwm-start is re-executed, or the user manually restarts it, two instances run simultaneously. Both compete for clipboard ownership, causing rapid oscillation of XSetSelectionOwner, infinite event storms, and CPU spikes. The existing shell script has no duplicate prevention either.

**Why it happens:**
Daemons need explicit single-instance enforcement. The shell script relies on the user running it once from dwm-start. But dwm-start has no guard, and "just restart it" is a common user action.

**How to avoid:**
Use a PID file with flock():
```c
int fd = open("/tmp/dmenu-clipd.lock", O_CREAT | O_RDWR, 0600);
if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
    die("dmenu-clipd: already running");
}
```
This is the pattern scm uses. flock() is atomic and automatically releases on process exit (even on crash). Do not use PID files with manual write/check -- they go stale on crashes.

**Warning signs:**
- Two dmenu-clipd processes visible in ps
- Clipboard history filling with duplicate entries
- High CPU usage from X11 event storm

**Phase to address:**
Clipboard daemon conversion phase.

---

## Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Using system() instead of fork+exec | 3 lines instead of 15 | Shell injection risk if any argument is user-derived; spawns /bin/sh for every call; cannot capture exit status reliably; CERT C ENV33-C violation | Never for utilities that may receive external input. Acceptable only for hardcoded commands like `system("systemctl poweroff")` where no variables are interpolated |
| Skipping INCR protocol in clipboard daemon | Avoids ~100 lines of chunked transfer code | Paste fails silently for data >256KB (large code blocks, base64 images) | Acceptable for MVP since clipboard history stores text previews only. Flag as known limitation |
| Hardcoding BAT0 battery path | Works on single-battery laptops | Breaks on multi-battery systems, desktops without batteries | Acceptable -- project targets single Arch Linux laptop. Document the assumption |
| Using popen() for dmenu interaction | Simpler than fork+dup2+exec | Only unidirectional; cannot write stdin AND read stdout in same popen call | Never for dmenu utilities. Use fork+dup2+exec from the start |
| Global mutable state (buf[] pattern from slstatus) | Matches existing suckless style, zero allocations | Not thread-safe, not reentrant | Always acceptable -- these are single-threaded utilities. Follow suckless convention |

## Integration Gotchas

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| betterlockscreen | Not checking if another instance is already running (--display 1 flag ignored, multiple lock screens stacked) | Check `pgrep -x betterlockscreen` or use a lock file before spawning. The PROJECT.md notes "duplicate prevention" as a requirement |
| powerprofilesctl | Calling `powerprofilesctl set performance` without checking if power-profiles-daemon is running, causing silent failure | Verify the service: `execvp("powerprofilesctl", {"powerprofilesctl", "list", NULL})` and check exit status before attempting `set`. Alternatively, check D-Bus service availability |
| dunst/dunstify | Calling notify_init() with an empty string or not calling it at all before notify_notification_show() -- triggers assertion failure in libnotify | Either shell out to dunstify (simpler, matches suckless philosophy of using existing tools) or call notify_init("program-name") with a non-empty string. Shelling out to `dunstify` avoids the libnotify dependency entirely |
| maim (screenshot) | Not handling maim's exit status -- maim returns non-zero if user cancels selection (presses Escape), but the notification fires anyway saying "Image copied to clipboard" | Check waitpid() status: only proceed to xclip + notification if WIFEXITED(status) && WEXITSTATUS(status) == 0 |
| xclip (clipboard set) | Forking xclip to set clipboard, but xclip forks itself to stay alive as selection owner. The parent's waitpid() waits forever because xclip's foreground process exited but the daemon child persists | Use `xclip -selection clipboard` which handles this internally. When waitpid() returns, the clipboard is set. Do NOT try to wait for xclip's background daemon |
| dmenu | Writing to dmenu's stdin pipe without sending a trailing newline on the last item -- dmenu ignores the last item if it lacks a newline terminator | Always terminate each menu item with '\n', including the last one |

## Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| Clipboard daemon polling (current shell approach) | 0.5s poll loop via xclip consumes constant CPU even when clipboard is idle. Over a full day: ~172,800 unnecessary xclip invocations | Use XFixes events -- zero CPU when clipboard is idle. This is the entire motivation for the C rewrite | Already broken (current shell implementation) |
| Opening X11 display connection per operation | Each XOpenDisplay() call is a round-trip to the X server. If a utility opens/closes the display for each clipboard check, latency adds up | Open the display once at startup, use it throughout, close on shutdown. The slstatus code does exactly this | Noticeable at >1 operation/second |
| Rebuilding clipboard preview list on every dmenu invocation | For 50 entries with 80-char previews, this means 50 file opens + reads + string operations | Cache the preview list in memory. Rebuild only when the cache directory mtime changes | Not a real problem at 50 entries, but the pattern matters for correctness |
| Blocking on XNextEvent in clipboard daemon | XNextEvent blocks the entire process. If you also need to handle signals (SIGTERM for graceful shutdown), a blocked XNextEvent will not return until the next X event | Use XPending() in a loop with a short timeout, or use ConnectionNumber(dpy) with select()/poll() to multiplex X11 events with signal handling via a self-pipe | Shutdown takes arbitrarily long (until next clipboard change) |

## Security Mistakes

| Mistake | Risk | Prevention |
|---------|------|------------|
| Using system() with string concatenation for commands | Shell injection if any component of the command string comes from clipboard data, filenames, or user input. CERT C ENV33-C explicitly prohibits this | Use execvp() with an argv array. No shell involved, no injection possible. For all utilities in this project, command arguments are either hardcoded constants or from a fixed dmenu selection list |
| Installing compiled utilities as SUID root for powerprofilesctl | Gives root to the entire binary. Any vulnerability in the utility = root exploit | powerprofilesctl does NOT need root -- it talks to power-profiles-daemon over D-Bus. The old cpupower approach needed pkexec because cpupower modifies kernel state directly. With powerprofilesctl, no privilege escalation is needed at all |
| Storing clipboard history in world-readable files | Passwords copied from password managers end up in plaintext files readable by any user | Create cache directory with mode 0700: `mkdir -p` with `chmod 700`. The existing shell script does not set restrictive permissions |
| Not sanitizing clipboard content before writing to files | Clipboard could contain path traversal characters in the hash-based filename scheme. MD5 hashes are safe, but if the filename scheme changes to content-based, "../" in content could escape the cache directory | Keep using hash-based filenames (md5sum of content). Never use clipboard content directly as a filename or path component |

## UX Pitfalls

| Pitfall | User Impact | Better Approach |
|---------|-------------|-----------------|
| Lock screen fails silently (DISPLAY not set) | User thinks screen is locked, walks away from unlocked machine | Before spawning betterlockscreen, verify DISPLAY is set and betterlockscreen binary exists. On failure, fall back to slock (which is already installed) or show a dunst error notification |
| Screenshot notification fires even when user cancelled | User presses Escape to cancel area selection, still gets "Image copied to clipboard" notification | Check maim's exit code before proceeding to notification |
| CPU profile change shows no confirmation | User selects "performance" in dmenu but gets no feedback that it worked | After powerprofilesctl set, read back the active profile with powerprofilesctl get, compare to requested, and show dunst notification confirming the change |
| Battery notification fires repeatedly | If battery-notify runs every minute via cron/timer and battery stays at 19%, user gets notification every minute | Track notification state: write a flag file after alerting, clear it when battery rises above threshold. Only notify on the transition from >20% to <=20%, not on every check |
| dmenu-clip shows truncated previews that all look identical | Multiple clipboard entries with same first 80 characters (e.g., similar code blocks) look identical in dmenu | Include a line number or index prefix: "1: preview text", "2: preview text" to disambiguate |

## "Looks Done But Isn't" Checklist

- [ ] **Clipboard daemon:** Responds to SelectionRequest (not just monitors) -- verify by copying in the daemon, closing the source app, and pasting in another app
- [ ] **Clipboard daemon:** Handles TARGETS requests -- verify by pasting into an app that queries TARGETS first (e.g., LibreOffice)
- [ ] **Clipboard daemon:** Single-instance enforcement with flock -- verify by launching twice, second instance should exit with error
- [ ] **dmenu-session lock:** DISPLAY passed to betterlockscreen -- verify by checking betterlockscreen's stderr output when launched from dwm keybinding, not terminal
- [ ] **dmenu-session lock:** Duplicate betterlockscreen prevention -- verify by pressing lock keybinding twice rapidly
- [ ] **battery-notify:** Handles missing battery gracefully -- verify by running on desktop without /sys/class/power_supply/BAT0
- [ ] **battery-notify:** Does not re-alert on same low battery state -- verify by running twice at 19% battery, should only notify once
- [ ] **screenshot-notify:** Cancelled selection (Escape) does not trigger notification -- verify by starting area select and pressing Escape
- [ ] **All utilities:** No zombie processes after 24h uptime -- verify with `ps aux | grep Z`
- [ ] **All utilities:** Graceful shutdown on SIGTERM -- verify clipboard daemon cleanup releases X selection ownership
- [ ] **Makefiles:** `make clean` does not remove installed binaries, only build artifacts -- verify PREFIX/bin still has the binary after `make clean`
- [ ] **install.sh:** Handles both fresh install (no existing binaries) and upgrade (binaries exist) -- verify by running install.sh twice

## Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Clipboard daemon not serving SelectionRequest | MEDIUM | Add SelectionRequest event handling to the event loop. Requires understanding X11 selection protocol, ~50-80 lines of code. Can be added without restructuring |
| Zombie process accumulation | LOW | Add SIGCHLD handler with waitpid loop. 5-line fix per utility. Alternatively, set `signal(SIGCHLD, SIG_IGN)` for fire-and-forget children (Linux-specific, auto-reaps) |
| dmenu pipe deadlock | LOW | Replace popen() with fork+dup2+exec pattern. Structural change but small code. Test with large menu item lists |
| DISPLAY not set for betterlockscreen | LOW | Add getenv("DISPLAY") check before fork+exec. 3-line fix. Add fallback to slock |
| Battery false-alert spam | LOW | Add state file tracking. ~10 lines. Write /tmp/battery-notified when alert fires, check for it before alerting, remove when charge >20% |
| Sysfs stale reads | LOW | Ensure open-read-close pattern for every sysfs read. Follow slstatus pscanf() pattern exactly |
| Duplicate daemon instances | LOW | Add flock() at startup. 5-line fix. Immediate and complete solution |

## Pitfall-to-Phase Mapping

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| Clipboard SelectionRequest handling | Clipboard daemon phase | Copy text, close source app, paste in different app succeeds |
| Zombie processes | First utility conversion (establish pattern) | Run utility for 24h, check `ps aux` for zombies |
| dmenu pipe deadlock | dmenu-session conversion (simplest dmenu utility) | Launch session menu, select option, verify no hangs |
| DISPLAY propagation | dmenu-session conversion (betterlockscreen spawn) | Lock from dwm keybinding, screen actually locks |
| Signal handler safety | First utility conversion (establish pattern) | Run under AddressSanitizer, no signal-related crashes |
| Sysfs stale reads | Battery-notify conversion | Read battery 10 times in a loop, values update correctly |
| Duplicate daemon instances | Clipboard daemon phase | Launch daemon twice, second exits with error message |
| Battery re-alert spam | Battery-notify conversion | Hold battery at 19%, only one notification in 10 minutes |
| Screenshot cancel handling | Screenshot-notify conversion | Press Escape during area select, no notification fires |
| Blocking XNextEvent | Clipboard daemon phase | Send SIGTERM to daemon, exits within 1 second |

## Sources

- [X11 clipboard protocol deep-dive](https://movq.de/blog/postings/2017-04-02/0/POSTING-en.html) -- comprehensive overview of selections, TARGETS, INCR, ownership pitfalls
- [Implementing copy/paste in X11](https://handmade.network/forums/articles/t/8544-implementing_copy_paste_in_x11) -- practical C implementation with gotcha documentation
- [X11 clipboard management foibles](https://jameshunt.us/writings/x11-clipboard-management-foibles/) -- clipboard loss on application exit, Docker edge case
- [scm (simple clipboard manager)](https://github.com/thimc/scm) -- suckless-style clipboard manager, demonstrates flock pattern, omits SelectionRequest handling
- [Reaping zombie processes in C](https://copyprogramming.com/howto/reaping-zombie-process) -- waitpid patterns, SIGCHLD handling
- [CERT C ENV33-C: Do not call system()](https://wiki.sei.cmu.edu/confluence/pages/viewpage.action?pageId=87152177) -- security rationale for exec over system()
- [Async-signal-safety (CS:APP)](https://csapp.cs.cmu.edu/2e/waside/waside-safety.pdf) -- definitive reference on what is safe in signal handlers
- [signal-safety(7) Linux man page](https://man7.org/linux/man-pages/man7/signal-safety.7.html) -- authoritative list of async-signal-safe functions
- [betterlockscreen DISPLAY issue #291](https://github.com/betterlockscreen/betterlockscreen/issues/291) -- documented DISPLAY detection failure in service context
- [Linux power supply sysfs documentation](https://docs.kernel.org/power/power_supply_class.html) -- kernel docs for battery attribute reading
- [sysfs rules](https://docs.kernel.org/admin-guide/sysfs-rules.html) -- kernel rules for reading sysfs attributes
- [dmenu non-blocking stdin patch](https://tools.suckless.org/dmenu/patches/non_blocking_stdin/) -- documents the stdin blocking behavior in dmenu
- [libnotify with dunst assertion failure](https://github.com/dunst-project/dunst/issues/768) -- empty appname causes crash
- Existing codebase: slstatus/slstatus.c (signal handling pattern), slstatus/components/battery.c (sysfs reading pattern), slstatus/util.c (error handling pattern)

---
*Pitfalls research for: suckless C desktop utilities*
*Researched: 2026-04-09*
