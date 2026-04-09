# External Integrations

**Analysis Date:** 2026-04-09

## System Service Integrations

**X11 Display Server:**
- Primary integration point for all UI components
- Used by: dwm, st, dmenu, slstatus, dunst
- Headers: `<X11/Xlib.h>`, `<X11/XF86keysym.h>`, `<X11/keysym.h>`
- Files: `dwm/dwm.c`, `st/x.c`, `dmenu/dmenu.c`, `slstatus/slstatus.c`

**PulseAudio:**
- Audio volume control via `pactl` command
- Used by: dwm hotkeys, slstatus volume display
- Integration method: Shell command invocation
- Files: `dwm/config.h` (lines 9-11), `slstatus/config.h` (line 76)
- Commands executed:
  - `pactl set-sink-volume @DEFAULT_SINK@ ±5%`
  - `pactl set-sink-mute @DEFAULT_SINK@ toggle`
  - `pamixer --get-volume` (for volume display)
  - `pamixer --get-mute` (for mute status)

**Display Backlight:**
- Brightness control via `xbacklight` tool
- Used by: dwm hotkeys
- Integration method: Shell command invocation
- Files: `dwm/config.h` (lines 7-8)
- Commands executed:
  - `xbacklight -inc 5` (brightness up)
  - `xbacklight -dec 5` (brightness down)

**Systemd:**
- Power management (reboot, shutdown)
- Used by: dmenu-session script for system control
- Integration method: `systemctl` command
- Files: `scripts/dmenu-session` (lines 16, 20)
- Commands executed:
  - `systemctl reboot`
  - `systemctl poweroff`

**Input Method Framework (Fcitx5):**
- CJK (Chinese, Japanese, Korean) text input
- Started by: dwm-start initialization script
- Files: `dwm-start` (line 14)
- Integration method: Process launch (`fcitx5 -d` for daemonize)

**PolicyKit (polkit-gnome):**
- Privilege escalation for protected operations
- Used by: CPU power governor changes, system power control
- Files: `dwm-start` (line 13), `scripts/dmenu-cpupower` (line 18)
- Integration method: `pkexec` wrapper command
- Commands protected:
  - `pkexec cpupower frequency-set -g <governor>` (CPU governor change)
  - System power operations (reboot, shutdown require sudo)

## Hardware Control Integration

**CPU Frequency Scaling:**
- CPUpower utility for governor management
- Used by: dmenu-cpupower script for CPU power profile selection
- Files: `scripts/dmenu-cpupower` (lines 5, 18)
- Reads: `/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor`
- Supports governors: performance, schedutil (balanced), powersave (economy)

**Battery Status:**
- System battery monitoring for status display
- Used by: slstatus for battery indicator
- Files: `slstatus/config.h` (lines 74-75)
- Components: `slstatus/components/battery.c`
- Argument: `BAT1` (second battery device)
- Shows: battery percentage, charge state (+, -, o), remaining time

**Network Interfaces:**
- Network speed and connectivity monitoring
- Used by: slstatus WiFi/ethernet monitoring
- Files: `slstatus/components/netspeeds.c`, `slstatus/components/wifi.c`, `slstatus/components/ip.c`
- Monitors: interface speed (rx/tx), WiFi ESSID, signal strength, IPv4/IPv6 addresses

**Thermal Sensors:**
- CPU temperature monitoring
- Used by: slstatus for thermal status
- Files: `slstatus/components/temperature.c`
- Source: `/sys/class/thermal/` (Linux thermal zone files)

## Clipboard Integration

**X Selection Protocol:**
- X11 clipboard primary selection and copy/paste
- Used by: dmenu-clipd (monitoring), dmenu-clip (retrieval), xclip tools
- Files:
  - `scripts/dmenu-clipd` - Clipboard history daemon
    - Monitors: X11 clipboard with TARGETS check
    - Stores: MD5-hashed clipboard entries (up to 50)
    - Cache location: `${XDG_CACHE_HOME:-$HOME/.cache}/dmenu-clipboard`
    - Poll interval: 0.5 seconds
    - Timeout: 2 seconds for xclip operations
  - `scripts/dmenu-clip` - Clipboard history selector
    - Retrieves: Stored clipboard entries via dmenu interface
    - Preview: First 80 characters of each entry
    - Source: xclip for both read/write operations

**Screenshot Capture:**
- Image capture with dmenu selection interface
- Used by: Print key hotkey in dwm
- Files: `dwm/config.h` (line 101)
- Integration: `maim -s | xclip -selection clipboard -t image/png`

## Screen Locking

**Simple Lock (slock):**
- X11-based screen locker
- Used by: dmenu-session script
- Files: `scripts/dmenu-session` (line 13)
- Launch method: `slock` command
- No integration parameters needed (uses X11 directly)

## Wallpaper Management

**Feh Image Viewer:**
- Wallpaper setting
- Used by: dwm-start initialization
- Files: `dwm-start` (line 24)
- Command: `feh --no-fehbg --bg-fill ~/wallpapers/sushi_original.png`
- Wallpaper location: User home directory wallpapers folder

## Process Management

**Signal-based Communication:**
- Clean shutdown coordination between processes
- Used by: dmenu-session for graceful termination
- Files: `scripts/dmenu-session` (line 11)
- Signals: `pkill -x` (SIGTERM by default)
- Targets:
  - `slstatus` - Status monitor daemon
  - `dmenu-clipd` - Clipboard history daemon
  - `dwm` - Window manager (triggers X11 shutdown)

## Data Storage

**Filesystem-based:**
- Configuration files (`.h` headers as config, `dunstrc` INI format)
- Shell scripts stored in `scripts/` directory
- Clipboard cache stored in `$XDG_CACHE_HOME/dmenu-clipboard`
- Wallpaper images in user home `~/wallpapers/`
- Manual pages in `/usr/local/share/man/` (installed)

**No External Storage:**
- No database systems (SQLite, PostgreSQL, etc.)
- No remote API calls
- No cloud synchronization
- No web service dependencies

## Font Integration

**Fontconfig:**
- Font discovery and selection system
- Used by: dwm, st, dmenu, dunst for rendering
- Primary font: Iosevka Nerd Font (regular and Mono variants)
  - Used in: `dwm/config.h`, `st/config.h`, `dmenu/config.h`, `dunst/dunstrc`
  - Sizes: 14pt (dwm, dmenu), 16pt (st), 11pt (dunst)
- Fallback font: Roboto Mono for Powerline (st only)
- Support: Nerd Font icons for status indicators and UI glyphs

---

*Integration audit: 2026-04-09*
