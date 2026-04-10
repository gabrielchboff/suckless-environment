<!-- GSD:project-start source:PROJECT.md -->
## Project

**Suckless Environment v2**

A hardened, C-native utility suite for a dwm-based Arch Linux desktop environment. Replaces fragile shell scripts with compiled C programs that integrate with dmenu, dunst, and X11 — while fixing broken functionality (lock screen, CPU profiles) and adding missing features (battery alerts, screenshot notifications).

**Core Value:** Every user-facing utility is a fast, safe, native C program that works reliably without runtime dependencies on shell interpreters.

### Constraints

- **Platform**: Arch Linux only — pacman/AUR package names
- **Philosophy**: Suckless-style C — minimal dependencies, no frameworks
- **Display server**: X11 — all utilities use Xlib/XFixes directly where needed
- **Deps**: betterlockscreen (AUR), power-profiles-daemon (pacman), maim + xclip (pacman)
- **Build**: Each util has its own Makefile, install.sh orchestrates everything
<!-- GSD:project-end -->

<!-- GSD:stack-start source:codebase/STACK.md -->
## Technology Stack

## Languages
- C (C99) - Window manager, terminal emulator, menu system, status monitor
- Shell (POSIX sh) - Installation scripts, automation, clipboard management, power control
- POSIX Make - All suckless tools use Makefiles with config.mk pattern
## Runtime
- X11 (Xlib) - Display server foundation
- Linux (Arch/Void with pacman) - Target operating system
- pacman - Arch Linux package manager
- AUR helpers (yay/paru) - Community repository packages
- Manual builds - Suckless tools built from source
## Frameworks
- dwm 6.8 - Dynamic window manager
- st - Simple terminal emulator
- dmenu - Dynamic menu utility
- slstatus 1.1 - Status monitor daemon
- dunst - Notification daemon
- slock - Simple screen locker
- Standard POSIX Make - All components use identical Makefile pattern
- X11/Xlib - X11 protocol client library
- libxft - Font rendering for X11
- libxinerama - Multi-monitor support
- freetype2 - Font rasterization
- fontconfig - Font configuration
- libpulse - PulseAudio client (for volume control)
## Key Dependencies
- `base-devel` - GNU toolchain (gcc, make, etc.)
- `libxft` - Font rendering in X11
- `libxinerama` - Multi-monitor support for dwm
- `freetype2` - TrueType font support
- `fontconfig` - Font discovery and configuration
- `xorg-server` - X11 server (if not already installed)
- `xorg-xinit` - X11 initialization scripts (startx, xinitrc)
- `ttf-iosevka-nerd` - Nerd Font with extended glyphs (status icons, tags)
- `feh` - Image viewer/wallpaper setter
- `fcitx5` - Input method framework (CJK text input)
- `polkit-gnome` - PolicyKit GNOME authentication agent (for privilege escalation)
- `libpulse` - PulseAudio client library (volume control)
- `xorg-xbacklight` - Backlight brightness control
- `maim` - Screenshot utility with selection
- `xclip` - Clipboard access (copy/paste with X selection)
- `xsel` - Clipboard selection utility (alternative to xclip)
- `xdotool` - X11 automation tool
- `thunar` - File manager (likely for context menu integration)
- `cpupower` - CPU frequency and governor management
- `slock` - Simple screen locker
- `brave-bin` - Brave browser pre-compiled (referenced in install.sh, but not used in WM config)
## Configuration
- `.xprofile` - X session environment setup
- `~/.local/bin/` - User scripts directory (added to PATH in dwm-start)
- `dwm/config.mk` - DWM build settings
- `slstatus/config.mk` - slstatus build settings
- `st/config.mk` - Simple Terminal build settings
- `dmenu/config.mk` - dmenu build settings
- `dwm/config.h` - Window manager bindings, colors, tags, layouts
- `st/config.h` - Terminal appearance and behavior
- `dmenu/config.h` - Menu appearance
- `slstatus/config.h` - Status bar components and formatting
- `dunst/dunstrc` - Notification daemon configuration
## Platform Requirements
- Linux with X11 display server
- C99-compliant C compiler (gcc recommended)
- POSIX-compatible shell (sh, bash)
- Standard GNU tools (make, sed, awk, etc.)
- Full development headers (libxft-dev, freetype2-dev, etc.)
- X11 server running (not required if using Wayland-incompatible setup)
- Arch Linux / Void Linux (uses pacman)
- Install with: `cd /home/void/.config/suckless-environment && ./install.sh`
- Built binaries installed to `/usr/local/bin/`
- Manual pages installed to `/usr/local/share/man/`
<!-- GSD:stack-end -->

<!-- GSD:conventions-start source:CONVENTIONS.md -->
## Conventions

## Naming Patterns
- C source files: lowercase with underscore separation (e.g., `cpu.c`, `datetime.c`, `netspeeds.c`)
- Header files: uppercase for config headers (`config.h`, `config.def.h`); lowercase for utility headers (`util.h`, `slstatus.h`)
- Shell scripts: lowercase with hyphens (e.g., `dmenu-clip`, `dmenu-clipd`, `dmenu-cpupower`)
- Component modules: organized in `components/` directory with individual `.c` files per feature
- Snake_case with descriptive names: `get_gid()`, `cpu_freq()`, `battery_perc()`, `netspeed_rx()`
- Function pointers in structs follow same convention: `(*func)(const char *)`
- Static helper functions prefixed with context: `load_uvmexp()`, `pagetok()`
- Parameter names either descriptive or `unused` when intentionally not used (e.g., `cpu_freq(const char *unused)`)
- Snake_case for local and static variables: `free_pages`, `total_size`, `polling_interval`
- Single letter or abbreviated names acceptable for loop counters and mathematical operations: `i`, `n`, `sum`, `freq`
- Prefix `last_` for tracking previous state: `last_sum`, `last_buffer`
- Global state variables in uppercase prefixed with context: `CPU_FREQ`, `CACHE_DIR`
- Standard POSIX types preferred: `FILE`, `time_t`, `uintmax_t`, `struct timespec`
- Custom structs defined inline where needed (e.g., `struct arg` in `slstatus.c`)
- No typedef abuse—use `struct` keyword directly in most cases
- Enum names descriptive and grouped by purpose (e.g., `enum { SchemeNorm, SchemeSel }`)
- #define macro names in UPPERCASE: `MAXLEN`, `MAX_PREVIEW`, `MAX_ENTRIES`, `CACHE_DIR`
- Color/style constants in UPPERCASE: `LEN(x)`, `WIDTH(X)`, `HEIGHT(X)`
- Version macros: `VERSION_MAJOR`, `VERSION_MINOR`
## Code Style
- K&R style braces: opening brace on same line, closing on new line
- Indentation: tabs (default suckless standard)
- Line length: generally fits in 80 columns; pragmatic exceptions allowed for long format strings or paths
- No trailing whitespace
- No enforced linter; code follows suckless conventions manually
- Code review based on clarity and POSIX compliance
- Warning suppression avoided—warnings indicate problems to fix
- Single space after control flow keywords: `if (`, `while (`, `for (`
- No space between function name and parenthesis for calls: `printf(...)`, `fopen(...)`
- Space around operators: `a + b`, `x = 5`, `n != 1`
- Multiple statements rare; typically one per line
## Import Organization
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../slstatus.h"
#include "../util.h"
- Platform-specific headers guarded by preprocessor conditionals: `#if defined(__linux__)`
- Allows single codebase across Linux, OpenBSD, FreeBSD
- Relative paths used for local includes: `"../util.h"`, `"../../util.h"`
- No alias system; paths reflect actual directory structure
## Error Handling
- Return `NULL` on error from value-returning functions: `pscanf()`, `fmt_human()`, `bprintf()`
- Use `die()` for fatal errors that should exit program: `die("usage: %s [-v] [-s] [-1]", argv0)`
- Use `warn()` for non-fatal errors that allow continuation: `warn("vsnprintf: Output truncated")`
- Check return values explicitly: `if (!(fp = fopen(path, "r"))) return NULL`
- Include context function name: `warn("strftime: Result string exceeds buffer size")`
- Include specific reason: `die("XOpenDisplay: Failed to open display")`
- Message format: `function_name: description` or `system_call: description`
- Inverted logic common: `if (!(pw = getpwuid(geteuid()))) return NULL;`
- Guard allocations: all `malloc`/`fopen` results checked immediately
## Logging
- Error output to stderr via `warn()` function in `util.c`
- Info messages can go to stdout (not used much in suckless tools)
- `verr()` helper centralizes error formatting
## Comments
- Explain non-obvious algorithms or workarounds: See `cpu.c` line 31 for CPU stats parsing
- Clarify platform-specific code: `/* in kHz */`, `/* in MHz */`
- Mark configuration: `/* Tokyo Night colors for status2d */`
- Indicate known limitations: `/* FIXME not sure if I have to send these events, too */`
- Copyright header in every file: `/* See LICENSE file for copyright and license details. */`
- Multi-line comments for design notes at top of files (e.g., `dwm.c` lines 1-22)
- Single-line comments for inline explanations: `/* cpu user nice system idle iowait irq softirq */`
- Not used; C doesn't have standard format
- Function signatures serve as documentation via header files (`.h` files)
## Function Design
- Functions typically 20-50 lines; longer functions often indicate complex platform-specific logic
- Some exceptions in component modules for platform conditionals (up to 100+ lines for multi-platform support)
- Complexity preferred over premature abstraction
- Keep minimal: 1-3 parameters typical
- Use `const char *` for format strings and paths
- Unused parameters marked with `unused` identifier: `cpu_freq(const char *unused)`
- No variadic functions except utility wrappers (`warn()`, `die()`, `bprintf()`)
- Functions return `const char *` for display strings
- Return `NULL` on failure (universal pattern)
- Return `int` for status codes (usually count or error indicator)
- Multiple outputs achieved through pointer parameters (e.g., `memcpy(b, a, sizeof(b))`)
## Module Design
- Component modules export single or related set of functions via header files
- `slstatus.h` declares all public component functions
- Static helper functions kept private with `static` keyword
- Naming exports: `battery_perc()`, `battery_state()`, `cpu_freq()`, etc.
- Header file aggregation used: `slstatus.h` collects all component declarations
- Avoids circular dependencies by centralizing declarations
- Each component (e.g., `cpu.c`) fully self-contained
- Platform-specific code enclosed in `#if defined(...)` blocks
- Shared utilities in `util.c` and included via `util.h`
#include "../slstatus.h"
#include "../util.h"
#if defined(__linux__)
#elif defined(__OpenBSD__)
#elif defined(__FreeBSD__)
#endif
## Configuration Pattern
- Default configuration in `config.def.h`: contains all defaults
- User config created from defaults: `config.h` (copied from `.def.h`)
- Configuration loaded at compile time via `#include "config.h"`
- No runtime config files
#define MAXLEN 2048
## Macro Conventions
- `LEN(x)` for array length: `#define LEN(x) (sizeof(x) / sizeof((x)[0]))`
- Comparison macros: `MIN()`, `MAX()`
- Simple abstractions for code clarity
- Conditional compilation used extensively: `#if defined(__linux__)`
- Platform gates allow single codebase
- No feature flags—only OS/architecture detection
<!-- GSD:conventions-end -->

<!-- GSD:architecture-start source:ARCHITECTURE.md -->
## Architecture

## Pattern Overview
- Minimalist X11-based desktop environment components
- Configuration-driven customization via C header files (config.h)
- Event-driven architecture for window management and status display
- Lightweight shell script glue for system integration
- Clipboard and power management daemon patterns
## Layers
- Purpose: Core X11 window management, layout control, and client organization
- Location: `dwm/` - `dwm/dwm.c`, `dwm/config.h`, `dwm/drw.c`
- Contains: Window event handlers, layout engines (tile, float, monocle), monitor management, status bar
- Depends on: X11 libraries (Xlib, Xft), drawing abstraction (drw)
- Used by: All X11 applications, status bar display
- Purpose: System information aggregation and X11 root window title updates
- Location: `slstatus/` - `slstatus/slstatus.c`, `slstatus/config.h`, `slstatus/components/`
- Contains: Component modules for CPU, RAM, battery, network, disk, datetime monitoring
- Depends on: X11 (for setting root window name), system files (/proc, /sys), platform-specific APIs
- Used by: dwm status bar (reads root window name), system monitoring
- Purpose: Textual menu interface for program execution and user selection
- Location: `dmenu/` - `dmenu/dmenu.c`, `dmenu/config.h`, `dmenu/drw.c`
- Contains: Item filtering, text input, rendering, keyboard/mouse navigation
- Depends on: X11 libraries, drawing abstraction (drw)
- Used by: User keybindings, shell scripts for system operations
- Purpose: Lightweight terminal for shell interaction
- Location: `st/` - `st/st.c`, `st/config.h`
- Contains: PTY management, VT100 escape sequence parsing, text rendering
- Depends on: X11, system PTY interface, POSIX terminal APIs
- Used by: User shell sessions
- Purpose: System-level operations and daemon management
- Location: `scripts/`, `dwm-start`, `.xprofile`
- Contains: Clipboard daemon (dmenu-clipd), clipboard browser (dmenu-clip), CPU governor selector (dmenu-cpupower), session manager (dmenu-session)
- Depends on: Shell (sh), dmenu, system utilities (xclip, cpupower, systemctl, slock)
- Used by: dwm keybindings, X11 initialization
- Purpose: Desktop notification rendering and management
- Location: `dunst/dunstrc`
- Contains: Notification style, positioning, timeout rules, urgency levels
- Depends on: dunst daemon, X11
- Used by: Applications sending notifications
## Data Flow
- **dwm clients:** Linked list per monitor, sorted by focus history (stack)
- **dwm tags:** Bitmask per client (0-8), visibility based on seltags per monitor
- **dmenu-clipd:** File-based (MD5 hash as filename), on-disk state
- **slstatus:** Stateless iterator, reads live system state each cycle
- **X11 root window:** Central messaging point for window manager ↔ status display
## Key Abstractions
- Purpose: Represents an X11 window managed by dwm
- Examples: `dwm/dwm.c` lines 104-117
- Pattern: Fixed-size struct with pointers to next/stack, bitmask for tags, geometry tracking
- Purpose: Pluggable system info collector
- Examples: `slstatus/components/cpu.c`, `slstatus/components/ram.c`
- Pattern: Signature `const char *func(const char *arg)` returns pointer to static buffer
- Purpose: Tile arrangement algorithm
- Examples: `tile()`, `monocle()` in `dwm/dwm.c`
- Pattern: Takes monitor pointer, rearranges client geometries in-place
- Purpose: Graphics layer decoupling X11 details
- Examples: `dwm/drw.c`, `dmenu/drw.c` (shared implementation)
- Pattern: Functions like `drw_rect()`, `drw_text()`, `drw_map()` hide X11 Drawable operations
- Purpose: Maps input event to action with arguments
- Examples: `dwm/config.h` lines 85-131
- Pattern: Struct with modifier mask, key/button code, function pointer, Arg union
## Entry Points
- Location: `dwm/dwm.c` main() function (after reading `dwm/config.h`)
- Triggers: X11 session initialization via xdm/startx
- Responsibilities: Event dispatch loop, window lifecycle, layout management, keybinding execution
- Location: `slstatus/slstatus.c` main() function (after reading `slstatus/config.h`)
- Triggers: Called from `dwm-start` script in background loop with 1s sleep
- Responsibilities: Iterate component array, format status string, update X11 root window name
- Location: `dmenu/dmenu.c` main() function
- Triggers: User keybinding via dwm spawn(), or manual shell invocation
- Responsibilities: Read stdin/list items, filter by input, display menu, output selection
- Location: `scripts/dmenu-clipd` - infinite loop polling X11 clipboard
- Triggers: Started in background from `dwm-start`
- Responsibilities: Monitor clipboard changes, deduplicate by MD5 hash, maintain cache directory
- Location: `scripts/dmenu-clip` - wrapper around dmenu for clipboard selection
- Triggers: User keybinding via dwm (Super+v)
- Responsibilities: Format cached clipboard entries, invoke dmenu, restore selection
- Location: `scripts/dmenu-cpupower` - wrapper for CPU frequency scaling
- Triggers: User keybinding via dwm (Super+p)
- Responsibilities: Read current governor, show options in dmenu, apply via pkexec cpupower
- Location: `scripts/dmenu-session` - wrapper for system power operations
- Triggers: User keybinding via dwm (Ctrl+Alt+Delete)
- Responsibilities: Show session menu (logout/lock/reboot/shutdown), confirm, execute
- Location: `st/st.c` main() function (after reading `st/config.h`)
- Triggers: User keybinding via dwm (Super+Return)
- Responsibilities: Create PTY, fork shell, manage terminal state, render text
## Error Handling
- C utilities exit with die(msg) on fatal conditions (missing display, allocation failure)
- Shell scripts use `[ -z "$var" ] && exit [N]` for validation
- slstatus components return "n/a" string on read errors (configurable via unknown_str)
- dmenu-clipd silently skips non-text clipboard content and empty entries
- No retry logic; fail fast and let supervisors (systemd/scripts) restart as needed
## Cross-Cutting Concerns
<!-- GSD:architecture-end -->

<!-- GSD:skills-start source:skills/ -->
## Project Skills

No project skills found. Add skills to any of: `.claude/skills/`, `.agents/skills/`, `.cursor/skills/`, or `.github/skills/` with a `SKILL.md` index file.
<!-- GSD:skills-end -->

<!-- GSD:workflow-start source:GSD defaults -->
## GSD Workflow Enforcement

Before using Edit, Write, or other file-changing tools, start work through a GSD command so planning artifacts and execution context stay in sync.

Use these entry points:
- `/gsd-quick` for small fixes, doc updates, and ad-hoc tasks
- `/gsd-debug` for investigation and bug fixing
- `/gsd-execute-phase` for planned phase work

Do not make direct repo edits outside a GSD workflow unless the user explicitly asks to bypass it.
<!-- GSD:workflow-end -->



<!-- GSD:profile-start -->
## Developer Profile

> Profile not yet configured. Run `/gsd-profile-user` to generate your developer profile.
> This section is managed by `generate-claude-profile` -- do not edit manually.
<!-- GSD:profile-end -->
