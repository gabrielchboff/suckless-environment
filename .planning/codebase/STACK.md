# Technology Stack

**Analysis Date:** 2026-04-09

## Languages

**Primary:**
- C (C99) - Window manager, terminal emulator, menu system, status monitor
- Shell (POSIX sh) - Installation scripts, automation, clipboard management, power control

**Build:**
- POSIX Make - All suckless tools use Makefiles with config.mk pattern

## Runtime

**Environment:**
- X11 (Xlib) - Display server foundation
- Linux (Arch/Void with pacman) - Target operating system

**Package Manager:**
- pacman - Arch Linux package manager
- AUR helpers (yay/paru) - Community repository packages
- Manual builds - Suckless tools built from source

## Frameworks

**Core Applications:**
- dwm 6.8 - Dynamic window manager
- st - Simple terminal emulator
- dmenu - Dynamic menu utility
- slstatus 1.1 - Status monitor daemon
- dunst - Notification daemon
- slock - Simple screen locker

**Build System:**
- Standard POSIX Make - All components use identical Makefile pattern

**Supporting Libraries:**
- X11/Xlib - X11 protocol client library
- libxft - Font rendering for X11
- libxinerama - Multi-monitor support
- freetype2 - Font rasterization
- fontconfig - Font configuration
- libpulse - PulseAudio client (for volume control)

## Key Dependencies

**Critical Build:**
- `base-devel` - GNU toolchain (gcc, make, etc.)
- `libxft` - Font rendering in X11
- `libxinerama` - Multi-monitor support for dwm
- `freetype2` - TrueType font support
- `fontconfig` - Font discovery and configuration
- `xorg-server` - X11 server (if not already installed)
- `xorg-xinit` - X11 initialization scripts (startx, xinitrc)

**Critical Runtime:**
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

**AUR Packages:**
- `brave-bin` - Brave browser pre-compiled (referenced in install.sh, but not used in WM config)

## Configuration

**Environment:**
- `.xprofile` - X session environment setup
  - `CM_LAUNCHER` - Set to `dmenu` for clipboard manager
  - `CM_LAUNCHER_OPTS` - Custom dmenu appearance with Tokyo Night colors, 10-line vertical menu
- `~/.local/bin/` - User scripts directory (added to PATH in dwm-start)

**Build Configuration:**
- `dwm/config.mk` - DWM build settings
  - X11 include/lib paths
  - Xinerama support enabled
  - FreeType font support enabled
  - Optimization: `-Os` (size), no debugging symbols
- `slstatus/config.mk` - slstatus build settings
  - Standard POSIX C99 compilation
  - X11 linking with `-s` stripping
- `st/config.mk` - Simple Terminal build settings
- `dmenu/config.mk` - dmenu build settings

**Runtime Configuration:**
- `dwm/config.h` - Window manager bindings, colors, tags, layouts
  - Hotkeys: brightness, volume, CPU power, clipboard, session
  - Tokyo Night color scheme (#1a1b26 bg, #a9b1d6 fg, #7aa2f7 blue)
  - Japanese tags: 一 through 九 (numerals one through nine)
  - Modifier: Win key (Super/Mod4)
- `st/config.h` - Terminal appearance and behavior
  - Font: Iosevka Nerd Font Mono 16pt
  - Fallback: Roboto Mono for Powerline 12pt
  - TERM: st-256color
  - Shell: /bin/sh
- `dmenu/config.h` - Menu appearance
  - Font: Iosevka Nerd Font 14pt
  - Bottom bar placement
  - Centered mode supported
  - Tokyo Night colors
  - 5-line default vertical mode
- `slstatus/config.h` - Status bar components and formatting
  - Update interval: 1000ms
  - Components: CPU %, RAM, battery, volume, date/time
  - Display format uses status2d color codes
  - Japanese labels for status indicators

**System Configuration:**
- `dunst/dunstrc` - Notification daemon configuration
  - Position: top-right, 10px offset
  - Size: 300px wide, up to 300px tall
  - Font: Iosevka Nerd Font 11pt
  - Tokyo Night colors with transparency
  - Urgency levels: low (5s), normal (10s), critical (0s/persistent)
  - Corner radius: 8px
  - Icon support: 32-48px range

## Platform Requirements

**Development:**
- Linux with X11 display server
- C99-compliant C compiler (gcc recommended)
- POSIX-compatible shell (sh, bash)
- Standard GNU tools (make, sed, awk, etc.)
- Full development headers (libxft-dev, freetype2-dev, etc.)

**Production:**
- X11 server running (not required if using Wayland-incompatible setup)
- Arch Linux / Void Linux (uses pacman)
- Install with: `cd /home/void/.config/suckless-environment && ./install.sh`
- Built binaries installed to `/usr/local/bin/`
- Manual pages installed to `/usr/local/share/man/`

---

*Stack analysis: 2026-04-09*
