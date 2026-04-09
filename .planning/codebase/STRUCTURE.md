# Codebase Structure

**Analysis Date:** 2026-04-09

## Directory Layout

```
/home/void/.config/suckless-environment/
├── backup/                          # Backup of original config.def.h files from suckless
│   ├── dmenu/
│   ├── dwm/
│   ├── slstatus/
│   └── st/
├── dwm/                             # Dynamic Window Manager source
│   ├── config.h                     # Customized keybindings, layout, colors
│   ├── config.def.h                 # Default configuration (reference)
│   ├── dwm.c                        # Main window manager implementation
│   ├── drw.c                        # Drawing/rendering implementation
│   ├── drw.h                        # Drawing interface
│   ├── transient.c                  # Transient window handling
│   ├── util.c                       # Utility functions
│   ├── util.h                       # Utility interface
│   └── arg.h                        # Argument parsing macro
├── dmenu/                           # Menu/launcher implementation
│   ├── config.h                     # Customized colors and keybindings
│   ├── config.def.h                 # Default configuration
│   ├── dmenu.c                      # Main menu implementation
│   ├── stest.c                      # Test utility
│   ├── drw.c                        # Drawing implementation
│   ├── drw.h                        # Drawing interface
│   ├── util.c                       # Utility functions
│   ├── util.h                       # Utility interface
│   └── arg.h                        # Argument parsing macro
├── st/                              # Simple Terminal implementation
│   ├── config.h                     # Customized terminal settings
│   ├── config.def.h                 # Default configuration
│   ├── st.c                         # Main terminal emulator
│   ├── x.c                          # X11 integration
│   └── [other terminal files]
├── slstatus/                        # Status bar information provider
│   ├── config.h                     # Customized status modules and format
│   ├── config.def.h                 # Default configuration
│   ├── slstatus.c                   # Main status collector loop
│   ├── slstatus.h                   # Component interface definitions
│   ├── util.c                       # Utility functions
│   ├── util.h                       # Utility interface
│   ├── arg.h                        # Argument parsing macro
│   └── components/                  # Pluggable system info modules
│       ├── battery.c                # Battery percentage and state
│       ├── cat.c                    # Read arbitrary files
│       ├── cpu.c                    # CPU frequency and usage
│       ├── datetime.c               # Date/time formatting
│       ├── disk.c                   # Disk usage statistics
│       ├── entropy.c                # System entropy
│       ├── hostname.c               # System hostname
│       ├── ip.c                     # IP address (IPv4/IPv6)
│       ├── keyboard_indicators.c    # Caps/Num lock status
│       ├── keymap.c                 # Keyboard layout
│       ├── load_avg.c               # CPU load average
│       ├── netspeeds.c              # Network RX/TX speeds
│       ├── num_files.c              # File count in directory
│       ├── ram.c                    # RAM usage statistics
│       ├── run_command.c            # Execute arbitrary shell commands
│       ├── swap.c                   # Swap usage statistics
│       ├── temperature.c            # CPU/system temperature
│       ├── uptime.c                 # System uptime
│       ├── user.c                   # User/UID/GID info
│       ├── volume.c                 # Audio volume (ALSA/OSS)
│       └── wifi.c                   # WiFi ESSID and signal strength
├── dunst/                           # Notification daemon configuration
│   └── dunstrc                      # Dunst notification settings
├── scripts/                         # Custom shell scripts
│   ├── dmenu-clip                   # Clipboard history browser
│   ├── dmenu-clipd                  # Clipboard history daemon
│   ├── dmenu-cpupower               # CPU governor selector menu
│   └── dmenu-session                # Power/session management menu
├── .planning/                       # GSD planning documents (generated)
│   └── codebase/
│       ├── ARCHITECTURE.md
│       └── STRUCTURE.md (this file)
├── .claude/                         # Claude workspace metadata
├── .xprofile                        # X11 session initialization
├── dwm-start                        # DWM session startup script
├── install.sh                       # Installation and build script
├── .gitignore                       # Git ignore patterns
├── LICENSE                          # License file
├── README.md                        # Project readme
└── .git/                            # Git repository
```

## Directory Purposes

**dwm/:**
- Purpose: Window manager source code and configuration
- Contains: C implementation files, configuration header, build files
- Key files: `config.h` (keybindings, colors, layout), `dwm.c` (main logic), `drw.c` (rendering)

**dmenu/:**
- Purpose: Application launcher and menu interface source
- Contains: C implementation files, configuration header, drawing utilities
- Key files: `config.h` (colors, fonts), `dmenu.c` (main logic), `drw.c` (rendering)

**st/:**
- Purpose: Terminal emulator source code
- Contains: C implementation files, configuration header
- Key files: `config.h` (fonts, colors), `st.c` (terminal logic), `x.c` (X11 integration)

**slstatus/:**
- Purpose: Status bar information provider and component modules
- Contains: Main status loop, pluggable component modules for system monitoring
- Key files: `config.h` (which modules to display), `slstatus.c` (main loop), `components/` (individual monitors)

**scripts/:**
- Purpose: Custom shell script utilities for system operations
- Contains: Clipboard management, CPU governor control, session management
- Key files: `dmenu-clipd` (daemon), `dmenu-clip` (UI), `dmenu-cpupower`, `dmenu-session`

**dunst/:**
- Purpose: Desktop notification daemon configuration
- Contains: Notification styling, urgency rules, display settings
- Key files: `dunstrc` (main config with color scheme and behavior)

**.xprofile:**
- Purpose: X11 session initialization environment variables
- Contains: Clipboard manager configuration for dmenu
- Key files: Sets CM_LAUNCHER and CM_LAUNCHER_OPTS for clipboard integration

**dwm-start:**
- Purpose: X11 session startup script that launches all daemons
- Contains: Daemon initialization (polkit, fcitx5, dunst, slstatus), background setting
- Key files: Single executable script

**install.sh:**
- Purpose: Automated build and installation script
- Contains: Dependency checking, build compilation for all suckless tools, script installation, config copying
- Key files: Single script calling make in each tool directory

## Key File Locations

**Entry Points:**
- `dwm/dwm.c`: Window manager entry (spawned by X11 session)
- `slstatus/slstatus.c`: Status display entry (spawned from dwm-start)
- `dmenu/dmenu.c`: Menu launcher entry (spawned on-demand by dwm keybindings)
- `st/st.c`: Terminal entry (spawned by dwm Super+Return)
- `scripts/dmenu-clipd`: Clipboard daemon (spawned from dwm-start)
- `dwm-start`: X11 session entry point (replaces /usr/bin/dwm for custom startup)

**Configuration:**
- `dwm/config.h`: Window manager keybindings, colors, fonts, layouts, window rules
- `dmenu/config.h`: Menu colors, fonts
- `st/config.h`: Terminal fonts, colors, scrollback
- `slstatus/config.h`: Status bar module selection and format strings
- `dunst/dunstrc`: Notification styling and rules
- `.xprofile`: Environment setup for X11 session

**Core Logic:**
- `dwm/dwm.c`: Window lifecycle, event handling, layout management (3000+ lines)
- `slstatus/slstatus.c`: Main loop for status collection and X11 updates (135 lines, modular via config)
- `dmenu/dmenu.c`: Menu filtering and rendering (400+ lines)
- `st/st.c`: Terminal state machine and VT100 parsing (2000+ lines)

**Testing:**
- No formal test framework; suckless philosophy emphasizes code review and manual testing
- Each tool built with `make` from its directory; `make install` with sudo

**Shared Code:**
- `drw.c` / `drw.h`: Rendering abstraction shared between dwm and dmenu
- `util.c` / `util.h`: Common utilities (strdup, die, etc.) per tool
- `arg.h`: Macro for argument parsing - shared by all tools

## Naming Conventions

**Files:**
- C source: lowercase with underscores (`slstatus.c`, `keyboard_indicators.c`)
- Headers: `.h` extension, same naming as source (drw.h, util.h)
- Config: `config.h` for customized, `config.def.h` for defaults
- Scripts: lowercase with hyphens (`dmenu-clip`, `dmenu-cpupower`)
- Shell scripts: no extension, executable (+x)

**Directories:**
- Component modules: `components/` subdirectory in slstatus
- Backup copies: `backup/` directory preserves original configs
- Planning output: `.planning/codebase/` for generated documentation

**Functions:**
- Snake_case: `cpu_perc()`, `battery_state()`, `difftimespec()`
- Initialization: `init_*()` pattern (e.g., `initroot()` in dwm)
- Cleanup: `cleanup()` in main tools

**Variables:**
- Global state: lowercase with underscores (`struct arg args[]`, `static Display *dpy`)
- Constants: UPPERCASE with underscores in macros (#define BUTTONMASK, #define MAXLEN)
- Temporary/local: lowercase (`i`, `j`, `x`, `y`)

**Types:**
- Structs: CamelCase (`Monitor`, `Client`, `Button`, `Key`)
- Enums: lowercase in enum declaration, UPPERCASE values if #define-style
- Typedefs: CamelCase for structs (Client, Monitor), lowercase for function pointers

## Where to Add New Code

**New Feature in dwm (e.g., new keybinding):**
- Primary code: `dwm/config.h` - add to keys[] array
- If new function needed: `dwm/dwm.c` above or near related functions
- Tests: Manual testing by rebuilding and running `sudo make install` in dwm/

**New status module in slstatus:**
- New component: Create `slstatus/components/mymodule.c`
- Interface: Add declaration to `slstatus/slstatus.h` following pattern `const char *mymodule(const char *arg);`
- Configuration: Reference in `slstatus/config.h` args[] array
- Tests: Manual verification after rebuild

**New dmenu script integration:**
- Shell script: Create `scripts/dmenu-myfeature` with proper shebang and chmod +x
- Integration: Reference from `dwm/config.h` in keys[] array as keybinding
- Installation: Add to install.sh mkdir/cp/chmod sections

**New system utility/helper:**
- Shared if used by multiple tools: `dwm/util.c` or per-tool util.c
- Tool-specific: Keep in respective tool directory
- Follow existing util.h interface pattern

**Utilities:**
- Shared helpers: `dwm/util.c` and `slstatus/util.c` (copies, not linked)
- Drawing code: `dwm/drw.c` and `dmenu/drw.c` (identical implementations)
- Include guards: `#ifndef DRW_H` / `#define DRW_H` pattern

## Special Directories

**backup/:**
- Purpose: Archive of original config.def.h from upstream suckless repos
- Generated: No, manually copied
- Committed: Yes - reference for tracking customizations
- Use: Compare against config.def.h to see what was changed from defaults

**.planning/:**
- Purpose: GSD codebase mapping output
- Generated: Yes, by `/gsd-map-codebase` command
- Committed: Yes - used by `/gsd-plan-phase` and `/gsd-execute-phase`
- Contains: ARCHITECTURE.md, STRUCTURE.md, and other analysis docs

**.claude/:**
- Purpose: Claude Code workspace metadata
- Generated: Yes, by Claude Code editor
- Committed: Likely (if .gitignore doesn't exclude)
- Use: Editor configuration, context preservation

**.git/:**
- Purpose: Version control repository
- Generated: Yes, by `git init`
- Committed: Yes - entire .git/ folder
- Recent commits track feature additions (notifications, CPU/clipboard utilities)

---

*Structure analysis: 2026-04-09*
