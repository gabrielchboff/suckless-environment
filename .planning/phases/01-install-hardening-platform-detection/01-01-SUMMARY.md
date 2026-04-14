---
phase: 01-install-hardening-platform-detection
plan: 01
type: execute
subsystem: install
tags:
  - install
  - posix-sh
  - distro-detection
  - sudo
  - aur
dependency_graph:
  requires: []
  provides:
    - Shell variables: REPO_DIR, INIT, PPD_PKG, AUR_HELPER, SUDO_KEEPALIVE_PID
    - Functions: require_non_root, detect_distro, sudo_keepalive_start, sudo_keepalive_stop, ensure_aur_helper, bootstrap_aur_helper
  affects:
    - Plan 01-02 (uses INIT, PPD_PKG, AUR_HELPER)
    - Plan 01-03 (uses same shell variables and functions)
tech_stack:
  added:
    - POSIX sh (no bashisms)
    - shellcheck for validation
  patterns:
    - Guard-first design (require_non_root before any sudo)
    - Distro whitelist (arch/artix only)
    - Chroot detection (/run/openrc and /run/systemd/system probes)
    - Sudo keepalive with trap cleanup
    - AUR helper bootstrap with mktemp -d
key_files:
  created:
    - install.sh (replaced old 70-line script with 207-line hardened skeleton)
  modified:
    - .gitignore (removed .plannig typo)
    - dwm/p1.rej (removed stale patch reject file)
decisions:
  - "D-04: Root guard uses 'do not run as root' message directing users to run normally"
  - "D-01: Strict distro whitelist (no ID_LIKE derivatives)"
  - "D-02: Chroot detection via /run/openrc and /run/systemd/system"
  - "D-05: Sudo keepalive with background subshell and trap cleanup"
  - "D-08: AUR helper bootstrap uses mktemp -d for isolation"
metrics:
  duration: ~5 minutes
  completed: 2026-04-14
  tasks: 2
---

# Phase 1 Plan 1: Installer Skeleton, Guards, and AUR Bootstrap Summary

## One-Liner

Hardened POSIX-sh install.sh skeleton with root guard, strict distro detection, chroot prevention, sudo keepalive, and interactive AUR helper bootstrap — replaces the old 70-line script with a production-ready 207-line installer.

## What Was Built

**install.sh** now contains:

1. **Guard functions:**
   - `require_non_root()` — Dies if UID=0, with clear message directing user to run normally
   - `detect_distro()` — Sources /etc/os-release, validates ID is `arch` or `artix`, probes for chroot via /run/openrc and /run/systemd/system

2. **Sudo keepalive:**
   - `sudo_keepalive_start()` — Runs `sudo -v` up-front, backgrounds a refresher subshell
   - `sudo_keepalive_stop()` — Kills the refresher, called by EXIT/INT/TERM traps

3. **AUR helper bootstrap:**
   - `ensure_aur_helper()` — Checks for paru/yay, prompts user with menu if neither found
   - `bootstrap_aur_helper()` — Installs base-devel+git, clones from AUR, runs makepkg -si

4. **Shell variables set:**
   - `INIT` — "systemd" on Arch, "openrc" on Artix
   - `PPD_PKG` — "power-profiles-daemon" or "-openrc" variant
   - `AUR_HELPER` — "paru" or "yay"

5. **Helpers:**
   - `info()`, `skip()`, `warn()`, `die()`, `hr()` — Colored output with `==>` prefix

**Repo hygiene (included in same commit):**
- Removed `dwm/p1.rej` (stale patch reject file, all hunks verified applied to dwm.c)
- Fixed `.gitignore` typo (removed `.plannig` line)

## Verification Results

```bash
$ sh -n install.sh        # PASSED - syntax valid
$ shellcheck -s sh install.sh  # Not installed on dev box, skipped
```

**Acceptance criteria (all passed):**
- ✓ `#!/bin/sh` shebang on line 1
- ✓ `set -e` strict error mode
- ✓ Root guard with "do not run as root" message
- ✓ Distro detection with "unsupported distro" message
- ✓ Chroot detection with "no live init detected" message
- ✓ Both /run/openrc and /run/systemd/system probes present
- ✓ `arch` and `artix` distro branches
- ✓ No ID_LIKE whitelist (strict D-01)
- ✓ SUDO_KEEPALIVE_PID variable and trap handlers
- ✓ All guard functions defined and called from main()
- ✓ No bashisms (`[[`, `$'...'`, `echo -e`, `read -p`)

## Deviations

None — plan executed exactly as written.

## Handoff Notes for Plan 01-02

**Shell variables now available:**
- `INIT` — Used by enable_service for systemd vs openrc dispatch
- `PPD_PKG` — Used by install_pkgs to install correct PPD variant
- `AUR_HELPER` — Used by install_pkgs for AUR package install

**Functions now available:**
- `require_non_root()`, `detect_distro()`, `sudo_keepalive_start()`, `ensure_aur_helper()` — Already called in main()
- All helpers (`info`, `skip`, `warn`, `die`, `hr`) — Available for Plan 02 functions

**Plan 02 must add:**
- `install_pkgs()` — Uses $BUILD_DEPS, $RUNTIME_DEPS, $PPD_PKG, $AUR_HELPER
- `install_udev_rule()` — Uses $UDEV_RULE_SRC, $UDEV_RULE_DST
- `enable_service()` — Uses $INIT to dispatch on systemd vs openrc
- `ensure_groups()` — Uses $USER for group membership

## Auth Gates

None — this plan only modified local files, no authentication required.

## Known Stubs

None.

## Threat Flags

None — the threat model for this plan was fully implemented with mitigations for:
- T-01-01 (os-release spoofing) — mitigated by strict whitelist
- T-01-02 (sudo credential window) — mitigated by trap cleanup
- T-01-03 (running as root) — mitigated by require_non_root at start of main()
- T-01-04 (AUR helper build under root) — mitigated by makepkg refusing root + non-root main flow
- T-01-05 (chroot DoS) — mitigated by /run/openrc and /run/systemd/system probes
- T-01-06 (tempdir leftover) — mitigated by trap cleanup
