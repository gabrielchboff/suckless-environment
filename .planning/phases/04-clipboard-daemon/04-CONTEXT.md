# Phase 4: Clipboard Daemon - Context

**Gathered:** 2026-04-09
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver `dmenu-clipd`, a long-running X11 clipboard daemon that uses XFixes events to monitor CLIPBOARD selection changes, stores text entries as hash-named files in `~/.cache/dmenu-clipboard/`, and provides the data source that dmenu-clip (Phase 3) reads from. This completes the full clipboard workflow: copy in any app → dmenu-clipd captures → dmenu-clip browses → paste.

</domain>

<decisions>
## Implementation Decisions

### Clipboard Monitoring Scope
- **D-01:** Monitor CLIPBOARD selection only via XFixes. This captures:
  - App copies (Ctrl+C)
  - Terminal copies (Ctrl+Shift+C in st)
  - Vim/Neovim yanks to system clipboard (`"+y`)
  - Tmux copies (when `set-clipboard on` is configured)
- **D-02:** Do NOT monitor PRIMARY selection — mouse highlights would flood the history with noise.

### Event-Driven Architecture
- **D-03:** Use XFixesSelectSelectionInput() + XNextEvent() event loop. Zero CPU when idle, instant change detection. Replaces the shell script's 0.5s polling.
- **D-04:** On clipboard change event: XConvertSelection() to request data, handle SelectionNotify to read the content via XGetWindowProperty().
- **D-05:** Text-only storage: check if clipboard contains UTF8_STRING or STRING target. Skip images/binary data.

### Content Storage
- **D-06:** Store entries in `~/.cache/dmenu-clipboard/` (XDG_CACHE_HOME fallback) — same directory dmenu-clip reads.
- **D-07:** FNV-1a hash of content for filename (replaces shell's md5sum). ~10 lines inline, zero dependencies.
- **D-08:** Max 50 entries with LRU pruning — remove oldest by mtime when count exceeds limit.
- **D-09:** Write content directly to hash-named file (v1 — atomic writes via .tmp+rename deferred to v2).

### Daemon Lifecycle
- **D-10:** flock on a lock file in the cache directory for single-instance enforcement. Second instance exits immediately.
- **D-11:** SIGTERM handler sets a `volatile sig_atomic_t done = 1` flag. Event loop checks this flag each iteration.
- **D-12:** On shutdown: close X11 display connection, release flock, exit cleanly.
- **D-13:** To prevent XNextEvent from blocking SIGTERM indefinitely, use `ConnectionNumber(dpy)` + `select()` with a timeout (e.g., 1 second) to periodically check the done flag.

### Build Integration
- **D-14:** Links against `-lX11 -lXfixes` (only utility needing X11 libraries). Add to Makefile with explicit LDFLAGS.
- **D-15:** Add to BINS in utils/Makefile. Compile-time config via config.def.h (cache dir, max entries).

### Claude's Discretion
- FNV-1a implementation details (32-bit vs 64-bit, exact constants)
- Exact XConvertSelection/SelectionNotify handling sequence
- Whether to use config.def.h for cache dir or hardcode with XDG fallback
- dwm-start update to launch new dmenu-clipd instead of shell script

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Foundation (Phase 1)
- `utils/common/util.h` — die(), warn(), setup_sigchld()
- `utils/config.mk` — Shared compiler flags (will need X11 LDFLAGS addition for this utility)
- `utils/Makefile` — Build system to extend

### Phase 3 (dmenu-clip)
- `utils/dmenu-clip/dmenu-clip.c` — Reads the cache directory this daemon writes to. Must maintain compatible format (hash-named files, raw text content).

### Research
- `.planning/research/STACK.md` — XFixes API, libX11/libXfixes verified installed
- `.planning/research/ARCHITECTURE.md` — Clipboard daemon architecture, SelectionRequest protocol
- `.planning/research/PITFALLS.md` — XNextEvent blocking SIGTERM, clipboard data loss on app exit, zombie processes

### External References (read-only)
- `scripts/dmenu-clipd` — Current shell script (DO NOT MODIFY, reference only)
- clipnotify source (github.com/cdown/clipnotify) — ~80 line XFixes reference implementation

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `common/util.c` — die(), warn(), setup_sigchld() for daemon lifecycle
- Phase 2 pattern: config.def.h/config.h for compile-time configuration

### Established Patterns
- fork+execvp for process spawning (not needed here — daemon is self-contained)
- SIGCHLD handler pattern from util.c
- Hash-named files in cache dir (compatible with dmenu-clip)

### Integration Points
- `utils/Makefile` BINS — add dmenu-clipd with X11 LDFLAGS
- `~/.cache/dmenu-clipboard/` — write-side of the cache dmenu-clip reads
- `dwm-start` — needs update to launch new C daemon instead of shell script

</code_context>

<specifics>
## Specific Ideas

- The daemon replaces the shell script `dmenu-clipd` which polled xclip every 0.5s
- FNV-1a produces different hashes than MD5 — existing cache entries from the shell script will be orphaned (acceptable, they'll be pruned by LRU)
- dwm-start currently launches `dmenu-clipd &` — same binary name, no change needed if installed to same PATH location

</specifics>

<deferred>
## Deferred Ideas

- Full SelectionRequest serving (CLIPD-06, v2) — daemon claims clipboard ownership and serves data to requestors
- Atomic file writes via .tmp+rename (CLIPD-07, v2)
- INCR protocol for >256KB entries (CLIPD-08, v2)

</deferred>

---

*Phase: 04-clipboard-daemon*
*Context gathered: 2026-04-09*
