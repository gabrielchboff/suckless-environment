# Architecture Research

**Domain:** Suckless C desktop utilities (dmenu wrappers, daemons, notification tools)
**Researched:** 2026-04-09
**Confidence:** HIGH

## Standard Architecture

### System Overview

```
                         dwm (keybindings)
                              |
             spawn() via fork/exec from config.h
                              |
    +-----------+--------+----+-----+-----------+
    |           |        |          |           |
dmenu-session  |   dmenu-cpupower   |     screenshot-notify
    |       dmenu-clip       battery-notify
    |           |
    |     reads cache/
    |           ^
    |           | writes
    |       dmenu-clipd (daemon, started by dwm-start)
    |
    +--- dmenu (stdin/stdout pipe) --- user selection
    |
    +--- betterlockscreen | systemctl | pkill (exec)
    +--- powerprofilesctl (exec)
    +--- maim | xclip (exec via pipe)
    +--- notify-send (exec for dunst notifications)

External dependencies:
  X11 Xlib     -- dmenu-clipd (XFixes events), dmenu-clip (display)
  X11 XFixes   -- dmenu-clipd (clipboard change notifications)
  /sys/class/  -- battery-notify (sysfs reads)
  dunst        -- battery-notify, screenshot-notify (via notify-send exec)
  filesystem   -- dmenu-clipd/dmenu-clip (cache dir: ~/.cache/dmenu-clipboard/)
```

### Component Responsibilities

| Component | Responsibility | Integration Method |
|-----------|----------------|-------------------|
| dmenu-session | Present lock/logout/reboot/shutdown menu, confirm, execute | Pipe to dmenu, exec betterlockscreen/systemctl/pkill |
| dmenu-cpupower | Present power profiles, apply selection | Pipe to dmenu, exec powerprofilesctl |
| dmenu-clip | Display clipboard history, restore selected entry | Read cache dir, pipe to dmenu, exec xclip to restore |
| dmenu-clipd | Monitor X11 clipboard changes, persist to cache | X11 XFixes events, write to cache dir |
| battery-notify | One-shot battery check, alert if <=20% | Read sysfs, exec notify-send |
| screenshot-notify | Capture screen area, copy to clipboard, notify | Exec maim + xclip pipeline, exec notify-send |

### Component Classification

**Interactive (dmenu-based):** dmenu-session, dmenu-cpupower, dmenu-clip
- Short-lived processes launched by dwm keybinding
- Fork a pipe to dmenu, feed choices on stdin, read selection from stdout
- Execute an action based on selection, then exit

**Daemon:** dmenu-clipd
- Long-running process started by dwm-start
- Event loop monitoring X11 clipboard via XFixes
- Writes state to filesystem cache

**One-shot:** battery-notify, screenshot-notify
- Run once, perform action, exit
- No user interaction (screenshot-notify uses maim's own UI for selection)
- Triggered by dwm keybinding or cron/timer

## Recommended Project Structure

```
utils/
├── common/                    # Shared code (linked, not copied)
│   ├── util.h                 # die(), warn(), spawn_pipe(), spawn_exec()
│   ├── util.c                 # Implementation
│   ├── dmenu.h                # dmenu pipe helpers
│   └── dmenu.c                # open_dmenu(), dmenu_select(), close_dmenu()
├── dmenu-session/
│   ├── dmenu-session.c        # Single-file utility
│   ├── config.mk              # Suckless-style build config
│   └── Makefile               # Suckless-style Makefile
├── dmenu-cpupower/
│   ├── dmenu-cpupower.c
│   ├── config.mk
│   └── Makefile
├── dmenu-clip/
│   ├── dmenu-clip.c
│   ├── config.mk
│   └── Makefile
├── dmenu-clipd/
│   ├── dmenu-clipd.c
│   ├── config.mk
│   └── Makefile
├── battery-notify/
│   ├── battery-notify.c
│   ├── config.mk
│   └── Makefile
└── screenshot-notify/
    ├── screenshot-notify.c
    ├── config.mk
    └── Makefile
```

### Structure Rationale

- **common/:** Shared utility code compiled into each binary that needs it. Follows the suckless pattern where dwm and dmenu both carry their own util.c, but here we avoid duplication by having each Makefile reference `../common/util.o`. This is the one deviation from strict suckless "copy everything" -- justified because all six utilities live in the same repo and evolve together.

- **Per-utility directories:** Each utility is self-contained with its own Makefile and config.mk exactly matching the suckless pattern (see slstatus/Makefile). The Makefile includes config.mk, defines CFLAGS/LDFLAGS, and has `all:`, `clean:`, `install:` targets.

- **No config.h per utility:** Unlike dwm/dmenu/slstatus, these utilities have minimal configuration. Hardcode sensible defaults directly in the .c file using `#define` at the top. If a value might change per-user (like battery device name "BAT1"), use a `#define` with a sensible default that can be overridden at compile time via `-DBATTERY_NAME=\"BAT0\"` in config.mk.

## Architectural Patterns

### Pattern 1: dmenu Pipe Pattern (Interactive Utilities)

**What:** Fork a child process running dmenu, pipe choices to its stdin, read selection from its stdout. This is the C equivalent of `printf 'a\nb\nc\n' | dmenu -p "prompt"`.

**When to use:** dmenu-session, dmenu-cpupower, dmenu-clip -- any utility that presents a menu.

**Trade-offs:** Slightly more complex than system("echo ... | dmenu"), but avoids shell injection, gives direct control over error handling, and eliminates shell interpreter dependency.

**Example:**
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

/*
 * Open a dmenu instance with the given prompt and extra args.
 * Returns a FILE* for writing choices (caller writes, then calls
 * dmenu_read to get the selection).
 *
 * Internally: fork, pipe stdin to dmenu, pipe stdout back.
 */
static pid_t
dmenu_open(FILE **write_end, FILE **read_end,
           const char *prompt, char *const argv[])
{
	int to_dmenu[2], from_dmenu[2];
	pid_t pid;

	if (pipe(to_dmenu) < 0 || pipe(from_dmenu) < 0)
		die("pipe:");

	pid = fork();
	if (pid < 0)
		die("fork:");

	if (pid == 0) {
		/* child: dmenu process */
		close(to_dmenu[1]);
		close(from_dmenu[0]);
		dup2(to_dmenu[0], STDIN_FILENO);
		dup2(from_dmenu[1], STDOUT_FILENO);
		close(to_dmenu[0]);
		close(from_dmenu[1]);
		execvp("dmenu", argv);
		die("execvp dmenu:");
	}

	/* parent */
	close(to_dmenu[0]);
	close(from_dmenu[1]);
	*write_end = fdopen(to_dmenu[1], "w");
	*read_end = fdopen(from_dmenu[0], "r");
	return pid;
}
```

### Pattern 2: Exec-Only Action Pattern (One-Shot Utilities)

**What:** After determining what to do, replace the process with exec (or fork+exec) to run the target command. No shell involved.

**When to use:** All action execution -- betterlockscreen, systemctl, powerprofilesctl, maim, notify-send, xclip.

**Trade-offs:** More code than system(), but eliminates shell as attack surface and dependency.

**Example:**
```c
static void
exec_cmd(const char *const argv[])
{
	pid_t pid = fork();
	if (pid < 0)
		die("fork:");
	if (pid == 0) {
		setsid();
		execvp(argv[0], (char *const *)argv);
		die("execvp '%s':", argv[0]);
	}
	/* parent does not wait -- fire and forget for
	   betterlockscreen, notify-send, etc. */
}
```

For commands where we need to wait and check the result (powerprofilesctl set):
```c
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
```

### Pattern 3: X11 XFixes Event-Driven Clipboard Monitoring

**What:** Use XFixesSelectSelectionInput to receive XFixesSelectionNotify events when the CLIPBOARD selection owner changes. Blocks on XNextEvent -- zero CPU when idle. Reference implementation: clipnotify by cdown (github.com/cdown/clipnotify).

**When to use:** dmenu-clipd exclusively.

**Trade-offs:** Requires linking against libX11 and libXfixes (-lX11 -lXfixes). Adds X11 dependency to the daemon but eliminates the 0.5s polling loop that currently wastes CPU. The daemon blocks in XNextEvent and only wakes on actual clipboard changes.

**Event loop skeleton:**
```c
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xfixes.h>

static Display *dpy;
static int xfixes_event_base;

static void
setup_clipboard_watch(void)
{
	int evt, err;
	Window win;

	dpy = XOpenDisplay(NULL);
	if (!dpy)
		die("XOpenDisplay:");

	if (!XFixesQueryExtension(dpy, &evt, &err))
		die("XFixes not available");
	xfixes_event_base = evt;

	win = DefaultRootWindow(dpy);
	/* Could also create an unmapped window */

	XFixesSelectSelectionInput(dpy, win,
		XInternAtom(dpy, "CLIPBOARD", False),
		XFixesSetSelectionOwnerNotifyMask);
}

/* Main loop */
for (;;) {
	XEvent ev;
	XNextEvent(dpy, &ev);  /* blocks until clipboard changes */

	if (ev.type == xfixes_event_base + XFixesSelectionNotify) {
		/* Clipboard changed -- retrieve content and store */
		retrieve_and_cache_clipboard();
	}
}
```

### Pattern 4: sysfs Direct Read (No External Commands)

**What:** Read battery status directly from /sys/class/power_supply/BAT*/capacity and /sys/class/power_supply/BAT*/status. Identical to what slstatus/components/battery.c already does.

**When to use:** battery-notify.

**Trade-offs:** Linux-specific (acceptable per project constraints). Zero dependencies. Uses the same pscanf() pattern from slstatus util.c.

```c
#define BATTERY_CAPACITY "/sys/class/power_supply/%s/capacity"
#define BATTERY_STATUS   "/sys/class/power_supply/%s/status"
#define BATTERY_NAME     "BAT1"  /* overridable via config.mk */
#define LOW_THRESHOLD    20

static int
battery_low(void)
{
	int cap;
	char path[PATH_MAX], status[16];

	snprintf(path, sizeof(path), BATTERY_CAPACITY, BATTERY_NAME);
	/* pscanf from common/util.c */
	if (pscanf(path, "%d", &cap) != 1)
		return 0;  /* can't read = don't alert */

	snprintf(path, sizeof(path), BATTERY_STATUS, BATTERY_NAME);
	if (pscanf(path, "%15s", status) != 1)
		return 0;

	/* Only alert when discharging and low */
	return (cap <= LOW_THRESHOLD && strcmp(status, "Discharging") == 0);
}
```

## Data Flow

### dmenu-session Flow

```
User presses Ctrl+Alt+Delete
    |
dwm spawn() --> fork/exec dmenu-session
    |
dmenu-session opens pipe to dmenu
    |
writes "lock\nlogout\nreboot\nshutdown\n" to pipe
    |
dmenu displays menu --> user picks "lock"
    |
dmenu-session reads "lock" from pipe
    |
opens SECOND pipe to dmenu (confirmation: "no\nyes")
    |
user confirms "yes"
    |
fork/exec: betterlockscreen --lock blur --display 1
    |
dmenu-session exits
```

### dmenu-clipd Flow (Event-Driven)

```
dwm-start launches dmenu-clipd in background
    |
dmenu-clipd opens X11 display, registers XFixes on CLIPBOARD
    |
    +--- EVENT LOOP (blocks in XNextEvent, zero CPU) ---+
    |                                                     |
    |  XFixesSelectionNotify received                     |
    |       |                                             |
    |  XConvertSelection(CLIPBOARD, UTF8_STRING)          |
    |       |                                             |
    |  SelectionNotify received                           |
    |       |                                             |
    |  XGetWindowProperty --> content bytes               |
    |       |                                             |
    |  MD5 hash content --> filename                      |
    |       |                                             |
    |  Write to ~/.cache/dmenu-clipboard/<hash>           |
    |       |                                             |
    |  Prune if > MAX_ENTRIES (delete oldest by mtime)    |
    |       |                                             |
    +--- loop back to XNextEvent ---+
```

### dmenu-clip Flow

```
User presses Super+V
    |
dwm spawn() --> fork/exec dmenu-clip
    |
opendir(~/.cache/dmenu-clipboard/)
    |
stat() each file, sort by mtime descending
    |
read first 80 bytes of each file as preview
    |
pipe previews to dmenu stdin
    |
dmenu shows list --> user picks entry
    |
match preview back to hash filename
    |
read full content from cache file
    |
fork/exec: xclip -selection clipboard
    write content to xclip's stdin
    |
dmenu-clip exits (clipboard now has restored entry)
```

### battery-notify Flow

```
Timer/cron fires (or dwm-start runs on login)
    |
battery-notify reads /sys/class/power_supply/BAT1/capacity
    |
reads /sys/class/power_supply/BAT1/status
    |
capacity <= 20 AND status == "Discharging"?
    |
YES --> fork/exec: notify-send -u critical "Battery Low" "20% remaining"
    |
exit 0 (or exit 1 if no alert needed -- for cron/timer conditional use)
```

### screenshot-notify Flow

```
User presses Print
    |
dwm spawn() --> fork/exec screenshot-notify
    |
fork/exec pipeline:
    maim --select --format=png  (child 1, stdout to pipe)
        |
    xclip -selection clipboard -t image/png  (child 2, stdin from pipe)
    |
wait for pipeline to complete
    |
exit status 0? (user completed selection, not cancelled)
    |
YES --> fork/exec: notify-send "Screenshot" "Image copied to clipboard"
    |
exit 0
```

### Key Data Flows

1. **Clipboard persistence flow:** X11 CLIPBOARD selection --> dmenu-clipd (XFixes event) --> filesystem cache --> dmenu-clip (user trigger) --> xclip --> X11 CLIPBOARD. The filesystem cache at `~/.cache/dmenu-clipboard/` is the communication channel between the daemon and the picker.

2. **dmenu interaction flow:** All three interactive utilities follow the same pattern: C program --> pipe(2) --> fork(2) --> execvp("dmenu") --> read selection --> act on it. The dmenu binary is always invoked as a child process with args passed from dwm's config.h (forwarded via argv).

3. **Notification flow:** battery-notify and screenshot-notify both use `notify-send` via fork/exec (not libnotify). This avoids linking against libnotify/glib/dbus, staying true to suckless minimalism. dunst picks up the notification via D-Bus, which notify-send handles internally.

## Shared Code Analysis

### What to Share (common/)

| Function | Used By | Origin |
|----------|---------|--------|
| `die(fmt, ...)` | All 6 utilities | slstatus/util.c |
| `warn(fmt, ...)` | All 6 utilities | slstatus/util.c |
| `pscanf(path, fmt, ...)` | battery-notify, dmenu-clipd (optional) | slstatus/util.c |
| `dmenu_open(prompt, argv)` | dmenu-session, dmenu-cpupower, dmenu-clip | New -- shared dmenu pipe logic |
| `dmenu_select(write, read)` | dmenu-session, dmenu-cpupower, dmenu-clip | New -- write choices, read selection |
| `exec_wait(argv)` | dmenu-session, dmenu-cpupower, screenshot-notify | New -- fork/exec/waitpid |
| `exec_detach(argv)` | dmenu-session (betterlockscreen), battery-notify, screenshot-notify | New -- fork/exec/no-wait |

### What NOT to Share

- **X11 / XFixes code:** Only dmenu-clipd needs this. Do not put it in common/.
- **Config defines:** Each utility has its own `#define` block at the top of its .c file. No shared config.h.
- **MD5 hashing:** Only dmenu-clipd needs content hashing. Implement inline or use a simple hash function (not openssl -- use a ~50-line public domain MD5 or even simpler: hash the first N bytes with djb2/fnv for dedup, full content comparison on collision).

### Build Linkage

Each Makefile references common objects explicitly:

```makefile
# Example: dmenu-session/Makefile
include config.mk

all: dmenu-session

dmenu-session: dmenu-session.o ../common/util.o ../common/dmenu.o
	$(CC) -o $@ $(LDFLAGS) $^ $(LDLIBS)

dmenu-session.o: dmenu-session.c ../common/util.h ../common/dmenu.h
	$(CC) -o $@ -c $(CPPFLAGS) $(CFLAGS) $<

../common/%.o: ../common/%.c ../common/%.h
	$(CC) -o $@ -c $(CPPFLAGS) $(CFLAGS) $<

clean:
	rm -f dmenu-session *.o

install: all
	mkdir -p "$(DESTDIR)$(PREFIX)/bin"
	cp -f dmenu-session "$(DESTDIR)$(PREFIX)/bin"
	chmod 755 "$(DESTDIR)$(PREFIX)/bin/dmenu-session"

uninstall:
	rm -f "$(DESTDIR)$(PREFIX)/bin/dmenu-session"
```

dmenu-clipd's Makefile additionally links -lX11 -lXfixes:
```makefile
LDLIBS = -lX11 -lXfixes
```

battery-notify and screenshot-notify need only common/util.o (no dmenu.o, no X11).

## Anti-Patterns

### Anti-Pattern 1: Using system() for Command Execution

**What people do:** `system("betterlockscreen --lock blur")` or `system("notify-send ...")`.
**Why it's wrong:** Spawns /bin/sh, introduces shell injection risk, adds shell interpreter as runtime dependency, slower, harder to handle errors.
**Do this instead:** fork/execvp with explicit argv arrays. Every command invocation should use the exec_wait() or exec_detach() helpers described above.

### Anti-Pattern 2: Linking libnotify/GLib for Notifications

**What people do:** `#include <libnotify/notify.h>`, link `-lnotify -lglib-2.0`, call `notify_init()`, `notify_notification_new()`, etc.
**Why it's wrong:** Pulls in GLib (~5MB), GObject, D-Bus client library as transitive dependencies. Violates suckless minimalism. notify-send is already installed (it comes with libnotify package) and handles all D-Bus plumbing.
**Do this instead:** Fork/exec `notify-send` with appropriate arguments. Two utilities need it (battery-notify, screenshot-notify), both one-shot -- the fork/exec overhead is irrelevant.

### Anti-Pattern 3: Shared Library (.so) for Common Code

**What people do:** Build common/ as libutils.so, install it, link dynamically.
**Why it's wrong:** Overkill for 5 small functions. Introduces shared library versioning, install-path management, and LD_LIBRARY_PATH concerns. Suckless tools statically compile everything.
**Do this instead:** Compile common/*.c into .o files, link statically into each binary. Each binary is fully self-contained.

### Anti-Pattern 4: Polling for Clipboard Changes

**What people do:** `while (1) { read_clipboard(); sleep(0.5); }` (current dmenu-clipd).
**Why it's wrong:** Wastes CPU waking every 500ms. On battery, this adds measurable power drain. Misses rapid clipboard changes between polls.
**Do this instead:** Use XFixesSelectionInput for event-driven clipboard monitoring. Zero CPU when clipboard is idle. Instant detection of changes.

### Anti-Pattern 5: Using popen() to Read Command Output

**What people do:** `FILE *f = popen("powerprofilesctl get", "r"); fgets(buf, ...); pclose(f);`
**Why it's wrong:** Spawns shell, same issues as system(). Also creates a dependency on the exact command output format.
**Do this instead:** For powerprofilesctl, fork/exec and read from a pipe directly. For battery status, read sysfs directly (no command needed at all).

## Integration Points

### External Commands (fork/exec)

| Command | Used By | Args Pattern | Wait? |
|---------|---------|-------------|-------|
| `dmenu` | session, cpupower, clip | `-p prompt -c -bw 1 -fn font ...` (from dwm argv passthrough) | Yes (read stdout) |
| `betterlockscreen` | session | `--lock blur --display 1` | No (detach) |
| `systemctl` | session | `reboot` or `poweroff` | No (system handles) |
| `pkill` | session (logout) | `-x slstatus`, `-x dmenu-clipd`, `-x dwm` | No |
| `powerprofilesctl` | cpupower | `get` (read), `set profile-name` (write) | Yes (check exit) |
| `xclip` | clip, screenshot-notify | `-selection clipboard [-t image/png]` | Yes (pipe data) |
| `maim` | screenshot-notify | `--select --format=png` | Yes (pipe output) |
| `notify-send` | battery-notify, screenshot-notify | `-u urgency "title" "body"` | No (fire and forget) |

### Internal Boundaries

| Boundary | Communication | Notes |
|----------|---------------|-------|
| dmenu-clipd <-> dmenu-clip | Filesystem (cache dir) | No direct IPC. Clip reads whatever clipd has written. Race-free because clipd writes atomically (write tmp, rename). |
| All utilities <-> dwm | Process spawn (fork/exec from dwm's spawn()) | dwm passes dmenu styling args via config.h command arrays. Utilities receive these as argv and forward to dmenu. |
| dmenu-clipd <-> X11 | XFixes events, XConvertSelection | Daemon holds open Display* connection for lifetime. |
| battery-notify <-> kernel | sysfs reads | /sys/class/power_supply/BAT*/capacity, status |
| All utilities <-> dunst | D-Bus (via notify-send) | No direct dunst integration. notify-send handles D-Bus. |

### argv Passthrough Design

dwm's config.h defines command arrays like:
```c
static const char *sessioncmd[] = {
    "dmenu-session", "-c", "-bw", "1", "-fn", dmenufont,
    "-nb", col_bg, "-nf", col_fg, "-sb", col_sel, "-sf", col_selfg, NULL
};
```

The C utility receives these dmenu styling args in `argv` and must forward them to the dmenu child process. Implementation:

```c
int
main(int argc, char *argv[])
{
    /* argv[0] is our name, argv[1..] are dmenu args */
    char **dmenu_argv = argv; /* reuse -- replace argv[0] with "dmenu" */
    dmenu_argv[0] = "dmenu";
    /* Now dmenu_argv = {"dmenu", "-c", "-bw", "1", ...} */
    /* Pass to dmenu_open() */
}
```

This preserves the existing dwm config.h pattern without modification. Each utility acts as a transparent wrapper that adds `-p prompt -l N` before the user-supplied dmenu args.

## Build Order and Dependencies

### Dependency Graph

```
common/util.{c,h}  ──────────────────> ALL utilities need this
common/dmenu.{c,h}  ─────────────────> dmenu-session, dmenu-cpupower, dmenu-clip
                                        (NOT dmenu-clipd, battery-notify, screenshot-notify)
X11 + XFixes  ────────────────────────> dmenu-clipd only
```

### Recommended Build Order (Phase Sequencing)

Build common/ first, then utilities in order of increasing complexity:

1. **common/util.c** -- die(), warn(), pscanf(), exec_wait(), exec_detach()
   - No dependencies beyond libc
   - Foundation for everything else

2. **battery-notify** -- Simplest utility
   - Depends on: common/util only
   - Reads sysfs, execs notify-send. ~50 lines of logic.
   - Good first utility to validate the build system and patterns.

3. **screenshot-notify** -- Second simplest
   - Depends on: common/util only
   - Fork/exec pipeline (maim | xclip), then notify-send. ~40 lines of logic.
   - Validates the exec pipeline pattern.

4. **common/dmenu.c** -- dmenu pipe helpers
   - Depends on: common/util
   - Needed before any interactive utility.

5. **dmenu-session** -- First interactive utility
   - Depends on: common/util, common/dmenu
   - Two dmenu calls (menu + confirm), then exec action.
   - Validates the dmenu pipe pattern.

6. **dmenu-cpupower** -- Second interactive utility
   - Depends on: common/util, common/dmenu
   - One dmenu call, reads powerprofilesctl output, sets profile.
   - Validates reading command output via pipe.

7. **dmenu-clip** -- Third interactive utility
   - Depends on: common/util, common/dmenu
   - Directory listing, file I/O, dmenu pipe, xclip restore.
   - More complex file handling.

8. **dmenu-clipd** -- Most complex, daemon
   - Depends on: common/util, libX11, libXfixes
   - X11 event loop, clipboard retrieval, content hashing, file cache management.
   - Build last because it is the most architecturally distinct piece.

### install.sh Integration

The existing install.sh builds suckless tools with `cd dir && sudo make clean install`. The new utilities follow the same pattern:

```sh
# --- Build and install C utilities ---
echo "==> Building common utilities library"
cd "$REPO_DIR/utils/common" && make clean all

for util in battery-notify screenshot-notify dmenu-session dmenu-cpupower dmenu-clip dmenu-clipd; do
    echo "==> Building and installing $util"
    cd "$REPO_DIR/utils/$util" && sudo make clean install
done
```

No `sudo` needed for the build step (only for install to /usr/local/bin). Common objects are built first without install (they are not standalone binaries).

## Scaling Considerations

| Concern | Current (6 utils) | At 15+ utils | Mitigation |
|---------|-------------------|-------------|------------|
| Build time | Negligible (<2s total) | Still negligible -- each is ~1 .c file | No action needed |
| Common code growth | ~200 lines | Could grow to ~500 | Keep common/ focused: util + dmenu helpers only. No kitchen sink. |
| Clipboard cache size | 50 entries (~50 files) | Fine at 200+ | Already has pruning. Consider size-based limit (1MB total) if entries get large. |
| Binary size | ~20KB each | ~30KB with more features | Acceptable. Static linking keeps each binary self-contained. |

Scaling is not a concern for this project. These are tiny single-purpose utilities on a single-user desktop.

## Sources

- [clipnotify - X11 XFixes clipboard notification](https://github.com/cdown/clipnotify) -- reference C implementation for XFixes clipboard events (HIGH confidence)
- [xclipd - X11 clipboard daemon](https://github.com/jhunt/xclipd) -- reference for full clipboard daemon with XFixes (MEDIUM confidence)
- [powerprofilesctl(1) Arch man page](https://man.archlinux.org/man/extra/power-profiles-daemon/powerprofilesctl.1.en) -- CLI interface for power profile management (HIGH confidence)
- [betterlockscreen GitHub](https://github.com/betterlockscreen/betterlockscreen) -- lock screen command interface (HIGH confidence)
- [Linux kernel sysfs power_supply class](https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-class-power) -- battery sysfs interface (HIGH confidence)
- [libnotify notify-send source](https://github.com/GNOME/libnotify/blob/master/tools/notify-send.c) -- reference for why exec'ing notify-send is simpler than linking libnotify (MEDIUM confidence)
- Existing codebase: slstatus/util.c, slstatus/components/battery.c, dmenu/Makefile, scripts/* -- patterns to follow (HIGH confidence, direct observation)

---
*Architecture research for: suckless C desktop utilities*
*Researched: 2026-04-09*
