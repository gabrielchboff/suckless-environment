# Codebase Concerns

**Analysis Date:** 2026-04-09

## Tech Debt

**DWM Geometry Management:**
- Issue: `updategeom()` function is acknowledged as overly complex and fragile, with inline FIXME comment
- Files: `dwm/dwm.c` (line 669)
- Impact: Window geometry handling in multi-monitor setups may be unreliable; changes risk cascading failures
- Fix approach: Refactor `updategeom()` to separate concerns (monitor detection, workspace sizing, bar positioning); add comprehensive test coverage for multi-monitor scenarios

**ST Terminal VT Escape Sequence Coverage:**
- Issue: Multiple unimplemented ANSI/VT escape sequences stubbed with TODO comments (0x80-0x9f range)
- Files: `st/st.c` (lines 2252-2288)
- Impact: Terminal may not handle less common escape sequences; breaks compatibility with programs using these features
- Fix approach: Implement remaining escape sequences or clearly document which are intentionally unsupported; prioritize commonly used ones (CSI, ST, RI)

**ST Font Cache Complexity:**
- Issue: Font cache implementation is acknowledged in TODO as needing simplification
- Files: `st/st.c`, `st/x.c`
- Impact: Font loading may be inefficient; harder to debug rendering issues
- Fix approach: Consolidate font caching logic; profile to identify bottlenecks

**DWM XEMBED Atom Handling:**
- Issue: `getatomprop()` function incomplete documentation; should return item count and pointer, currently returns single value
- Files: `dwm/dwm.c` (lines 1091-1100)
- Impact: System tray integration may miss or misinterpret multi-valued X atom properties
- Fix approach: Extend API to return full atom data; add bounds checking and validation

## Known Bugs

**ST Terminal Line Ending Inconsistency:**
- Symptoms: Copy/paste line endings behave inconsistently between st and other terminal emulators
- Files: `st/st.c` (lines 629, 2252-2288)
- Trigger: User selects text across multiple lines and pastes in different application; newlines may not align
- Workaround: Manually adjust pasted text or use terminal multiplexer with consistent line ending handling
- Root cause: Suckless philosophy prioritizes small code over perfect compatibility; different terminals use different EOF conventions (LF vs CR)

**DWM XEMBED Event Handling:**
- Symptoms: System tray icons may not receive proper activation/focus events
- Files: `dwm/dwm.c` (line 617: "FIXME not sure if I have to send these events")
- Trigger: System tray applications that depend on XEMBED activation events (XEMBED_WINDOW_ACTIVATE)
- Workaround: Restart application or manually refocus window
- Root cause: Uncertainty about X11 XEMBED specification compliance; may be sending incomplete event set

**Clipboard History Deduplication Race Condition:**
- Symptoms: Very rapid clipboard changes (within 0.5s) may be missed or cause filename collisions
- Files: `scripts/dmenu-clipd` (lines 14-45)
- Trigger: Copy multiple different items in rapid succession; hash collision with md5sum in edge cases
- Workaround: Slow down clipboard operations or increase POLL_INTERVAL
- Root cause: 0.5s polling interval is relatively coarse; md5 collision rate is theoretically non-zero

## Security Considerations

**Clipboard Cache Permissions:**
- Risk: Clipboard cache stored as individual files in `~/.cache/dmenu-clipboard/` with default permissions; world-readable on some systems
- Files: `scripts/dmenu-clipd` (line 10, line 33)
- Current mitigation: Directory created with default umask (typically 0755)
- Recommendations: 
  - Set cache directory permissions to 0700 explicitly: `mkdir -p -m 0700 "$CACHE_DIR"`
  - Consider using encrypted storage for sensitive clipboard data
  - Document that clipboard history may be world-readable

**Shell Injection in dmenu-cpupower:**
- Risk: CPU governor selection passed to `pkexec` without validation, though limited to enumerated set
- Files: `scripts/dmenu-cpupower` (lines 11-18)
- Current mitigation: Hardcoded case statement limits selection to safe values (performance, schedutil, powersave)
- Recommendations: Already well-protected; consider adding explicit whitelist validation before pkexec call

**Shell Command Execution in dwm-start:**
- Risk: Commands executed via `exec` without full path validation; dependency on PATH
- Files: `dwm-start` (lines 13-24)
- Current mitigation: Relies on system PATH containing correct binaries
- Recommendations:
  - Use absolute paths for all external commands (`/usr/lib/polkit-gnome/...` is correct, follow for others)
  - Add error checking after each background process start (e.g., `dmenu-clipd &` should verify process started)
  - Document required PATH setup

**Run Command Component Shell Injection:**
- Risk: `run_command()` in slstatus executes arbitrary shell commands from config
- Files: `slstatus/components/run_command.c` (lines 8-31); invoked via `slstatus/config.h` (line 76)
- Current mitigation: Only executed at interval (1000ms by default); limited by suckless philosophy
- Recommendations:
  - Add length limits to prevent buffer exhaustion
  - Validate command format at runtime
  - Log executed commands for debugging
  - Document security implications of using run_command

**SUID/Capability Requirements Without Verification:**
- Risk: `dmenu-cpupower` and `dmenu-session` use `pkexec` without checking if programs are installed
- Files: `scripts/dmenu-cpupower` (line 18), `scripts/dmenu-session` (lines 16, 20)
- Current mitigation: `pkexec` will fail visibly if program not found
- Recommendations:
  - Check for existence of `cpupower`, `systemctl` before attempting execution
  - Provide clearer error messages when privilege escalation fails
  - Fallback to alternatives or graceful degradation

## Performance Bottlenecks

**Slstatus Polling with nanosleep:**
- Problem: Status bar updates on fixed 1000ms interval regardless of which components actually changed
- Files: `slstatus/slstatus.c` (lines 117-124), `slstatus/config.h` (line 4)
- Cause: All components evaluated on every cycle; expensive operations (file I/O, parsing) run regardless of necessity
- Improvement path:
  - Implement per-component refresh intervals
  - Cache frequently-accessed data (e.g., /proc/stat for CPU, battery status)
  - Use inotify/eventfd for reactive updates instead of polling

**Clipboard Daemon Polling at 0.5s Intervals:**
- Problem: dmenu-clipd wakes every 500ms even when clipboard unchanged
- Files: `scripts/dmenu-clipd` (lines 7, 45)
- Cause: Polling-based design; X11 has no native clipboard change notification
- Improvement path:
  - Use XFixesSelectionNotify if available
  - Implement exponential backoff with activity threshold
  - Switch to event-driven clipboard manager (e.g., xclip-copyfile integration with inotify)

**ST Terminal Rendering with Double Buffering:**
- Problem: Terminal redraws entire visible area on every change, even single-character updates
- Files: `st/x.c`, acknowledged in FAQ
- Cause: Double-buffering architecture trades memory for simplicity; suckless philosophy
- Improvement path:
  - Implement dirty rectangle tracking
  - Batch character updates
  - Profile actual CPU/GPU impact before optimization

**DWM Systray Layout Recalculation:**
- Problem: Systray width recalculated on every bar redraw
- Files: `dwm/dwm.c` (lines 200, 496, 2474-2476)
- Cause: Function `getsystraywidth()` called frequently; no caching between redraws
- Improvement path:
  - Cache systray width; invalidate only on tray icon add/remove
  - Use event-driven updates instead of continuous redraws

## Fragile Areas

**DWM Window Manager Event Dispatching:**
- Files: `dwm/dwm.c` (lines 8-100: event handler array)
- Why fragile: Large switch statement dispatching X11 events; ordering and completeness critical; one missing event type breaks functionality
- Safe modification:
  - Document expected behavior for each event type
  - Add test cases for each event in expected order
  - Use static analysis to verify all event types handled
  - Test with stress tools (e.g., wmctrl, xdotool) to generate various events
- Test coverage: Minimal; reliant on manual testing with real X11 servers

**ST Terminal Escape Sequence Parser:**
- Files: `st/st.c` (lines 2000-2350, escape state machine)
- Why fragile: Complex state machine with multiple interacting flags (ESC_START, ESC_CSI, ESC_STR_END, ESC_UTF8); unimplemented sequences cause silent failures
- Safe modification:
  - Add comprehensive test suite with known escape sequences from xterm, VTE
  - Add logging to parser to trace sequence handling
  - Test against real applications (vim, less, htop)
  - Document each case statement's corresponding escape code
- Test coverage: None; only documented in TODO

**DWM Layout Engine with Custom Patches:**
- Files: `dwm/dwm.c` (layout function pointers); applied patches in `dwm/patches/` (dwm-status2d-systray-6.4.diff)
- Why fragile: Multiple patches applied modifying core window management; patch files show merge artifacts (p1.rej exists)
- Safe modification:
  - Resolve .rej files completely before use
  - Document which patches are applied and their interaction
  - Test layouts with multiple windows, monitors, and floating windows
  - Keep patches as separate commits in version control
- Test coverage: Merge conflict markers (.rej file at `dwm/p1.rej`) indicate incomplete patch application

**Slstatus Component Loading with String Concatenation:**
- Files: `slstatus/components/battery.c` (lines 44, 67: `esnprintf` with PATH_MAX)
- Why fragile: Battery names and paths hardcoded; different systems use BAT0/BAT1; path buffer overflows if names exceed expectations
- Safe modification:
  - Add bounds checking even though esnprintf() protects
  - Support configurable battery names via environment variables
  - Test on systems with non-standard battery naming (BAT, BAT_0, etc.)
  - Use platform-specific path discovery
- Test coverage: None; hardcoded for Linux

## Scaling Limits

**Clipboard Cache Size:**
- Current capacity: 50 entries max (configurable in dmenu-clipd, line 6)
- Limit: Cache directory grows unbounded if MAX_ENTRIES increased without cleanup strategy; md5 filenames don't scale well
- Scaling path:
  - Implement size-based rotation (max MB instead of max entries)
  - Use SQLite or similar for metadata instead of filesystem
  - Implement compression for large clipboard entries
  - Add expiration (oldest entry > N days deleted)

**DWM Monitor Tracking:**
- Current capacity: Limited by Xinerama/X11; typically 4-16 monitors
- Limit: Hardcoded tag array length (9 tags in config.h); doesn't scale to ultra-wide multi-monitor setups
- Scaling path:
  - Make tag count dynamic instead of compile-time
  - Support monitor-specific tag sets
  - Implement virtual desktops instead of fixed tags

**ST Terminal Scrollback Buffer:**
- Current capacity: Fixed in compile-time configuration (config.h)
- Limit: Scrollback not implemented in suckless; all history lost when window closes
- Scaling path:
  - Add configurable scrollback history
  - Implement ring buffer for memory efficiency
  - Consider disk-backed history for long-running terminals

**Slstatus Status String Length:**
- Current capacity: 2048 characters (MAXLEN in slstatus/config.h, line 10)
- Limit: Over-long formatted status crashes with truncation warning; no graceful degradation
- Scaling path:
  - Dynamic buffer allocation based on component count
  - Per-component output length limits
  - Prioritize critical components if output exceeds limit

## Dependencies at Risk

**Arch Linux Pacman Hardcoded in install.sh:**
- Risk: Only supports Arch-based systems; breaks on Debian, Fedora, Alpine, etc.
- Files: `install.sh` (lines 10-29)
- Impact: Installation script not portable; requires manual adaptation for other distros
- Migration plan:
  - Detect Linux distribution (lsb_release, /etc/os-release)
  - Provide package lists for common distros
  - Offer minimal build-from-source alternative
  - Document manual installation steps for unsupported distros

**X11 Hard Dependency Without Wayland Support:**
- Risk: Complete project depends on X11; Wayland adoption growing; future systems may drop X11
- Files: All of `dwm/`, `dmenu/`, `st/`, implicitly in scripts
- Impact: Cannot run on Wayland; increasingly problematic as distributions transition
- Migration plan:
  - Research Wayland alternatives (wlroots-based window managers)
  - Evaluate whether suckless philosophy compatible with Wayland
  - Long-term: Plan transition path or accept X11-only limitation

**FCitx5 Input Method Without Fallback:**
- Risk: fcitx5 started unconditionally in dwm-start; if not installed or crashes, input broken
- Files: `dwm-start` (line 14)
- Impact: Non-English users without fcitx5 cannot input; crashes silently
- Migration plan:
  - Check fcitx5 exists before starting: `command -v fcitx5 && fcitx5 -d`
  - Provide input method selection mechanism
  - Document required packages per language

**Polkit-gnome Without Alternative:**
- Risk: Policy Kit authentication required for slock and systemctl; polkit-gnome is GNOME-dependent
- Files: `dwm-start` (line 13), `scripts/dmenu-cpupower`, `scripts/dmenu-session`
- Impact: Minimal desktop environment (dwm) requires GNOME auth daemon
- Migration plan:
  - Support suckless-specific auth mechanism or alternative polkit frontend
  - Use sudo with NOPASSWD sudoers entries as alternative
  - Document auth requirement and alternatives

## Missing Critical Features

**No Password-Protected Lock Screen:**
- Problem: `slock` used in dmenu-session; requires separate setup and may not be integrated with session
- Blocks: Cannot securely lock computer without additional installation
- Current workaround: Users must install slock separately
- Recommendation: Add slock as build dependency or provide integration guide

**No Error Recovery in Core Services:**
- Problem: slstatus, dmenu-clipd, polkit-gnome start in background without supervision
- Blocks: If any crashes, user unaware; no automatic restart
- Current workaround: Manual restart required
- Recommendation: Add process supervisor (runit, s6) or systemd user services

**No Configuration Hot-Reload:**
- Problem: DWM and st require recompilation for config changes
- Blocks: Cannot change keybindings or appearance without restarting WM
- Current workaround: Edit config.h and `sudo make clean install`
- Recommendation: Add runtime configuration support or configuration watchers

**No Shell Integration for ST Environment Variables:**
- Problem: ST terminal doesn't set TERM or other variables that scripts depend on
- Blocks: Some CLI tools may misbehave in st without proper TERM
- Current workaround: Manually export TERM=st in .profile
- Recommendation: Ensure ST sets appropriate TERM before spawning shell

## Test Coverage Gaps

**DWM Layout Switching:**
- What's not tested: Layout switching with multiple windows, floating/tiling transitions
- Files: `dwm/dwm.c` (layout array, lines 57-62; arrange functions lines ~800-1000)
- Risk: Layout bugs only discovered in manual use; regressions undetected
- Priority: High (core functionality)

**ST Escape Sequence Edge Cases:**
- What's not tested: UTF-8 multibyte sequences combined with escape codes; boundary conditions
- Files: `st/st.c` (parser, lines 2000-2350)
- Risk: Terminal crashes or garbles display with certain character sequences
- Priority: High (crash risk)

**Slstatus Component Failures:**
- What's not tested: Behavior when /proc files missing, permissions denied, or system files contain invalid data
- Files: `slstatus/components/*.c` (all file readers like `pscanf` in lines 125-141 of util.c)
- Risk: Status bar crashes or displays garbage if /proc files unavailable
- Priority: Medium (degradation, not crash)

**Clipboard Daemon Concurrent Access:**
- What's not tested: Multiple simultaneous reads/writes; what happens if cache directory deleted while running
- Files: `scripts/dmenu-clipd` (file I/O lines 33-42)
- Risk: Corrupted clipboard entries; cache directory recreation race condition
- Priority: Medium (data loss risk)

**DWM Multi-Monitor Geometry:**
- What's not tested: Monitor hot-plug, resolution changes, mismatched refresh rates
- Files: `dwm/dwm.c` (updategeom, lines 2242-2280)
- Risk: Window positioning, bar layout broken after monitor changes
- Priority: High (multi-monitor setup)

**Install Script Dependency Validation:**
- What's not tested: Behavior if pacman packages fail, AUR helper not found, sudo not configured
- Files: `install.sh` (lines 16-29)
- Risk: Incomplete installation without clear indication of what failed
- Priority: Medium (setup reliability)

---

*Concerns audit: 2026-04-09*
