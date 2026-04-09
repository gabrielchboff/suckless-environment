# Suckless Environment v2

## What This Is

A hardened, C-native utility suite for a dwm-based Arch Linux desktop environment. Replaces fragile shell scripts with compiled C programs that integrate with dmenu, dunst, and X11 — while fixing broken functionality (lock screen, CPU profiles) and adding missing features (battery alerts, screenshot notifications).

## Core Value

Every user-facing utility is a fast, safe, native C program that works reliably without runtime dependencies on shell interpreters.

## Requirements

### Validated

- ✓ dwm + st + dmenu + slstatus build pipeline — existing
- ✓ dunst notification system with Tokyo Night theme — existing
- ✓ Clipboard history (store/retrieve via dmenu) — existing
- ✓ Session management menu (logout/lock/reboot/shutdown) — existing
- ✓ CPU profile selector via dmenu — existing
- ✓ Installer script for Arch Linux — existing

### Active

- [ ] Fix lock screen: use betterlockscreen (blur 0.8, duplicate prevention, DISPLAY export)
- [ ] Fix CPU profiles: use powerprofilesctl (performance/balanced/power-saver)
- [ ] Battery notification: one-shot dunst alert when charge ≤20%
- [ ] Screenshot notification: maim area capture → clipboard, dunst "Image copied to clipboard"
- [ ] Convert dmenu-session to C
- [ ] Convert dmenu-cpupower to C
- [ ] Convert dmenu-clip to C
- [ ] Convert dmenu-clipd to C with X11 clipboard events (replace polling)
- [ ] Convert battery-notify to C
- [ ] Convert screenshot-notify to C
- [x] Per-tool directory structure under utils/ — Phase 1
- [x] Updated install.sh: compile C utilities, handle new dependencies — Phase 1

### Out of Scope

- dwm/st/dmenu/slstatus source modifications — separate concern
- Wayland support — X11 only
- Multi-monitor betterlockscreen config beyond --display 1 — single display target
- Interactive screenshot tools (annotation, cropping UI) — just capture and notify

## Context

- Arch Linux with pacman + AUR (yay/paru)
- Suckless ecosystem: dwm, st, dmenu, slstatus — all built from patched source
- Tokyo Night color scheme throughout (dunst, dmenu)
- dwm-start is the session launcher (stays as shell script)
- Current scripts work but are fragile: slock doesn't match desired UX, cpupower governor names don't map to powerprofilesctl, clipboard daemon wastes CPU polling
- X11 provides clipboard change events (XFixes extension) — much more efficient than polling xclip

## Constraints

- **Platform**: Arch Linux only — pacman/AUR package names
- **Philosophy**: Suckless-style C — minimal dependencies, no frameworks
- **Display server**: X11 — all utilities use Xlib/XFixes directly where needed
- **Deps**: betterlockscreen (AUR), power-profiles-daemon (pacman), maim + xclip (pacman)
- **Build**: Each util has its own Makefile, install.sh orchestrates everything

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Replace slock with betterlockscreen | Better UX (blur, wallpaper-based lock) | — Pending |
| Replace cpupower with powerprofilesctl | Modern power management, simpler interface | — Pending |
| X11 events for clipboard daemon | Eliminates CPU-wasting 0.5s poll loop | — Pending |
| Per-tool directories under utils/ | Clean separation, individual Makefiles | — Pending |
| dwm-start stays as shell script | Launcher role — shell is appropriate for spawning processes | — Pending |
| All utilities compiled C | Speed, safety, no shell interpreter dependency | — Pending |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition** (via `/gsd-transition`):
1. Requirements invalidated? → Move to Out of Scope with reason
2. Requirements validated? → Move to Validated with phase reference
3. New requirements emerged? → Add to Active
4. Decisions to log? → Add to Key Decisions
5. "What This Is" still accurate? → Update if drifted

**After each milestone** (via `/gsd-complete-milestone`):
1. Full review of all sections
2. Core Value check — still the right priority?
3. Audit Out of Scope — reasons still valid?
4. Update Context with current state

---
*Last updated: 2026-04-09 after Phase 1 completion*
