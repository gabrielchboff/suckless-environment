# Feature Research

**Domain:** Suckless C desktop utilities (clipboard, session, power, battery, screenshot)
**Researched:** 2026-04-09
**Confidence:** HIGH

## Feature Landscape

This covers six utilities being converted from shell to C. Features are organized per-utility, then cross-cutting concerns.

---

### Table Stakes (Users Expect These)

#### dmenu-clipd (Clipboard Daemon)

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| XFixes event-driven monitoring | Polling wastes CPU; clipnotify/clipmenu/scm all use XFixes. This is the entire reason for the rewrite. | MEDIUM | Use XFixesSelectSelectionInput on CLIPBOARD selection. ~50 lines of X11 event loop replaces the 0.5s poll. |
| CLIPBOARD selection monitoring | CLIPBOARD is the Ctrl+C/Ctrl+V buffer. Every clipboard daemon monitors this. | LOW | XA_CLIPBOARD atom via XInternAtom. |
| Content deduplication via hash | Prevents storing the same text 50 times. clipmenu and the existing shell script both do this with md5sum. | LOW | md5 or simple FNV-1a hash of content. Write to `$XDG_CACHE_HOME/dmenu-clipboard/<hash>`. |
| Max entry limit with LRU pruning | Unbounded history fills disk. Existing script caps at 50. clipmenu defaults to 1000. | LOW | On new entry, stat files by mtime, remove oldest beyond limit. |
| Text-only storage | Binary/image clipboard content should not be stored as text history. Existing script filters on UTF8_STRING/STRING/TEXT targets. | LOW | Check selection targets before requesting conversion. Only store if text type available. |
| Skip empty/whitespace-only clips | Storing blank entries is noise. Existing script already does this. | LOW | Trim and check length before hashing. |
| Graceful shutdown on SIGTERM/SIGINT | Daemon must clean up X11 resources. Expected for any long-running process. | LOW | Signal handler that calls XCloseDisplay and exits. |
| File-per-entry cache storage | Matches clipmenu/scm pattern. Allows atomic writes, easy pruning, and simple IPC with the picker. | LOW | Write content to `<cache_dir>/<hash>`. The picker reads this directory independently. |

#### dmenu-clip (Clipboard Picker)

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| List entries newest-first | Users want most recent clips at top. Universal in clipboard managers. | LOW | Sort cache files by mtime descending. |
| Truncated preview in dmenu | Full multi-line content is unreadable in dmenu. Existing script caps at 80 chars and flattens newlines to spaces. | LOW | Read first N bytes, replace `\n` with space. |
| Paste selected entry to CLIPBOARD | The core function: pick a clip, put it back on the clipboard. | LOW | Write selected file content to CLIPBOARD via XSetSelectionOwner + respond to SelectionRequest events, or exec xclip. |
| Pass-through dmenu arguments | All dmenu-* utilities receive theme args from dwm config. The `$@` passthrough is essential for visual consistency. | LOW | Forward argc/argv after own flags to dmenu exec call. |
| Handle empty history gracefully | No entries = exit silently, do not show empty dmenu or crash. | LOW | Check if cache dir empty before spawning dmenu. |

#### dmenu-session (Session Manager)

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Lock screen action | Every session menu has lock. Project requires betterlockscreen (blur, wallpaper-based). | LOW | exec betterlockscreen -l blur. Handle duplicate prevention (check if already running). |
| Logout action | Kill dwm and related processes cleanly. | LOW | pkill slstatus, pkill dmenu-clipd, pkill dwm. Same as existing script. |
| Reboot action | systemctl reboot. Universal. | LOW | exec systemctl reboot. |
| Shutdown action | systemctl poweroff. Universal. | LOW | exec systemctl poweroff. |
| Confirmation for destructive actions | Reboot/shutdown/logout are irreversible. Existing script confirms with yes/no dmenu. Standard safety pattern. | LOW | Spawn second dmenu with "no\nyes" for reboot, shutdown, logout. Lock needs no confirm. |
| DISPLAY environment export for lock | betterlockscreen needs DISPLAY set. dwm spawned processes inherit it, but explicit export prevents edge cases. | LOW | Ensure DISPLAY is set before exec. |

#### dmenu-cpupower (Power Profile Switcher)

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| List available power profiles | Show what can be set. powerprofilesctl provides: performance, balanced, power-saver. | LOW | exec powerprofilesctl list, parse output, or hardcode the three known profiles. |
| Show current profile in prompt | Users need to know current state before changing. Existing script shows governor in dmenu prompt. | LOW | Read from powerprofilesctl get. Display as dmenu -p "cpu: <current>". |
| Set selected profile | The core action. | LOW | exec powerprofilesctl set <profile>. No sudo/pkexec needed unlike cpupower. |
| Pass-through dmenu arguments | Same as other dmenu-* utilities. Visual consistency from dwm config. | LOW | Forward args to dmenu. |

#### battery-notify (Battery Monitor)

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Read battery percentage from sysfs | Standard Linux battery interface. /sys/class/power_supply/BAT1/capacity. slstatus already uses this exact path. | LOW | Open and read integer from sysfs file. |
| Read charging status from sysfs | Must only alert when discharging. /sys/class/power_supply/BAT1/status. | LOW | Read "Discharging"/"Charging"/"Full" string. |
| Dunst notification at low threshold | Alert the user. Project specifies <=20%. Use dunst-compatible notification. | LOW | exec notify-send or use libnotify directly. Dunst receives it. |
| One-shot execution model | PROJECT.md specifies one-shot. Run from cron/timer or slstatus hook, not a daemon. | LOW | Read, check, notify if needed, exit. No event loop. |

#### screenshot-notify (Screenshot Tool)

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Area selection capture via maim | Interactive region select. Already bound to PrintScreen in dwm config. maim -s is the standard. | LOW | fork+exec maim -s, pipe to xclip. |
| Copy to clipboard | Project spec says "Image copied to clipboard". Standard workflow: capture and paste into Discord/chat. | LOW | Pipe maim output to xclip -selection clipboard -t image/png. |
| Dunst notification on success | User confirmation that screenshot was captured. Without notification, user has no feedback. | LOW | exec notify-send "Screenshot" "Image copied to clipboard" on success. |
| Handle cancellation gracefully | User presses Escape during maim selection. Do not show error notification. | LOW | Check maim exit code. 0 = success, nonzero = cancelled/failed. Only notify on success. |

---

### Differentiators (Competitive Advantage)

These features go beyond the existing shell scripts and comparable tools. They align with the project's core value of "fast, safe, native C."

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| **dmenu-clipd: Zero-poll event loop** | The shell version polls every 0.5s, wasting CPU on a laptop. XFixes events mean zero CPU when idle. This is the flagship improvement. | MEDIUM | clipnotify proves this works in ~60 lines of C. The daemon blocks on XNextEvent, wakes only on clipboard change. |
| **dmenu-clipd: Atomic file writes** | Shell script can corrupt cache on concurrent write. C version can write to temp file then rename() for atomicity. | LOW | write to `<hash>.tmp`, then rename to `<hash>`. Prevents partial reads by dmenu-clip. |
| **dmenu-clip: Direct X11 clipboard ownership** | Instead of exec-ing xclip to set clipboard, own the selection directly via Xlib. Eliminates runtime dependency on xclip for the picker. | HIGH | Requires handling SelectionRequest events in an event loop after XSetSelectionOwner. More complex but removes external dependency. Worth deferring -- see Anti-Features. |
| **battery-notify: Configurable threshold via compile-time constant** | Suckless pattern: config.h with `#define LOW_THRESHOLD 20`. User recompiles to change. No runtime config parsing. | LOW | Single `#define` in header or config.h. |
| **battery-notify: Multiple severity levels** | Warning at 20%, critical at 5%. Different urgency in dunst (normal vs critical). Matches lowbat/powernotd patterns. | LOW | Two thresholds, two urgency levels. Critical urgency stays on screen until dismissed (timeout=0 in dunst). |
| **screenshot-notify: Multiple capture modes** | Area (maim -s), fullscreen (maim), active window (maim -i $(xdotool getactivewindow)). Selectable via argument or separate bindings. | LOW | Accept mode argument: "area", "full", "window". Default to area. |
| **screenshot-notify: Optional file save** | Save to ~/Screenshots/ in addition to clipboard. Timestamp filename. | LOW | Optional flag or compile-time toggle. `mkdir -p ~/Screenshots && maim -s ~/Screenshots/$(date).png` equivalent. |
| **All utilities: No shell interpreter dependency** | C binaries start faster, no shell parsing overhead, no quoting bugs, no word-splitting vulnerabilities. | N/A | This is the project's core thesis. Every utility benefits. |
| **All utilities: Consistent error handling** | Shell scripts silently fail or produce cryptic errors. C can check every system call return value and provide clear stderr messages. | LOW | fprintf(stderr, ...) for each failure path. Non-zero exit codes. |

---

### Anti-Features (Commonly Requested, Often Problematic)

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| **dmenu-clipd: PRIMARY selection monitoring** | clipmenu monitors PRIMARY (mouse selection) too. Completeness. | PRIMARY changes on every mouse text selection -- extremely noisy. Floods history with partial selections. Suckless users who want PRIMARY can pipe it manually. | Monitor only CLIPBOARD. PRIMARY is a different workflow. |
| **dmenu-clipd: Image/rich content storage** | "My clipboard manager stores images too." | Massively increases complexity: binary storage, MIME type detection, different display in dmenu, larger cache. Violates suckless minimalism. | Store text only. Images go through screenshot-notify's clipboard pipeline. |
| **dmenu-clipd: D-Bus notification interface** | Other clipboard managers expose D-Bus APIs. | D-Bus is a heavy dependency contrary to suckless philosophy. The file-based IPC (cache directory) is simpler and sufficient. | File-based IPC via shared cache directory. |
| **dmenu-clipd: Runtime configuration file** | "Let me change max entries without recompiling." | Suckless philosophy: config.h, recompile. Runtime config adds parsing code, error handling for malformed config, file search paths. | Compile-time `#define MAX_ENTRIES 50` in config.h. |
| **dmenu-clip: Fuzzy matching** | rofi users expect fuzzy search. | dmenu does not do fuzzy matching natively (requires patches). Adding custom fuzzy logic duplicates dmenu's job. | Use dmenu as-is. Users who want fuzzy matching patch dmenu itself (fuzzymatch patch exists). |
| **dmenu-clip: Direct Xlib clipboard set (v1)** | Eliminates xclip dependency. | Requires maintaining an event loop to respond to SelectionRequest events after setting ownership. If the process exits immediately, the clipboard is lost. Must either fork a background process or use xclip. Significant complexity for marginal gain. | Use xclip for v1. Consider direct Xlib only if xclip dependency becomes a real problem. |
| **dmenu-session: Suspend/hibernate** | dlogout and other session menus include these. | Suspend/hibernate behavior varies wildly across hardware. Requires testing, may need additional configuration (lid switch, wake sources). Out of scope per PROJECT.md which lists only lock/logout/reboot/shutdown. | Add later only if the user explicitly needs it. Two lines of code when needed. |
| **dmenu-cpupower: Auto-switching profiles** | "Switch to power-saver on battery, performance on AC." | Turns a simple picker into a daemon. Duplicates power-profiles-daemon's own auto-switching. Scope creep. | Use power-profiles-daemon's built-in automatic switching, or a separate udev rule. |
| **dmenu-cpupower: D-Bus integration** | powerprofilesctl uses D-Bus internally. Could call D-Bus API directly from C. | Adds libdbus or sd-bus dependency. powerprofilesctl already wraps this cleanly. exec-ing the CLI tool is simpler and sufficient for a picker that runs once per invocation. | exec powerprofilesctl. Let it handle D-Bus. |
| **battery-notify: Daemon mode with polling** | "Run it once and forget." | PROJECT.md explicitly says one-shot. Daemon adds signal handling, PID management, sleep loops. A systemd timer or cron job calling the one-shot binary is simpler and more Unix. | One-shot binary + systemd timer (e.g., every 60s). |
| **battery-notify: Remaining time estimate** | lowbat and slstatus can show time remaining. | Time remaining calculation is unreliable (fluctuates with load). slstatus already shows this in the bar. Duplicating it in a notification adds no value. | slstatus shows remaining time. battery-notify just alerts on threshold. |
| **screenshot-notify: Annotation/editing** | Flameshot-style markup. | Massive scope increase. Requires GUI toolkit, drawing primitives, text overlay. Explicitly out of scope in PROJECT.md. | Use gimp/pinta after capture if editing needed. |
| **screenshot-notify: Save to file by default** | "I want both clipboard and file." | Clutters filesystem. Most suckless users paste directly. File save should be opt-in. | Clipboard-only by default. Optional file save via compile-time flag or argument. |
| **All utilities: Wayland support** | "Future-proofing." | Explicitly out of scope in PROJECT.md. Different API surface entirely (wl-clipboard vs Xlib). Would double the codebase. | X11 only. Wayland is a separate project. |
| **All utilities: Makefile config.mk auto-detection** | "Detect available libraries automatically." | Suckless projects use simple, explicit Makefiles. Autoconf/cmake is antithetical. The user knows their system. | Explicit flags in config.mk. Document dependencies in README. |

---

## Feature Dependencies

```
dmenu-clipd (daemon)
    |
    +--writes-to--> cache directory (<XDG_CACHE_HOME>/dmenu-clipboard/)
    |
    +--read-by--> dmenu-clip (picker)
                      |
                      +--exec--> dmenu (binary, already installed)
                      +--exec--> xclip (to set clipboard)

dmenu-session
    +--exec--> betterlockscreen (AUR, for lock)
    +--exec--> systemctl (for reboot/shutdown)
    +--exec--> pkill (for logout: slstatus, dmenu-clipd, dwm)
    +--exec--> dmenu (for menu + confirmation)

dmenu-cpupower
    +--exec--> powerprofilesctl (for get/set profiles)
    +--exec--> dmenu (for menu)

battery-notify
    +--reads--> /sys/class/power_supply/BAT1/capacity
    +--reads--> /sys/class/power_supply/BAT1/status
    +--exec--> notify-send (or libnotify) -> dunst

screenshot-notify
    +--exec--> maim (screenshot capture)
    +--exec--> xclip (copy to clipboard)
    +--exec--> notify-send -> dunst

dwm-start (stays shell, launches everything)
    +--spawns--> dmenu-clipd (background daemon)
    +--spawns--> dunst
    +--spawns--> slstatus
    +--exec--> dwm
```

### Dependency Notes

- **dmenu-clip requires dmenu-clipd:** The picker reads the cache directory populated by the daemon. Without the daemon, there is nothing to pick from.
- **All dmenu-* require dmenu binary:** They exec dmenu with arguments. dmenu is already built and installed by the suckless build pipeline.
- **battery-notify requires dunst (or any notification daemon):** It sends notifications via notify-send / libnotify. dunst is already running in the environment.
- **screenshot-notify requires maim + xclip:** Both are already listed as runtime dependencies in install.sh.
- **dmenu-session requires betterlockscreen:** New dependency (AUR). Must be added to install.sh.
- **dmenu-cpupower requires power-profiles-daemon:** New dependency. Must replace cpupower in install.sh.
- **No circular dependencies exist.** Each utility is independent except the clipd/clip pair.

---

## MVP Definition

### Launch With (v1)

Core functionality matching or exceeding the existing shell scripts.

- [ ] **dmenu-clipd** -- XFixes event-driven daemon, text-only, hash dedup, max 50 entries, file-per-entry cache
- [ ] **dmenu-clip** -- List entries newest-first, truncated preview, select and copy via xclip, dmenu arg passthrough
- [ ] **dmenu-session** -- Lock (betterlockscreen), logout, reboot, shutdown with confirmation dialogs
- [ ] **dmenu-cpupower** -- List profiles, show current, set via powerprofilesctl
- [ ] **battery-notify** -- One-shot: read sysfs, notify if <=20% and discharging
- [ ] **screenshot-notify** -- Area capture via maim -s, copy to clipboard, dunst notification on success

### Add After Validation (v1.x)

Features to add once all six C utilities are working and stable.

- [ ] **battery-notify: Two-tier alerts** -- Warning at 20% (normal urgency), critical at 5% (critical urgency, no timeout)
- [ ] **screenshot-notify: Multiple capture modes** -- Area, fullscreen, active window via argument
- [ ] **screenshot-notify: Optional file save** -- Save to ~/Screenshots/ with timestamp, controlled by argument
- [ ] **dmenu-clipd: Atomic writes** -- Write to .tmp then rename() to prevent partial reads
- [ ] **dmenu-session: Duplicate lock prevention** -- Check if betterlockscreen is already running before spawning
- [ ] **install.sh updates** -- Compile C utilities, add betterlockscreen and power-profiles-daemon to deps

### Future Consideration (v2+)

Features to defer until the C utilities are battle-tested.

- [ ] **dmenu-clip: Direct Xlib clipboard ownership** -- Remove xclip dependency by owning CLIPBOARD selection directly. Complex event loop required.
- [ ] **dmenu-clipd: Configurable cache dir via compile-time config.h** -- Currently hardcoded to XDG_CACHE_HOME. Allow override.
- [ ] **battery-notify: Systemd timer unit** -- Ship a .timer and .service file for periodic execution
- [ ] **dmenu-session: Suspend/hibernate** -- Only if user requests it

---

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| dmenu-clipd XFixes event loop | HIGH | MEDIUM | P1 |
| dmenu-clipd hash dedup + pruning | HIGH | LOW | P1 |
| dmenu-clipd text-only filter | MEDIUM | LOW | P1 |
| dmenu-clip entry listing + selection | HIGH | LOW | P1 |
| dmenu-clip dmenu arg passthrough | HIGH | LOW | P1 |
| dmenu-session lock (betterlockscreen) | HIGH | LOW | P1 |
| dmenu-session logout/reboot/shutdown | HIGH | LOW | P1 |
| dmenu-session confirmation dialogs | HIGH | LOW | P1 |
| dmenu-cpupower profile list/get/set | HIGH | LOW | P1 |
| battery-notify sysfs read + notify | HIGH | LOW | P1 |
| screenshot-notify area capture + notify | HIGH | LOW | P1 |
| battery-notify two-tier alerts | MEDIUM | LOW | P2 |
| screenshot-notify multiple modes | MEDIUM | LOW | P2 |
| screenshot-notify file save option | LOW | LOW | P2 |
| dmenu-clipd atomic writes | MEDIUM | LOW | P2 |
| dmenu-clip direct Xlib clipboard | LOW | HIGH | P3 |
| battery-notify systemd timer | LOW | LOW | P3 |

**Priority key:**
- P1: Must have for launch -- matches or exceeds existing shell scripts
- P2: Should have, add when stable
- P3: Nice to have, future consideration

---

## Competitor Feature Analysis

| Feature | clipmenu (shell+clipnotify) | scm (C) | Our dmenu-clipd/clip |
|---------|---------------------------|---------|---------------------|
| Language | Shell + C (clipnotify) | C | C |
| Event detection | XFixes via clipnotify binary | XFixes built-in | XFixes built-in |
| Selections monitored | CLIPBOARD + PRIMARY + SECONDARY | CLIPBOARD | CLIPBOARD only |
| Storage format | File per entry, hash name | File per entry, cache dir | File per entry, hash name |
| Deduplication | Yes (newest wins) | Unknown | Yes (newest wins) |
| Max entries | 1000 (configurable) | Unknown | 50 (compile-time) |
| dmenu integration | Shell wrapper | scmenu wrapper | Built-in C binary |
| Dependencies | xsel, clipnotify, shell | Xlib, Xfixes | Xlib, Xfixes, xclip |
| Lines of code | ~400 total (shell) | ~200 (C) | Target: ~200-300 (C) |

| Feature | lowbat (C) | powernotd (Rust) | Our battery-notify |
|---------|-----------|-----------------|-------------------|
| Language | C | Rust | C |
| Detection | sysfs polling (3s) | sysfs polling (60s) | sysfs one-shot |
| Thresholds | 20% + 5% | Configurable JSON | 20% compile-time |
| Execution model | Daemon | Daemon | One-shot |
| Notification | libnotify | XDG notifications | notify-send exec |
| Config | Source code | JSON file | config.h |
| Lines of code | ~150 | ~500 | Target: ~60-80 |

| Feature | dlogout (C) | Our dmenu-session |
|---------|------------|------------------|
| Language | C | C |
| Interface | Custom (dbus) | dmenu |
| Actions | Session, logout, suspend, hibernate, shutdown, reboot | Lock, logout, reboot, shutdown |
| Lock method | N/A | betterlockscreen |
| Confirmation | N/A (separate UI) | dmenu yes/no prompt |
| Dependencies | dbus | dmenu, betterlockscreen, systemctl |

---

## Sources

- [clipmenu - Clipboard management using dmenu](https://github.com/cdown/clipmenu) -- Shell-based clipboard manager, reference for feature set
- [clipnotify - X11 clipboard change notification](https://github.com/cdown/clipnotify) -- C reference for XFixes event loop pattern
- [scm - Simple clipboard manager for X11](https://github.com/thimc/scm) -- C clipboard manager, suckless-style reference
- [xclipd - X11 Clipboard Manager](https://github.com/jhunt/xclipd) -- C clipboard daemon implementation reference
- [lowbat - Minimal battery monitor daemon](https://www.tjkeller.xyz/projects/lowbat/) -- C battery daemon, ~150 lines, suckless-aligned
- [powernotd - Battery notification daemon](https://github.com/Laeri/powernotd) -- Rust battery daemon, multi-threshold reference
- [slstatus battery-notify patch](https://tools.suckless.org/slstatus/patches/battery-notify/) -- Suckless-native battery notification
- [dlogout - Suckless session manager](https://dwm.suckless.narkive.com/lAj0CWrE/dev-dlogout-i-wrote-a-basic-logout-shutdown-menu-for-use-with) -- C session manager reference
- [maim - Screenshot utility](https://github.com/naelstrof/maim) -- Screenshot tool used by this project
- [powerprofilesctl man page](https://man.archlinux.org/man/extra/power-profiles-daemon/powerprofilesctl.1.en) -- Power profile management CLI
- [betterlockscreen](https://github.com/betterlockscreen/betterlockscreen) -- Lock screen with blur effects
- [Linux power supply sysfs](https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-class-power) -- Battery sysfs interface documentation
- [X11 clipboard management](https://jameshunt.us/writings/x11-clipboard-management-foibles/) -- XFixes clipboard implementation patterns
- [dmenu best practices](https://www.troubleshooters.com/linux/dmenu/bestpractices.htm) -- Security considerations for dmenu scripts
- [dmenu scripts collection](https://tools.suckless.org/dmenu/scripts/) -- Community dmenu scripts reference

---
*Feature research for: Suckless C desktop utilities*
*Researched: 2026-04-09*
