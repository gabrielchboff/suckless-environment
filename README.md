# suckless-environment

A hardened, C-native utility suite for a dwm-based desktop environment targeting Arch Linux (systemd) and Artix Linux (OpenRC + elogind).

## Project Purpose

This project provides a complete desktop environment built on the suckless philosophy: minimal dependencies, compile-time configuration, and fast, safe C programs. The core value proposition is:

- **One install.sh works on both Arch and Artix** — no separate scripts, no conditional logic the user must understand
- **Every user-facing utility is a fast, safe C program** — no fragile shell scripts, no Python overhead, no runtime config files

The environment includes dwm (window manager), st (terminal), dmenu (application launcher), slstatus (status bar), and custom utilities for battery alerts, brightness control, clipboard management, session/power management, and CPU profile switching.

## Supported Distributions

- **Arch Linux** — systemd init system
- **Artix Linux** — OpenRC init system with elogind

No other distributions are supported. No Wayland, no BSD, no macOS.

## Quick Start

### One-Line Install

```bash
git clone https://github.com/YOUR_USERNAME/suckless-environment.git
cd suckless-environment
./install.sh
```

### Review Before Running

```bash
git clone https://github.com/YOUR_USERNAME/suckless-environment.git
cd suckless-environment
less install.sh   # review before running
./install.sh
```

The install script will:
1. Detect your distribution (Arch or Artix)
2. Install required dependencies via pacman
3. Build and install all suckless tools to `/usr/local/bin`
4. Build and install custom utilities to `~/.local/bin`
5. Copy configuration files to their proper locations

## Keybindings

The window manager uses **Super (Windows key)** as the primary modifier (MODKEY).

### Launch Applications

| Key | Action | Description |
|-----|--------|-------------|
| `Super+d` | dmenu_run | Launch application |
| `Super+Return` | st | Open terminal |
| `Super+e` | thunar | Open file manager |
| `Super+Shift+b` | brave | Launch browser |

### Window Management

| Key | Action | Description |
|-----|--------|-------------|
| `Super+j` | focusstack +1 | Focus next window |
| `Super+k` | focusstack -1 | Focus previous window |
| `Super+h` | setmfact -0.05 | Shrink master area |
| `Super+l` | setmfact +0.05 | Expand master area |
| `Super+z` | zoom | Bring window to master |
| `Super+q` | killclient | Close focused window |
| `Super+t` | setlayout tile | Tile layout |
| `Super+f` | setlayout float | Floating layout |
| `Super+m` | setlayout monocle | Monocle layout |
| `Super+space` | setlayout | Cycle layouts |
| `Super+Shift+space` | togglefloating | Toggle floating |

### Tag Navigation

| Key | Action | Description |
|-----|--------|-------------|
| `Super+[1-9]` | view | Switch to tag |
| `Super+0` | view | View all tags |
| `Super+Shift+[1-9]` | tag | Send window to tag |
| `Super+Shift+0` | tag | Send window to all tags |

### Monitor Navigation

| Key | Action | Description |
|-----|--------|-------------|
| `Super+,` | focusmon -1 | Focus previous monitor |
| `Super+.` | focusmon +1 | Focus next monitor |
| `Super+Shift+,` | tagmon -1 | Send window to previous monitor |
| `Super+Shift+.` | tagmon +1 | Send window to next monitor |

### System

| Key | Action | Description |
|-----|--------|-------------|
| `Super+b` | togglebar | Toggle status bar |
| `Super+Tab` | view | View last tag |
| `Super+v` | dmenu-clip | Clipboard history |
| `Super+p` | dmenu-cpupower | CPU power profile |
| `Ctrl+Alt+Delete` | dmenu-session | Session menu (lock/logout/reboot/shutdown) |
| `Super+Shift+q` | quit | Exit dwm |
| `Super+Ctrl+Shift+q` | quit (1) | Force quit dwm |

### Media Keys

| Key | Action | Description |
|-----|--------|-------------|
| `Print` | flameshot gui | Screenshot (region select, annotate, copy) |
| `Brightness Up` | brightness-notify up | Increase brightness |
| `Brightness Down` | brightness-notify down | Decrease brightness |
| `Volume Up` | pactl set-sink-volume +5% | Increase volume |
| `Volume Down` | pactl set-sink-volume -5% | Decrease volume |
| `Volume Mute` | pactl set-sink-mute toggle | Toggle mute |

## Utilities

### battery-notify
Monitors battery level and sends notifications via dunst when low. Designed to run from cron or a timer.

### brightness-notify
Adjusts display brightness using brightnessctl and shows an OSD notification.

### dmenu-clip
Clipboard history browser. Shows cached clipboard entries via dmenu, lets you restore any entry.

### dmenu-clipd
Clipboard daemon that watches for clipboard changes and caches them to disk. Runs as a background service.

### dmenu-cpupower
CPU power profile selector. Uses power-profiles-daemon to switch between performance, balanced, and power-saving modes.

### dmenu-session
Session menu for lock screen, logout, reboot, and shutdown. Uses loginctl for session management.

## Troubleshooting

### Issue 1: "command not found" for dependencies

**Symptom:** After running install.sh, you get "command not found" errors when trying to run tools.

**Diagnosis:**
```bash
which dmenu st dwm slstatus   # check if binaries are installed
echo $PATH                    # verify ~/.local/bin is in PATH
```

**Fix:** Ensure `base-devel` is installed:
```bash
sudo pacman -Syu base-devel
```

If using bash, add to `~/.bashrc`:
```bash
export PATH="$HOME/.local/bin:$PATH"
```

### Issue 2: AUR helper fails

**Symptom:** install.sh fails when trying to build AUR packages (betterlockscreen, brave-bin).

**Diagnosis:**
```bash
which yay paru   # check if AUR helper is installed
```

**Fix:** Install yay or paru manually first:
```bash
# Option 1: Install yay
sudo pacman -Syu git && git clone https://aur.archlinux.org/yay.git && cd yay && makepkg -si

# Option 2: Install paru
sudo pacman -Syu git && git clone https://aur.archlinux.org/paru.git && cd paru && makepkg -si

# Option 3: Use pacman directly for dependencies
sudo pacman -Syu --noconfirm <package>
```

### Issue 3: brightnessctl permissions

**Symptom:** Brightness keys don't work, or brightnessctl returns "No devices found".

**Diagnosis:**
```bash
brightnessctl -l                    # list available devices
groups                              # check your groups
ls -la /sys/class/backlight/        # check backlight permissions
```

**Fix:** Add user to video group and ensure udev rules are applied:
```bash
sudo usermod -aG video $USER
# Log out and back in for group membership to take effect

# If using brightnessctl with setuid, verify:
ls -la /usr/bin/brightnessctl
```

### Issue 4: Session doesn't start

**Symptom:** After install, running startx or entering desktop from display manager doesn't show dwm.

**Diagnosis:**
```bash
cat ~/.xprofile
cat ~/.local/bin/dwm-start
```

**Fix:** Ensure dwm-start is executable and referenced:
```bash
chmod +x ~/.local/bin/dwm-start
# In ~/.xprofile or as xinitrc:
exec ~/.local/bin/dwm-start
```

### Issue 5: dmenu utilities not found

**Symptom:** Keybindings like Super+v, Super+p don't open menus.

**Diagnosis:**
```bash
which dmenu-clip dmenu-cpupower dmenu-session
ls -la ~/.local/bin/
```

**Fix:** Build and install utils:
```bash
cd utils
make
make install
```

## Contributing

For contributor documentation, AI assistant context, and project roadmap, see:

- [CLAUDE.md](./CLAUDE.md) — AI assistant context and project specifications
- [.planning/](.planning/) — Project roadmap, phase documentation, and requirements traceability

## Screenshots

![dwm bar with slstatus](docs/screenshots/screenshot-dwm-bar.png)
*Status bar showing CPU, RAM, battery, and other metrics via slstatus*

![dmenu application launcher](docs/screenshots/screenshot-dmenu.png)
*dmenu open with filtered program list*

![dunst notification](docs/screenshots/screenshot-dunst.png)
* dunst notification example*

## License

See individual LICENSE files in each subdirectory (dwm/, st/, dmenu/, slstatus/, utils/).
