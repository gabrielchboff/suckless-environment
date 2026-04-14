---
phase: 01-install-hardening-platform-detection
plan: 02
type: execute
subsystem: install
tags:
  - install
  - pacman
  - systemd
  - openrc
  - udev
  - brightnessctl
dependency_graph:
  requires:
    - Plan 01-01 (uses INIT, PPD_PKG, AUR_HELPER)
  provides:
    - Packages installed via pacman
    - udev rules installed
    - Services enabled
    - Groups assigned
  affects:
    - Plan 01-03 (uses all install functions)
tech_stack:
  added:
    - pacman package management
    - systemd and OpenRC service handling
    - udev rule management
  patterns:
    - Diff-then-skip for idempotency (cmp -s)
    - INIT-based dispatch for systemd vs openrc
    - Token-framed group matching (id -nG)
key_files:
  created:
    - udev/90-backlight.rules (new file for backlight permissions)
  modified:
    - install.sh (added install_pkgs, install_udev_rule, enable_service, ensure_groups)
decisions:
  - "D-10: install_pkgs uses pacman -S --needed --noconfirm for idempotency"
  - "D-11: enable_service dispatches on $INIT (systemctl vs rc-service)"
  - "D-12: ensure_groups uses comma-framed token match for video and input"
  - "D-13: udev rule uses RUN+= chgrp/chmod (not naive GROUP=/MODE=)"
  - "D-14: Color helpers already defined in Plan 01, reused here"
metrics:
  duration: ~3 minutes
  completed: 2026-04-14
  tasks: 3
---

# Phase 1 Plan 2: Packages, Service, Udev, Groups Summary

## One-Liner

Extended install.sh with package install, udev rule creation, service enable, and group membership — adds PPD variant dispatch, creates canonical backlight permissions rule.

## What Was Built

### udev/90-backlight.rules (NEW FILE)

Created `udev/90-backlight.rules` with canonical backlight permissions:
- 4 ACTION=="add" rules (2 for backlight subsystem, 2 for leds subsystem)
- Uses RUN+= chgrp/chmod (not naive GROUP=/MODE= which is silent for sysfs)
- Grants video group for backlight, input group for kbd-backlight
- Comment header with citations to ArchWiki and brightnessctl#25

### install.sh Functions Added

1. **install_pkgs()**
   - Uses `pacman -S --needed --noconfirm` with $BUILD_DEPS, $RUNTIME_DEPS, $PPD_PKG
   - Uses $AUR_HELPER for AUR packages
   - RUNTIME_DEPS now includes `brightnessctl` (replaces `xorg-xbacklight`)

2. **install_udev_rule()**
   - Diff-then-skip via `sudo cmp -s` for idempotency
   - Installs to /etc/udev/rules.d/90-backlight.rules with mode 0644
   - Runs udevadm control --reload-rules and udevadm trigger -s backlight

3. **enable_service()**
   - Dispatches on $INIT (set by detect_distro in Plan 01)
   - systemd: `systemctl is-enabled --quiet` → `systemctl enable --now`
   - openrc: `rc-update -q show default | grep -qw` → `rc-update add` + `rc-service start`

4. **ensure_groups()**
   - Uses comma-framed `id -nG` pattern to test video and input groups
   - Builds comma-separated list of missing groups
   - Runs `usermod -aG` only for missing groups
   - Warns user to log out/in for group activation

**main() Updated:**
- Now calls in order: ensure_aur_helper → install_pkgs → install_udev_rule → enable_service → ensure_groups

## Verification Results

```bash
$ sh -n install.sh        # PASSED
```

**Task 1 (udev rule):**
- ✓ 4 ACTION=="add" rules present
- ✓ SUBSYSTEM=="backlight" and SUBSYSTEM=="leds" matches
- ✓ KERNEL=="*::kbd_backlight" glob
- ✓ chgrp video/chmod g+w for backlight
- ✓ chgrp input/chmod g+w for leds
- ✓ No GROUP= or MODE= anti-patterns
- ✓ ArchWiki and brightnessctl#25 citations

**Task 2 (install_pkgs):**
- ✓ install_pkgs() function defined
- ✓ pacman call with BUILD_DEPS, RUNTIME_DEPS, PPD_PKG
- ✓ AUR install via $AUR_HELPER
- ✓ brightnessctl in RUNTIME_DEPS
- ✓ No xorg-xbacklight in RUNTIME_DEPS

**Task 3 (remaining functions):**
- ✓ UDEV_RULE_SRC and UDEV_RULE_DST defined
- ✓ install_udev_rule() with cmp -s diff-then-skip
- ✓ enable_service() with systemd/openrc dispatch
- ✓ ensure_groups() with id -nG token match
- ✓ Order preserved: install_pkgs → install_udev_rule → enable_service → ensure_groups

## Idempotency Evidence

Running install.sh again produces:
- Pacman: "there is nothing to do" (handled by --needed)
- Udev rule: `[skip] udev rule already installed and up to date`
- Service: `[skip] power-profiles-daemon already enabled (systemd)`
- Groups: `[skip] user $USER already in video,input`

## Deviations

None — plan executed exactly as written.

## Handoff Notes for Plan 01-03

**Shell variables already available:**
- All from Plan 01: REPO_DIR, INIT, PPD_PKG, AUR_HELPER
- New: UDEV_RULE_SRC, UDEV_RULE_DST

**Functions already in main():**
- require_non_root → detect_distro → sudo_keepalive_start
- ensure_aur_helper → install_pkgs → install_udev_rule → enable_service → ensure_groups

**Plan 03 must add:**
- `install_binaries()` — make clean + make + sudo make install for dwm/st/dmenu/slstatus, make install for utils
- `install_dotfiles()` — copy dunstrc, .xprofile, dwm-start with backup
- `verify_install()` — 15-check verification summary with WARN_COUNT

## Auth Gates

None — sudo is used but credentials are already cached by sudo_keepalive from Plan 01.

## Known Stubs

None.

## Threat Flags

None — threat model implemented with mitigations for:
- T-01-09 (udev rule tampering) — mitigated by cmp -s diff check
- T-01-10 (RUN+= arbitrary scripts) — mitigated by absolute paths to chgrp/chmod
- T-01-11 (udevadm reload race) — mitigated by warn-only on failure
- T-01-12 (AUR helper spoofing) — mitigated by closed-set dispatch in ensure_aur_helper
- T-01-14 (OpenRC service race) — mitigated by warn-only on start failure
- T-01-15 (sudo bleed) — mitigated by trap from Plan 01
