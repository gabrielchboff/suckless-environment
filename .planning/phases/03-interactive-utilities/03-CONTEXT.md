# Phase 3: Interactive Utilities - Context

**Gathered:** 2026-04-09
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver three dmenu-driven C utilities: `dmenu-session` (lock/logout/reboot/shutdown with confirmation dialogs), `dmenu-cpupower` (power profile switching via powerprofilesctl), and `dmenu-clip` (clipboard history picker reading from dmenu-clipd's cache). All use the dmenu pipe library (common/dmenu.c) from Phase 1 and follow the established build/install pattern.

</domain>

<decisions>
## Implementation Decisions

### Session Manager (dmenu-session)
- **D-01:** Four actions: lock, logout, reboot, shutdown — presented via dmenu as a list
- **D-02:** Lock uses betterlockscreen: `betterlockscreen -l --display 1 --blur 0.8`
- **D-03:** DISPLAY validation: check `getenv("DISPLAY")` is non-null before spawning betterlockscreen. Fall back to error via warn() if DISPLAY unset.
- **D-04:** Duplicate lock prevention: `pgrep -f betterlockscreen` check before spawning. If already running, exit silently.
- **D-05:** Confirmation dialogs for destructive actions (logout, reboot, shutdown): second dmenu call with "no"/"yes" options. No confirmation for lock.
- **D-06:** Logout kills: slstatus, dmenu-clipd, dunst, then dwm (in that order). Uses exec_wait with pkill for each.
- **D-07:** Reboot via `systemctl reboot`, shutdown via `systemctl poweroff` — both via exec_detach after confirmation.

### Power Profiles (dmenu-cpupower)
- **D-08:** Read current profile: exec `powerprofilesctl get` and capture output for dmenu prompt text
- **D-09:** List profiles: performance, balanced, power-saver — hardcoded (standard across all systems with power-profiles-daemon)
- **D-10:** Set profile: exec `powerprofilesctl set {selected}` via exec_wait
- **D-11:** Show current profile in dmenu prompt: `dmenu_open("cpu: {current}", extra_argv)`
- **D-12:** Pass extra argv through to dmenu (same as all dmenu utilities)

### Clipboard Picker (dmenu-clip)
- **D-13:** Read cache from `~/.cache/dmenu-clipboard/` (XDG_CACHE_HOME fallback)
- **D-14:** Sort entries by mtime (newest first) using stat() on each file
- **D-15:** Preview: first 80 chars of each file, newlines replaced by spaces
- **D-16:** Display previews in dmenu, match selection back to hash filename
- **D-17:** Restore selected entry to clipboard via exec: `xclip -selection clipboard < cache_file`
- **D-18:** Pass extra argv through to dmenu

### Claude's Discretion
- Whether to use config.def.h for dmenu-session/dmenu-cpupower (probably not — no configurable values)
- Error handling strategy for powerprofilesctl failures
- Maximum entries to display in dmenu-clip (currently 50, matching shell script)

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Foundation (Phase 1)
- `utils/common/util.h` — die(), warn(), pscanf(), exec_wait(), exec_detach(), setup_sigchld()
- `utils/common/dmenu.h` — DmenuCtx, dmenu_open(), dmenu_write(), dmenu_read(), dmenu_close()
- `utils/common/dmenu.c` — Full two-step pipe implementation (fork+dup2+exec)
- `utils/config.mk` — Shared compiler flags
- `utils/Makefile` — Build system to extend with 3 new BINS entries

### Existing Shell Scripts (read-only reference — DO NOT MODIFY)
- `scripts/dmenu-session` — Current shell implementation of session manager
- `scripts/dmenu-cpupower` — Current shell implementation of CPU power selector
- `scripts/dmenu-clip` — Current shell implementation of clipboard picker

### Research
- `.planning/research/ARCHITECTURE.md` — dmenu fork+dup2+exec pipe pattern
- `.planning/research/PITFALLS.md` — DISPLAY propagation, bidirectional pipe deadlock

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `common/dmenu.c` — Full dmenu pipe API: dmenu_open (with prompt + extra_argv), dmenu_write, dmenu_read, dmenu_close
- `common/util.c` — exec_wait (for commands where exit status matters), exec_detach (fire-and-forget), setup_sigchld
- Phase 2 pattern: battery-notify/screenshot-notify show how to structure a main() that uses these helpers

### Established Patterns
- dmenu interaction: open → write items → read selection → close
- Confirmation dialog: second dmenu_open with "no"/"yes" items
- Process spawning: fork+execvp only, never system/popen
- Build: add to BINS in Makefile, create subdirectory with .c file

### Integration Points
- `utils/Makefile` BINS variable — add dmenu-session, dmenu-cpupower, dmenu-clip
- `~/.cache/dmenu-clipboard/` — dmenu-clip reads this (dmenu-clipd writes it in Phase 4)
- dmenu-clip can be tested with manually populated cache before Phase 4 daemon exists

</code_context>

<specifics>
## Specific Ideas

- dmenu-session should match the existing shell script's menu order: logout, lock, reboot, shutdown
- powerprofilesctl output for `get` is just the profile name on stdout (e.g., "balanced\n")
- Clipboard cache files are named by hash (e.g., md5sum), contents are the raw clipboard text

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 03-interactive-utilities*
*Context gathered: 2026-04-09*
