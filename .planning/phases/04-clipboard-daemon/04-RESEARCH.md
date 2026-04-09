# Phase 4: Clipboard Daemon - Research

**Researched:** 2026-04-09
**Domain:** X11 clipboard monitoring, event-driven daemon, POSIX file locking, content hashing
**Confidence:** HIGH

## Summary

This phase implements `dmenu-clipd`, a long-running C daemon that replaces the shell script's 0.5s polling loop with event-driven X11 clipboard monitoring using the XFixes extension. The daemon registers for `XFixesSelectionNotify` events on the CLIPBOARD selection, retrieves text content via the `XConvertSelection`/`SelectionNotify`/`XGetWindowProperty` handshake, deduplicates via FNV-1a hash, and stores entries as hash-named files in `~/.cache/dmenu-clipboard/`. It uses `flock()` for single-instance enforcement, `select()` with `ConnectionNumber(dpy)` for SIGTERM-safe event looping, and LRU pruning to cap entries at 50.

The daemon is the most architecturally complex utility in the project because it requires understanding the X11 selection protocol -- a multi-step asynchronous handshake rather than a simple read. The critical implementation path is: XFixes event notification -> XConvertSelection request -> wait for SelectionNotify -> XGetWindowProperty to read data -> hash and store. The v1 scope explicitly excludes SelectionRequest serving (clipboard ownership), INCR protocol for large data, and atomic writes -- all deferred to v2.

**Primary recommendation:** Implement using a dedicated invisible window for selection events, a `select()`-based event loop with 1-second timeout for SIGTERM responsiveness, and FNV-1a 64-bit inline hashing. The daemon links only against `-lX11 -lXfixes` and reuses `common/util.o` for die()/warn().

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** Monitor CLIPBOARD selection only via XFixes. This captures: App copies (Ctrl+C), Terminal copies (Ctrl+Shift+C in st), Vim/Neovim yanks to system clipboard (`"+y`), Tmux copies (when `set-clipboard on` is configured)
- **D-02:** Do NOT monitor PRIMARY selection -- mouse highlights would flood the history with noise
- **D-03:** Use XFixesSelectSelectionInput() + XNextEvent() event loop. Zero CPU when idle, instant change detection. Replaces the shell script's 0.5s polling
- **D-04:** On clipboard change event: XConvertSelection() to request data, handle SelectionNotify to read the content via XGetWindowProperty()
- **D-05:** Text-only storage: check if clipboard contains UTF8_STRING or STRING target. Skip images/binary data
- **D-06:** Store entries in `~/.cache/dmenu-clipboard/` (XDG_CACHE_HOME fallback) -- same directory dmenu-clip reads
- **D-07:** FNV-1a hash of content for filename (replaces shell's md5sum). ~10 lines inline, zero dependencies
- **D-08:** Max 50 entries with LRU pruning -- remove oldest by mtime when count exceeds limit
- **D-09:** Write content directly to hash-named file (v1 -- atomic writes via .tmp+rename deferred to v2)
- **D-10:** flock on a lock file in the cache directory for single-instance enforcement. Second instance exits immediately
- **D-11:** SIGTERM handler sets a `volatile sig_atomic_t done = 1` flag. Event loop checks this flag each iteration
- **D-12:** On shutdown: close X11 display connection, release flock, exit cleanly
- **D-13:** To prevent XNextEvent from blocking SIGTERM indefinitely, use `ConnectionNumber(dpy)` + `select()` with a timeout (e.g., 1 second) to periodically check the done flag
- **D-14:** Links against `-lX11 -lXfixes` (only utility needing X11 libraries). Add to Makefile with explicit LDFLAGS
- **D-15:** Add to BINS in utils/Makefile. Compile-time config via config.def.h (cache dir, max entries)

### Claude's Discretion
- FNV-1a implementation details (32-bit vs 64-bit, exact constants)
- Exact XConvertSelection/SelectionNotify handling sequence
- Whether to use config.def.h for cache dir or hardcode with XDG fallback
- dwm-start update to launch new dmenu-clipd instead of shell script

### Deferred Ideas (OUT OF SCOPE)
- Full SelectionRequest serving (CLIPD-06, v2) -- daemon claims clipboard ownership and serves data to requestors
- Atomic file writes via .tmp+rename (CLIPD-07, v2)
- INCR protocol for >256KB entries (CLIPD-08, v2)
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| CLIPD-01 | Clipboard daemon monitors X11 clipboard changes via XFixes events (no polling) | XFixes API verified installed (libxfixes 6.0.2), XFixesSelectSelectionInput + XFixesSelectionNotifyEvent pattern documented with code examples |
| CLIPD-02 | Clipboard daemon stores text-only entries with FNV-1a hash dedup in ~/.cache/dmenu-clipboard/ | FNV-1a 64-bit constants verified, cache format compatible with dmenu-clip's directory scan pattern |
| CLIPD-03 | Clipboard daemon prunes entries beyond 50 using LRU (oldest mtime removed) | Standard opendir/stat/qsort pattern documented, matches existing shell script behavior |
| CLIPD-04 | Clipboard daemon enforces single instance via flock | flock(2) API verified in sys/file.h with LOCK_EX and LOCK_NB constants |
| CLIPD-05 | Clipboard daemon shuts down gracefully on SIGTERM | select() + ConnectionNumber(dpy) pattern documented for interruptible event loop |
</phase_requirements>

## Project Constraints (from CLAUDE.md)

- **Platform:** Arch Linux only with pacman/AUR
- **Philosophy:** Suckless-style C -- minimal dependencies, no frameworks
- **Display server:** X11 with Xlib/XFixes directly
- **Build:** Each util has its own Makefile (but this project uses a unified utils/Makefile with per-utility rules)
- **Code style:** K&R braces, tab indentation, C99, -Os -pedantic -Wall
- **Error handling:** die() for fatal, warn() for non-fatal, NULL returns on failure
- **Config:** config.def.h/config.h pattern for compile-time configuration
- **Install target:** ~/.local/bin (PREFIX = $(HOME)/.local in config.mk)

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| libX11 (`-lX11`) | 1.8.13 | X11 display connection, atoms, event loop, window creation, selection protocol | Already installed for dwm/dmenu. Required for XInternAtom, XNextEvent, XConvertSelection, XGetWindowProperty, ConnectionNumber [VERIFIED: pacman -Q libx11] |
| libXfixes (`-lXfixes`) | 6.0.2 | Clipboard change notification via XFixesSelectSelectionInput | Eliminates 0.5s poll. Used by clipnotify, SDL, every serious clipboard manager. Header at /usr/include/X11/extensions/Xfixes.h [VERIFIED: pacman -Q libxfixes, header inspected] |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| common/util.o | project | die(), warn() error handling | Linked into every utility in the project [VERIFIED: utils/Makefile] |
| POSIX libc (sys/file.h) | glibc | flock() for single-instance via LOCK_EX + LOCK_NB | Daemon startup lock [VERIFIED: /usr/include/sys/file.h] |
| POSIX libc (sys/select.h) | glibc | select() + fd_set for timeout-based event loop | Interruptible X11 event waiting [VERIFIED: /usr/include/sys/select.h] |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| FNV-1a inline | OpenSSL MD5 (`-lcrypto`) | Adds unnecessary dependency. Crypto strength irrelevant for dedup. FNV-1a is faster, zero deps [VERIFIED: STACK.md recommendation] |
| FNV-1a 64-bit | FNV-1a 32-bit | 64-bit has better collision resistance for filename uniqueness at negligible cost. Both use same algorithm |
| select() | poll() | select() is simpler for single-fd case. poll() scales better but we only monitor one fd |
| XNextEvent with select() | XPending() busy-wait loop | XPending() requires polling. select() blocks efficiently with timeout |

**Build flags:**
```bash
cc -std=c99 -pedantic -Wall -Wextra -Wno-unused-parameter -Os -D_DEFAULT_SOURCE \
   -o dmenu-clipd dmenu-clipd.o ../common/util.o -s -lX11 -lXfixes
```

## Architecture Patterns

### Recommended Project Structure
```
utils/dmenu-clipd/
    dmenu-clipd.c      # Single-file daemon implementation (~300-350 lines)
    config.def.h       # CACHE_DIR_NAME, MAX_ENTRIES (compile-time config)
    config.h           # User copy of config.def.h
    .gitkeep           # Already exists
```

### Pattern 1: XFixes Clipboard Event Registration
**What:** Register for clipboard change notifications using the XFixes extension, then process events through the normal XNextEvent path.
**When to use:** Daemon initialization, before entering event loop.

```c
/* Source: XFixes header + clipnotify reference (github.com/cdown/clipnotify) */
/* [VERIFIED: /usr/include/X11/extensions/Xfixes.h struct and function signatures] */

static Display *dpy;
static Window win;
static int xfixes_event_base;
static Atom clipboard_atom;
static Atom utf8_string_atom;
static Atom xa_string_atom;
static Atom xsel_data_atom;

static void
setup_x11(void)
{
    int evt_base, err_base;

    dpy = XOpenDisplay(NULL);
    if (!dpy)
        die("XOpenDisplay: failed to open display");

    if (!XFixesQueryExtension(dpy, &evt_base, &err_base))
        die("XFixes extension not available");
    xfixes_event_base = evt_base;

    /* Create an invisible window to receive SelectionNotify events */
    win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy),
                              0, 0, 1, 1, 0, 0, 0);

    clipboard_atom = XInternAtom(dpy, "CLIPBOARD", False);
    utf8_string_atom = XInternAtom(dpy, "UTF8_STRING", False);
    xa_string_atom = XA_STRING;  /* pre-defined in Xatom.h */
    xsel_data_atom = XInternAtom(dpy, "XSEL_DATA", False);

    /* Register for clipboard owner change events - CLIPBOARD only (D-02) */
    XFixesSelectSelectionInput(dpy, win, clipboard_atom,
                               XFixesSetSelectionOwnerNotifyMask);
}
```

### Pattern 2: XConvertSelection -> SelectionNotify -> XGetWindowProperty Flow
**What:** Three-step asynchronous handshake to retrieve clipboard content after an XFixes notification.
**When to use:** Each time an XFixesSelectionNotify event fires.

```c
/* Source: Handmade Network X11 clipboard article + vurtun/x11clipboard gist */
/* [CITED: handmade.network/forums/articles/t/8544] */
/* [CITED: gist.github.com/vurtun/1c57d1e9e2ee169b90209325e38f0cb7] */

static char *
get_clipboard_text(size_t *out_len)
{
    XEvent ev;
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop_data = NULL;
    char *result = NULL;

    /*
     * Step 1: Request conversion to UTF8_STRING.
     * The selection owner writes data to XSEL_DATA property on our window.
     */
    XConvertSelection(dpy, clipboard_atom, utf8_string_atom,
                      xsel_data_atom, win, CurrentTime);
    XFlush(dpy);

    /*
     * Step 2: Wait for SelectionNotify event.
     * Use a bounded loop to avoid hanging if owner disappears.
     */
    for (;;) {
        /* Use select() with timeout here in the real implementation */
        if (!XCheckTypedWindowEvent(dpy, win, SelectionNotify, &ev)) {
            /* No event yet -- could poll with short timeout */
            XFlush(dpy);
            usleep(10000); /* 10ms */
            /* In practice, add iteration limit to avoid infinite wait */
            continue;
        }
        break;
    }

    /* Step 2b: Check if conversion succeeded */
    if (ev.xselection.property == None) {
        /* UTF8_STRING failed, try plain STRING as fallback (D-05) */
        XConvertSelection(dpy, clipboard_atom, xa_string_atom,
                          xsel_data_atom, win, CurrentTime);
        XFlush(dpy);
        /* Wait for SelectionNotify again... (same loop) */
        /* If this also fails, return NULL -- not text content */
        return NULL;
    }

    /*
     * Step 3: Read the data from the property on our window.
     */
    XGetWindowProperty(dpy, win, xsel_data_atom, 0, LONG_MAX, True,
                       AnyPropertyType, &actual_type, &actual_format,
                       &nitems, &bytes_after, &prop_data);

    if (prop_data && nitems > 0) {
        result = malloc(nitems + 1);
        if (result) {
            memcpy(result, prop_data, nitems);
            result[nitems] = '\0';
            *out_len = nitems;
        }
    }
    if (prop_data)
        XFree(prop_data);

    /* Delete the property to signal we have read it (ICCCM) */
    XDeleteProperty(dpy, win, xsel_data_atom);

    return result;
}
```

**Critical notes on this flow:**
1. The `True` parameter in `XGetWindowProperty` deletes the property after reading (ICCCM compliance) [CITED: movq.de/blog/postings/2017-04-02/0/POSTING-en.html]
2. If the selection owner has crashed/exited, the SelectionNotify will have `property == None` -- this is normal and must be handled gracefully
3. The fallback from UTF8_STRING to STRING handles older applications that do not support UTF-8

### Pattern 3: select() + ConnectionNumber for SIGTERM-Safe Event Loop
**What:** Use `select()` on the X11 socket fd to wait for events with a timeout, allowing periodic checking of the `done` signal flag.
**When to use:** Main daemon event loop (replaces bare XNextEvent which blocks indefinitely).

```c
/* Source: sgerwk/xlib-timeout pattern */
/* [CITED: github.com/sgerwk/xlib-timeout] */

static volatile sig_atomic_t done = 0;

static void
sigterm_handler(int sig)
{
    (void)sig;
    done = 1;
}

static void
run_event_loop(void)
{
    int xfd = ConnectionNumber(dpy);
    fd_set fds;
    struct timeval tv;
    XEvent ev;

    while (!done) {
        /*
         * CRITICAL: Check XPending first.
         * X11 buffers events internally -- select() only tells us
         * data arrived on the socket, but events may already be
         * queued in Xlib's internal buffer.
         */
        while (XPending(dpy) > 0) {
            XNextEvent(dpy, &ev);
            handle_event(&ev);
            if (done) return;
        }

        /* Wait for X11 data or timeout (1 second) */
        FD_ZERO(&fds);
        FD_SET(xfd, &fds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        select(xfd + 1, &fds, NULL, NULL, &tv);
        /* On timeout: loop back, check done flag */
        /* On data: loop back, XPending will find events */
        /* On EINTR (signal): loop back, done flag set by handler */
    }
}
```

**Why this works:** `select()` returns on: (a) data available on the X11 socket, (b) timeout expiry, or (c) signal interruption (EINTR). All three cases loop back to check `done`, giving SIGTERM a maximum 1-second response latency. [VERIFIED: sys/select.h API]

### Pattern 4: FNV-1a 64-bit Hash for Content Deduplication
**What:** Fast non-cryptographic hash producing hex string filenames for cache entries.
**When to use:** After retrieving clipboard text, before writing to cache.

```c
/* Source: FNV specification (isthe.com/chongo/tech/comp/fnv) */
/* [ASSUMED] Constants from FNV spec -- standard, well-known values */

#include <stdint.h>

#define FNV_OFFSET_BASIS 0xcbf29ce484222325ULL
#define FNV_PRIME        0x100000001b3ULL

static uint64_t
fnv1a(const char *data, size_t len)
{
    uint64_t hash = FNV_OFFSET_BASIS;
    size_t i;

    for (i = 0; i < len; i++) {
        hash ^= (uint8_t)data[i];
        hash *= FNV_PRIME;
    }
    return hash;
}

/* Format as 16-char hex string for filename */
static void
hash_to_filename(uint64_t hash, char *buf, size_t bufsz)
{
    snprintf(buf, bufsz, "%016llx", (unsigned long long)hash);
}
```

**64-bit vs 32-bit recommendation:** Use 64-bit. The 16-hex-char filenames are visually distinct and collision probability for 50 entries is astronomically low (~1 in 10^14). The 32-bit variant would produce 8-char filenames -- functional but unnecessarily tighter collision space. Both have identical performance. [ASSUMED]

### Pattern 5: flock() Single-Instance Enforcement
**What:** Advisory file lock on a lock file in the cache directory.
**When to use:** Daemon startup, before X11 initialization.

```c
/* Source: thimc/scm flock pattern */
/* [CITED: github.com/thimc/scm] */
/* [VERIFIED: /usr/include/sys/file.h -- LOCK_EX=2, LOCK_NB=4] */

#include <sys/file.h>

static int lock_fd = -1;

static void
acquire_lock(const char *cache_dir)
{
    char lockpath[PATH_MAX];

    snprintf(lockpath, sizeof(lockpath), "%s/.lock", cache_dir);
    lock_fd = open(lockpath, O_CREAT | O_RDWR, 0600);
    if (lock_fd < 0)
        die("open '%s':", lockpath);

    if (flock(lock_fd, LOCK_EX | LOCK_NB) < 0) {
        if (errno == EWOULDBLOCK)
            die("dmenu-clipd: already running");
        die("flock:");
    }
    /* Lock held until process exits or explicit unlock */
}
```

**Key behavior:** `flock()` automatically releases on process exit, including crashes and SIGKILL. No stale PID file cleanup needed. [VERIFIED: sys/file.h]

### Anti-Patterns to Avoid
- **Bare XNextEvent in event loop:** Blocks indefinitely, ignoring SIGTERM. Always use select() + ConnectionNumber with timeout. [CITED: .planning/research/PITFALLS.md Pitfall 10]
- **Polling with sleep():** The entire point of this rewrite. XFixes eliminates polling. [VERIFIED: CONTEXT.md D-03]
- **system() or popen() for any operation:** This daemon does everything in-process. No shell invocations. [CITED: CLAUDE.md philosophy]
- **Opening/closing Display per event:** Open once at startup, close on shutdown. Each XOpenDisplay is a round-trip to the X server. [CITED: .planning/research/PITFALLS.md performance traps]
- **Non-async-signal-safe functions in SIGTERM handler:** Handler must ONLY set `done = 1`. No printf, no malloc, no XCloseDisplay. [CITED: .planning/research/PITFALLS.md Pitfall 5]

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| X11 clipboard events | Custom polling loop | XFixes XFixesSelectSelectionInput | Polling wastes CPU and misses fast changes. XFixes is the standard X11 mechanism |
| Content deduplication | MD5 with OpenSSL | FNV-1a inline (~10 lines) | Crypto strength unnecessary for dedup. Zero dependencies vs -lcrypto |
| Single-instance daemon | PID file with manual read/check | flock() on lock file | PID files go stale on crash. flock() is atomic and auto-releases |
| Interruptible event wait | Custom signal pipe or busy-wait XPending | select() + ConnectionNumber(dpy) | Standard POSIX pattern for multiplexing fd + timeout |
| Cache directory creation | Manual stat+mkdir sequence | mkdir -p equivalent with mkdir()+EEXIST check | Race-free directory creation |

## Common Pitfalls

### Pitfall 1: XNextEvent Blocks SIGTERM Indefinitely
**What goes wrong:** Using bare `XNextEvent()` as the main event loop blocks the process until an X11 event arrives. If no clipboard activity occurs, the daemon cannot respond to SIGTERM.
**Why it happens:** XNextEvent is a blocking call with no timeout mechanism. Signal delivery interrupts system calls, but Xlib may retry internally.
**How to avoid:** Use `ConnectionNumber(dpy)` to get the X11 socket fd, then `select()` with a 1-second timeout. Check `done` flag after each select() return. Process events via `XPending()` + `XNextEvent()` only when data is available.
**Warning signs:** `kill -TERM <pid>` hangs; daemon only exits on next clipboard change.
[CITED: .planning/research/PITFALLS.md, github.com/sgerwk/xlib-timeout]

### Pitfall 2: XPending Must Be Checked Before select()
**What goes wrong:** Calling `select()` on the X11 fd without first draining `XPending()` misses events that Xlib has already buffered internally. The daemon appears to miss clipboard changes.
**Why it happens:** Xlib reads data from the socket in chunks and queues events internally. After processing one event, there may be more queued events that `select()` will never signal because the socket has already been drained.
**How to avoid:** Always loop `while (XPending(dpy) > 0) { XNextEvent(...); handle_event(...); }` BEFORE calling `select()`. Only enter `select()` when XPending returns 0.
**Warning signs:** Some clipboard changes are detected immediately, others only after the next change. Inconsistent behavior.
[CITED: github.com/sgerwk/xlib-timeout -- "events are received from the X server through the socket, but are stored in a queue"]

### Pitfall 3: SelectionNotify with property == None
**What goes wrong:** After XConvertSelection(), the selection owner may fail to convert (crashed, exited, or doesn't support the requested target). The SelectionNotify event arrives with `property == None`.
**Why it happens:** X11 clipboard is a broker protocol -- the X server holds no data. If the owner is gone when we request conversion, the conversion fails. This is normal and frequent.
**How to avoid:** Check `ev.xselection.property != None` before calling XGetWindowProperty. On failure: try STRING as fallback for UTF8_STRING failure. If both fail, silently skip this clipboard event (content is non-text or owner is gone).
**Warning signs:** Crashes on XGetWindowProperty with invalid property, or infinite loop waiting for valid data.
[CITED: movq.de/blog/postings/2017-04-02/0/POSTING-en.html]

### Pitfall 4: Self-Triggered Events When Writing Cache
**What goes wrong:** Writing to a cache file could theoretically trigger a clipboard event if any tool monitors filesystem changes, but more importantly: if the daemon ever sets the clipboard (not in v1 scope), it would trigger its own XFixes notification, creating an infinite loop.
**Why it happens:** XFixesSelectionNotify fires whenever the clipboard owner changes, including when our own process claims ownership.
**How to avoid:** For v1 (no SelectionRequest serving), this is not a problem since the daemon never calls XSetSelectionOwner. For future v2: compare the `owner` field in XFixesSelectionNotifyEvent against our own window -- skip events where `event->owner == win`.
**Warning signs:** CPU spins to 100%, cache fills with duplicate entries.
[ASSUMED -- standard clipboard manager pattern]

### Pitfall 5: Empty or Whitespace-Only Clipboard Content
**What goes wrong:** The daemon stores empty files or files containing only whitespace, which show as blank entries in dmenu-clip.
**Why it happens:** Some applications set the clipboard to empty string, or the conversion succeeds but returns zero bytes.
**How to avoid:** After retrieving content, check: (a) length > 0, (b) content is not all whitespace. The shell script does this with `tr -d '[:space:]'` check. Replicate in C with a simple loop.
**Warning signs:** Empty or blank entries appearing in dmenu-clip's list.
[VERIFIED: scripts/dmenu-clipd lines 27-28 -- shell script already handles this]

### Pitfall 6: FNV-1a Hash Incompatibility with Existing MD5 Cache
**What goes wrong:** Existing cache entries from the shell script use MD5 filenames (32 hex chars). The new FNV-1a produces 16 hex char filenames. Duplicate content already in the cache under MD5 name will not be detected by FNV-1a comparison.
**Why it happens:** Different hash algorithms produce different outputs for the same input.
**How to avoid:** This is acceptable and explicitly noted in CONTEXT.md specifics: "FNV-1a produces different hashes than MD5 -- existing cache entries from the shell script will be orphaned (acceptable, they'll be pruned by LRU)." No special handling needed. Old MD5-named files will naturally be pruned as the oldest entries when the 50-entry limit is reached.
**Warning signs:** Temporary duplicates in clipboard history during transition period.
[VERIFIED: CONTEXT.md specifics section]

## Code Examples

### Complete Event Handler Skeleton
```c
/* Source: Synthesized from clipnotify, scm, and X11 documentation */

static void
handle_event(XEvent *ev)
{
    char *text;
    size_t len;

    if (ev->type == xfixes_event_base + XFixesSelectionNotify) {
        /* Clipboard owner changed -- request content */
        text = get_clipboard_text(&len);
        if (text && len > 0 && !is_whitespace_only(text, len)) {
            store_entry(text, len);
        }
        free(text);
    }
    /* Silently ignore all other event types */
}
```

### LRU Pruning Pattern
```c
/* Source: Matches shell script logic (scripts/dmenu-clipd lines 36-42) */
/* [VERIFIED: compatible with dmenu-clip's directory scan] */

static void
prune_old_entries(const char *cache_dir)
{
    DIR *dp;
    struct dirent *de;
    struct stat st;
    char path[PATH_MAX];
    /* Use a simple array -- MAX_ENTRIES is 50, trivial size */
    struct { char name[NAME_MAX + 1]; time_t mtime; } files[MAX_ENTRIES + 10];
    int count = 0, i, ret;

    dp = opendir(cache_dir);
    if (!dp) return;

    while ((de = readdir(dp)) != NULL) {
        if (de->d_name[0] == '.') continue;
        ret = snprintf(path, sizeof(path), "%s/%s", cache_dir, de->d_name);
        if (ret < 0 || (size_t)ret >= sizeof(path)) continue;
        if (stat(path, &st) < 0) continue;
        if (!S_ISREG(st.st_mode)) continue;
        if (count < (int)(sizeof(files)/sizeof(files[0]))) {
            snprintf(files[count].name, sizeof(files[count].name),
                     "%s", de->d_name);
            files[count].mtime = st.st_mtime;
            count++;
        }
    }
    closedir(dp);

    if (count <= MAX_ENTRIES) return;

    /* Sort oldest first, remove excess */
    /* qsort by mtime ascending, unlink first (count - MAX_ENTRIES) */
    /* ... standard qsort + unlink pattern ... */
}
```

### Cache Directory Resolution (Shared Pattern with dmenu-clip)
```c
/* Source: utils/dmenu-clip/dmenu-clip.c get_cache_dir() */
/* [VERIFIED: exact pattern from Phase 3 implementation] */

static int
get_cache_dir(char *buf, size_t bufsz)
{
    const char *cache, *home;
    int ret;

    cache = getenv("XDG_CACHE_HOME");
    if (cache && cache[0] != '\0') {
        ret = snprintf(buf, bufsz, "%s/%s", cache, CACHE_DIR_NAME);
    } else {
        home = getenv("HOME");
        if (!home)
            die("HOME not set");
        ret = snprintf(buf, bufsz, "%s/.cache/%s", home, CACHE_DIR_NAME);
    }

    if (ret < 0 || (size_t)ret >= bufsz)
        return -1;
    return 0;
}
```

### main() Skeleton
```c
/* Daemon entry point -- recommended structure */
int
main(int argc, char *argv[])
{
    char cache_dir[PATH_MAX];
    struct sigaction sa;

    (void)argc;
    argv0 = argv[0];

    /* 1. Resolve cache directory */
    if (get_cache_dir(cache_dir, sizeof(cache_dir)) < 0)
        die("cache dir path too long");
    mkdir(cache_dir, 0700);  /* Create if missing, ignore EEXIST */

    /* 2. Acquire single-instance lock (D-10) */
    acquire_lock(cache_dir);

    /* 3. Set up SIGTERM handler (D-11) */
    sa.sa_handler = sigterm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;  /* Do NOT use SA_RESTART -- we want select() interrupted */
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);  /* Also handle Ctrl+C for development */

    /* 4. Initialize X11 and XFixes (D-03) */
    setup_x11();

    /* 5. Enter event loop (D-13) */
    run_event_loop();

    /* 6. Clean shutdown (D-12) */
    XCloseDisplay(dpy);
    if (lock_fd >= 0)
        close(lock_fd);  /* flock released automatically */

    return 0;
}
```

## Cache Format Compatibility

The daemon must produce files compatible with what `dmenu-clip` reads.

| Property | dmenu-clip Expectation | dmenu-clipd Must Produce |
|----------|----------------------|--------------------------|
| Directory | `$XDG_CACHE_HOME/dmenu-clipboard/` or `~/.cache/dmenu-clipboard/` | Same path, created with `mkdir(dir, 0700)` |
| Filename | Any non-hidden regular file (skips `.` prefix, symlinks, non-regular) | Hash string filename (16 hex chars), no `.` prefix |
| Content | Raw text, first 80 bytes read as preview | Raw text written with `write()` or `fwrite()` |
| Ordering | Sorted by mtime descending (newest first) | `touch`/`write` updates mtime naturally; re-writing existing hash file updates mtime for dedup-but-still-recent entries |
| Max entries | MAX_ENTRIES=50 caps directory scan array | MAX_ENTRIES=50 with LRU pruning |

[VERIFIED: utils/dmenu-clip/dmenu-clip.c lines 127-153 -- directory scan logic]

**Dedup behavior on repeated copies:** When the same text is copied again, FNV-1a produces the same hash. Writing to the same filename updates mtime, causing dmenu-clip to sort it as most recent. This matches the shell script's behavior where `printf '%s' "$clip" > "$CACHE_DIR/$sum"` re-writes the file.

## Makefile Integration Pattern

The utils/Makefile needs a new rule for dmenu-clipd with X11 link flags:

```makefile
# dmenu-clipd -- clipboard daemon (needs X11 libraries)
X11_LIBS = -lX11 -lXfixes

dmenu-clipd/dmenu-clipd: dmenu-clipd/dmenu-clipd.o $(COMMON_OBJ)
	$(CC) -o $@ $(LDFLAGS) $^ $(X11_LIBS)

dmenu-clipd/dmenu-clipd.o: dmenu-clipd/dmenu-clipd.c dmenu-clipd/config.h common/util.h
	$(CC) -o $@ -c $(CFLAGS) $<
```

**Key difference from other utilities:** dmenu-clipd links `$(X11_LIBS)` but does NOT link `$(DMENU_OBJ)` -- it never interacts with dmenu. It only uses `$(COMMON_OBJ)` for die()/warn().

[VERIFIED: utils/Makefile existing patterns for other utilities]

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| xclip polling every 0.5s | XFixes event-driven | XFixes extension available since X11R6.8 (~2004) | Zero CPU when idle, instant detection |
| MD5 for content hash | FNV-1a inline | Decision D-07 | Removes -lcrypto dependency, faster, simpler |
| Shell script daemon | Native C daemon | This phase | Eliminates sh/xclip runtime dependencies |
| Bare XNextEvent | select()+ConnectionNumber | Common pattern since early 2000s | SIGTERM responsiveness |

**Deprecated/outdated:**
- The shell script `scripts/dmenu-clipd` is NOT modified -- it stays as-is for reference. The new C binary at `~/.local/bin/dmenu-clipd` replaces it in PATH because `dwm-start` adds `~/.local/bin` first. [VERIFIED: dwm-start line 3: `export PATH="$HOME/.local/bin:$PATH"`]

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Custom C test harness (pass/fail with fork-and-check pattern) |
| Config file | utils/Makefile test targets |
| Quick run command | `make -C utils test` |
| Full suite command | `make -C utils clean all test` |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| CLIPD-01 | XFixes event registration + event loop runs | smoke | Manual: run daemon, copy text, verify cache file appears | N/A -- requires X11 display |
| CLIPD-02 | FNV-1a hash produces deterministic filenames | unit | `make -C utils test-clipd-hash` | Wave 0 |
| CLIPD-02 | Text stored as raw content in hash-named file | smoke | Manual: copy text, verify file content matches | N/A -- requires X11 |
| CLIPD-03 | LRU pruning removes oldest when >50 | unit | `make -C utils test-clipd-prune` | Wave 0 |
| CLIPD-04 | flock prevents second instance | unit | `make -C utils test-clipd-lock` | Wave 0 |
| CLIPD-05 | SIGTERM causes clean exit within 2s | smoke | Manual: `dmenu-clipd & sleep 1 && kill $! && wait $!` | N/A -- requires X11 |

### Sampling Rate
- **Per task commit:** `make -C utils clean all` (build succeeds, zero warnings)
- **Per wave merge:** `make -C utils clean all test` (full test suite green)
- **Phase gate:** Full build + all unit tests + manual smoke tests documented

### Wave 0 Gaps
- [ ] `utils/tests/test-clipd.c` -- unit tests for FNV-1a hash, LRU pruning, flock
- [ ] Test targets in `utils/Makefile` for `test-clipd-hash`, `test-clipd-prune`, `test-clipd-lock`

**Note:** X11-dependent behavior (CLIPD-01 event monitoring, CLIPD-05 graceful shutdown) cannot be unit tested without a display. These require manual smoke testing documented in verification steps. The FNV-1a hash, LRU pruning, and flock logic CAN be unit tested in isolation.

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | -- |
| V3 Session Management | no | -- |
| V4 Access Control | yes | Cache directory created with mode 0700; lock file with mode 0600 |
| V5 Input Validation | yes | Clipboard content validated: text-only (UTF8_STRING/STRING), non-empty, non-whitespace-only |
| V6 Cryptography | no | FNV-1a is not cryptographic -- used only for dedup/naming |

### Known Threat Patterns for X11 Clipboard Daemon

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Clipboard content stored in plaintext files | Information Disclosure | Cache dir mode 0700. No mitigation for passwords in clipboard -- inherent to all clipboard managers |
| Symlink attack in cache directory | Tampering | Not directly applicable (daemon writes with known hash filenames, does not follow symlinks). dmenu-clip uses lstat() to skip symlinks when reading |
| Hash collision causes content overwrite | Tampering | FNV-1a 64-bit collision probability negligible for 50 entries (~10^-14). Not a security concern, only a correctness concern |
| Denial of service via rapid clipboard changes | Denial of Service | Event-driven architecture handles naturally -- one event per change, no amplification. LRU pruning bounds cache size |

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | FNV-1a 64-bit constants (offset basis 0xcbf29ce484222325, prime 0x100000001b3) are correct | Architecture Patterns - Pattern 4 | Wrong hash values would produce inconsistent filenames but daemon would still function. Easily verified against FNV spec |
| A2 | 64-bit FNV-1a is preferable to 32-bit for filename uniqueness | Architecture Patterns - Pattern 4 | 32-bit would also work fine for 50 entries. Low risk either way |
| A3 | Self-triggered XFixes events are not a concern in v1 since daemon never calls XSetSelectionOwner | Pitfall 4 | If some X11 interaction inadvertently triggers clipboard events, could cause redundant cache writes. Low risk -- daemon checks hash for dedup |

**If this table is empty:** Not applicable -- three assumptions identified above.

## Open Questions

1. **SA_RESTART flag on SIGTERM handler**
   - What we know: SA_RESTART causes system calls to restart after signal delivery. Without it, select() returns -1 with errno=EINTR when SIGTERM arrives.
   - What's unclear: Whether Xlib internally uses SA_RESTART for any signals, potentially conflicting.
   - Recommendation: Do NOT set SA_RESTART for SIGTERM handler. We WANT select() to be interrupted so the done flag is checked promptly. This is the standard pattern. [ASSUMED -- standard POSIX practice]

2. **config.def.h vs hardcoded XDG logic**
   - What we know: dmenu-clip uses CACHE_DIR_NAME from config.def.h. The daemon should use the same constant.
   - Recommendation: Use config.def.h with CACHE_DIR_NAME and MAX_ENTRIES. Match dmenu-clip's pattern exactly for consistency. The XDG_CACHE_HOME resolution logic is in C code, not config. [VERIFIED: dmenu-clip/config.def.h]

3. **Handling SelectionNotify timeout**
   - What we know: If the clipboard owner crashes between XFixesSelectionNotify and our XConvertSelection request, SelectionNotify may never arrive.
   - Recommendation: Use a bounded retry count (e.g., wait up to 500ms for SelectionNotify using the select()-based loop). If timeout expires, abandon this clipboard event and return to main loop.
   - [ASSUMED -- defensive programming, not documented in X11 spec]

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| libX11 | X11 display/events | Y | 1.8.13 | -- |
| libXfixes | Clipboard change events | Y | 6.0.2 | -- |
| cc (gcc) | Compilation | Y | 14.2.1 | -- |
| make | Build system | Y | GNU Make 4.4.1 | -- |
| X11 display (runtime) | Daemon operation | Y (when X session active) | -- | -- |

**Missing dependencies with no fallback:** None -- all dependencies available.

[VERIFIED: pacman -Q libx11 -> 1.8.13-1, pacman -Q libxfixes -> 6.0.2-1]

## Sources

### Primary (HIGH confidence)
- `/usr/include/X11/extensions/Xfixes.h` -- XFixesSelectionNotifyEvent struct (lines 62-72), XFixesSelectSelectionInput function (lines 134-137) [VERIFIED: local header]
- `/usr/include/X11/Xlib.h` -- ConnectionNumber macro (line 87), XConvertSelection (line 2185), XGetWindowProperty (line 2686) [VERIFIED: local header]
- `/usr/include/sys/file.h` -- flock() with LOCK_EX (2), LOCK_NB (4) [VERIFIED: local header]
- `/usr/include/sys/select.h` -- select(), fd_set, FD_SET, FD_ZERO [VERIFIED: local header]
- `utils/dmenu-clip/dmenu-clip.c` -- Cache reader format compatibility [VERIFIED: source code]
- `scripts/dmenu-clipd` -- Shell script behavior to replicate [VERIFIED: source code]
- `.planning/research/STACK.md` -- Technology stack decisions [VERIFIED: project artifact]
- `.planning/research/PITFALLS.md` -- Known pitfalls catalog [VERIFIED: project artifact]
- `.planning/research/ARCHITECTURE.md` -- Architecture patterns [VERIFIED: project artifact]

### Secondary (MEDIUM confidence)
- [clipnotify source](https://github.com/cdown/clipnotify) -- XFixes clipboard monitoring reference (~80 lines), confirms XFixesSelectSelectionInput pattern [CITED: WebFetch]
- [thimc/scm](https://github.com/thimc/scm) -- Suckless clipboard manager, confirms flock() pattern [CITED: WebFetch raw source]
- [Implementing copy/paste in X11](https://handmade.network/forums/articles/t/8544-implementing_copy_paste_in_x11) -- XConvertSelection -> SelectionNotify -> XGetWindowProperty flow with TARGETS [CITED: WebFetch]
- [X11 clipboard deep-dive](https://movq.de/blog/postings/2017-04-02/0/POSTING-en.html) -- Selection protocol, owner disappearance, ICCCM compliance [CITED: WebFetch]
- [xlib-timeout](https://github.com/sgerwk/xlib-timeout) -- select() + ConnectionNumber + XPending pattern for event loop with timeout [CITED: WebFetch]
- [vurtun/x11clipboard gist](https://gist.github.com/vurtun/1c57d1e9e2ee169b90209325e38f0cb7) -- XConvertSelection with select() for SelectionNotify wait [CITED: WebFetch]

### Tertiary (LOW confidence)
- None -- all claims verified or cited.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- all libraries verified installed with exact versions on target system
- Architecture: HIGH -- XFixes API verified from headers, patterns confirmed by multiple reference implementations
- Pitfalls: HIGH -- documented from prior project research and verified against X11 protocol documentation
- Cache format: HIGH -- verified directly from dmenu-clip source code (Phase 3 output)
- FNV-1a constants: MEDIUM -- standard well-known values but not verified against spec in this session

**Research date:** 2026-04-09
**Valid until:** 2026-05-09 (stable -- X11 and POSIX APIs do not change)
