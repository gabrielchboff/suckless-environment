# Architecture

**Analysis Date:** 2026-04-09

## Pattern Overview

**Overall:** Modular suckless environment with independent, single-purpose tools communicating through X11 protocol and shell scripts.

**Key Characteristics:**
- Minimalist X11-based desktop environment components
- Configuration-driven customization via C header files (config.h)
- Event-driven architecture for window management and status display
- Lightweight shell script glue for system integration
- Clipboard and power management daemon patterns

## Layers

**Window Manager Layer (dwm):**
- Purpose: Core X11 window management, layout control, and client organization
- Location: `dwm/` - `dwm/dwm.c`, `dwm/config.h`, `dwm/drw.c`
- Contains: Window event handlers, layout engines (tile, float, monocle), monitor management, status bar
- Depends on: X11 libraries (Xlib, Xft), drawing abstraction (drw)
- Used by: All X11 applications, status bar display

**Status Display Layer (slstatus):**
- Purpose: System information aggregation and X11 root window title updates
- Location: `slstatus/` - `slstatus/slstatus.c`, `slstatus/config.h`, `slstatus/components/`
- Contains: Component modules for CPU, RAM, battery, network, disk, datetime monitoring
- Depends on: X11 (for setting root window name), system files (/proc, /sys), platform-specific APIs
- Used by: dwm status bar (reads root window name), system monitoring

**Application Launcher Layer (dmenu):**
- Purpose: Textual menu interface for program execution and user selection
- Location: `dmenu/` - `dmenu/dmenu.c`, `dmenu/config.h`, `dmenu/drw.c`
- Contains: Item filtering, text input, rendering, keyboard/mouse navigation
- Depends on: X11 libraries, drawing abstraction (drw)
- Used by: User keybindings, shell scripts for system operations

**Terminal Emulator Layer (st):**
- Purpose: Lightweight terminal for shell interaction
- Location: `st/` - `st/st.c`, `st/config.h`
- Contains: PTY management, VT100 escape sequence parsing, text rendering
- Depends on: X11, system PTY interface, POSIX terminal APIs
- Used by: User shell sessions

**Script Integration Layer:**
- Purpose: System-level operations and daemon management
- Location: `scripts/`, `dwm-start`, `.xprofile`
- Contains: Clipboard daemon (dmenu-clipd), clipboard browser (dmenu-clip), CPU governor selector (dmenu-cpupower), session manager (dmenu-session)
- Depends on: Shell (sh), dmenu, system utilities (xclip, cpupower, systemctl, slock)
- Used by: dwm keybindings, X11 initialization

**Notification Layer (dunst):**
- Purpose: Desktop notification rendering and management
- Location: `dunst/dunstrc`
- Contains: Notification style, positioning, timeout rules, urgency levels
- Depends on: dunst daemon, X11
- Used by: Applications sending notifications

## Data Flow

**Startup Sequence:**

1. X11 server starts and executes `/usr/bin/dwm` (via xdm or startx)
2. `dwm-start` wrapper executes: polkit-gnome agent, fcitx5 IME, dmenu-clipd daemon, dunst daemon
3. slstatus enters main loop, periodically collecting system stats
4. slstatus writes formatted string to X11 root window name
5. dwm reads root window name and displays in status bar
6. feh sets wallpaper, dwm takes focus, enters event loop

**User Input Flow (Keybinding):**

1. User presses keybinding (e.g., Super+d for dmenu)
2. dwm's event handler matches key from `config.h` arrays
3. dwm calls associated function with args (e.g., `spawn()`)
4. spawn() forks and executes command array
5. dmenu/application opens and runs to completion
6. dwm regains focus, resumes event loop

**Clipboard History Flow:**

1. dmenu-clipd daemon polls X11 clipboard at 0.5s intervals
2. New clipboard content → hash, write to `~/.cache/dmenu-clipboard/`
3. User presses Super+v for clipboard menu
4. dmenu-clip reads cache directory, formats preview text
5. dmenu displays numbered list of recent clipboard entries
6. User selects entry → dmenu-clip loads from cache
7. dmenu-clip writes selection back to X11 clipboard via xclip

**Status Bar Update Flow:**

1. slstatus main loop calls component functions (cpu_perc, ram_used, battery_perc, etc.)
2. Components read system files (/proc/stat, /proc/meminfo, /sys/class/power_supply/)
3. Components format output strings per config.h format specifiers
4. slstatus concatenates all outputs, respecting format strings
5. slstatus calls XStoreName(root_window, status_string)
6. dwm monitors root window for changes via PropertyNotify events
7. dwm redraws status bar segment with new content

**State Management:**

- **dwm clients:** Linked list per monitor, sorted by focus history (stack)
- **dwm tags:** Bitmask per client (0-8), visibility based on seltags per monitor
- **dmenu-clipd:** File-based (MD5 hash as filename), on-disk state
- **slstatus:** Stateless iterator, reads live system state each cycle
- **X11 root window:** Central messaging point for window manager ↔ status display

## Key Abstractions

**Client Structure (dwm):**
- Purpose: Represents an X11 window managed by dwm
- Examples: `dwm/dwm.c` lines 104-117
- Pattern: Fixed-size struct with pointers to next/stack, bitmask for tags, geometry tracking

**Component Function (slstatus):**
- Purpose: Pluggable system info collector
- Examples: `slstatus/components/cpu.c`, `slstatus/components/ram.c`
- Pattern: Signature `const char *func(const char *arg)` returns pointer to static buffer

**Layout Function (dwm):**
- Purpose: Tile arrangement algorithm
- Examples: `tile()`, `monocle()` in `dwm/dwm.c`
- Pattern: Takes monitor pointer, rearranges client geometries in-place

**Draw Abstraction (drw):**
- Purpose: Graphics layer decoupling X11 details
- Examples: `dwm/drw.c`, `dmenu/drw.c` (shared implementation)
- Pattern: Functions like `drw_rect()`, `drw_text()`, `drw_map()` hide X11 Drawable operations

**Button/Key Binding:**
- Purpose: Maps input event to action with arguments
- Examples: `dwm/config.h` lines 85-131
- Pattern: Struct with modifier mask, key/button code, function pointer, Arg union

## Entry Points

**dwm (Window Manager):**
- Location: `dwm/dwm.c` main() function (after reading `dwm/config.h`)
- Triggers: X11 session initialization via xdm/startx
- Responsibilities: Event dispatch loop, window lifecycle, layout management, keybinding execution

**slstatus (Status Display):**
- Location: `slstatus/slstatus.c` main() function (after reading `slstatus/config.h`)
- Triggers: Called from `dwm-start` script in background loop with 1s sleep
- Responsibilities: Iterate component array, format status string, update X11 root window name

**dmenu (Application Launcher):**
- Location: `dmenu/dmenu.c` main() function
- Triggers: User keybinding via dwm spawn(), or manual shell invocation
- Responsibilities: Read stdin/list items, filter by input, display menu, output selection

**dmenu-clipd (Clipboard Daemon):**
- Location: `scripts/dmenu-clipd` - infinite loop polling X11 clipboard
- Triggers: Started in background from `dwm-start`
- Responsibilities: Monitor clipboard changes, deduplicate by MD5 hash, maintain cache directory

**dmenu-clip (Clipboard Browser):**
- Location: `scripts/dmenu-clip` - wrapper around dmenu for clipboard selection
- Triggers: User keybinding via dwm (Super+v)
- Responsibilities: Format cached clipboard entries, invoke dmenu, restore selection

**dmenu-cpupower (CPU Governor Selector):**
- Location: `scripts/dmenu-cpupower` - wrapper for CPU frequency scaling
- Triggers: User keybinding via dwm (Super+p)
- Responsibilities: Read current governor, show options in dmenu, apply via pkexec cpupower

**dmenu-session (Power/Session Manager):**
- Location: `scripts/dmenu-session` - wrapper for system power operations
- Triggers: User keybinding via dwm (Ctrl+Alt+Delete)
- Responsibilities: Show session menu (logout/lock/reboot/shutdown), confirm, execute

**st (Terminal Emulator):**
- Location: `st/st.c` main() function (after reading `st/config.h`)
- Triggers: User keybinding via dwm (Super+Return)
- Responsibilities: Create PTY, fork shell, manage terminal state, render text

## Error Handling

**Strategy:** Minimal error reporting via die() macro or silent failures

**Patterns:**
- C utilities exit with die(msg) on fatal conditions (missing display, allocation failure)
- Shell scripts use `[ -z "$var" ] && exit [N]` for validation
- slstatus components return "n/a" string on read errors (configurable via unknown_str)
- dmenu-clipd silently skips non-text clipboard content and empty entries
- No retry logic; fail fast and let supervisors (systemd/scripts) restart as needed

## Cross-Cutting Concerns

**Logging:** No centralized logging; dwm/dmenu/slstatus write to stderr if needed; shell scripts silent unless error

**Validation:** Input validation via type checking (C) and shell parameter expansion checks; X11 API calls checked for NULL returns

**Authentication:** Privilege escalation via polkit (pkexec) for cpupower, systemctl; polkit-gnome agent started on startup

**Color Scheme:** Tokyo Night palette consistently applied across dwm, dmenu, slstatus, dunst configs - colors defined as hex strings in respective config.h / config files

**Font:** Iosevka Nerd Font size 14 primary; used in dwm/dmenu/dunst/slstatus for consistency

**X11 Protocol:** All components use X11 Xlib directly; coordinate through root window properties and EWMH atoms; no higher-level window manager protocol layer

---

*Architecture analysis: 2026-04-09*
