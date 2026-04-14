# Suckless Environment v2

## What This Is

A hardened, C-native utility suite for a dwm-based desktop environment targeting Arch Linux (systemd) and Artix Linux (OpenRC). Replaces fragile shell scripts with compiled C programs that integrate with dmenu, dunst, and X11 — and extends coverage with missing features (working battery alerts, brightness control). Built to be shareable as a public dotfiles project, not just personal config.

## Core Value

**One install.sh works on both Arch and Artix, and every user-facing utility is a fast, safe C program that works out of the box.** If distro detection fails, or the C utilities crash, the project has failed its purpose.

## Requirements

### Validated

<!-- Inferred from existing code in the current tree. These already ship. -->

- ✓ **dwm 6.8** tiling window manager with custom keybindings, Tokyo Night colors, nerd-font status — `dwm/`
- ✓ **st** terminal emulator — `st/`
- ✓ **dmenu** launcher/menu — `dmenu/`
- ✓ **slstatus** status bar with CPU/RAM/battery/network/datetime — `slstatus/`
- ✓ **dunst** notification daemon — `dunst/dunstrc`
- ✓ C util: `dmenu-clip` — clipboard history browser — `utils/dmenu-clip/`
- ✓ C util: `dmenu-clipd` — clipboard caching daemon — `utils/dmenu-clipd/`
- ✓ C util: `dmenu-cpupower` — CPU power profile selector (via `powerprofilesctl`) — `utils/dmenu-cpupower/`
- ✓ C util: `dmenu-session` — lock/logout/reboot/shutdown menu (via `loginctl` + `betterlockscreen`) — `utils/dmenu-session/`
- ✓ C util: `screenshot-notify` — maim-based screenshot with dunst notification — `utils/screenshot-notify/`
- ✓ C util: `battery-notify` scaffolding (exists but reported not working) — `utils/battery-notify/`
- ✓ Multi-monitor support via libxinerama
- ✓ PulseAudio volume control via libpulse
- ✓ Session/power actions via `loginctl` (elogind on Artix, systemd-logind on Arch)
- ✓ betterlockscreen integration for screen locking

### Active

<!-- v2 milestone scope. These are the three pillars the user named, decomposed into testable requirements. -->

**Pillar 1 — Dual-distro install (Arch + Artix)**

- [ ] **INST-01** — `install.sh` detects Arch vs Artix at runtime via `/etc/os-release`
- [ ] **INST-02** — Install selects the correct `power-profiles-daemon` package variant per distro (plain on Arch, `-openrc` on Artix)
- [ ] **INST-03** — Install enables `power-profiles-daemon` via the correct init (`systemctl enable --now` on Arch, `rc-update add && rc-service start` on Artix)
- [ ] **INST-04** — Install fails loudly with a clear, actionable error on unsupported distros (e.g. Ubuntu, Void) rather than silently breaking
- [ ] **INST-05** — Install is idempotent: re-running on an already-configured system does not duplicate services or fail
- [ ] **INST-06** — `install.sh` only installs C utilities from `utils/`; it does not copy `scripts/` to PATH

**Pillar 2 — Complete the shell → C migration**

- [ ] **MIG-01** — Parity audit confirms each C util in `utils/` matches or exceeds the behavior of its `scripts/` counterpart (flags, edge cases, error messages)
- [ ] **MIG-02** — `scripts/` is marked deprecated in `scripts/README.md` with removal planned for the next milestone
- [ ] **MIG-03** — Documentation drift fixed: `CLAUDE.md` and in-repo docs reference C utils, not shell scripts, and not removed deps like `slock`/`pkexec cpupower`
- [ ] **MIG-04** — Repo hygiene: remove `dwm/p1.rej` patch-reject artifact, fix `.gitignore` typo `.plannig` → `.planning`

**Pillar 3 — Missing features**

- [ ] **BATT-01** — Diagnose and fix the broken `battery-notify` so it reliably fires low/critical notifications on a running system
- [ ] **BATT-02** — Urgency-tiered notifications: low threshold → normal urgency, critical threshold → critical urgency (persistent in dunst until dismissed)
- [ ] **BATT-03** — Thresholds configurable via `config.def.h` following the suckless config pattern
- [ ] **BRIGHT-01** — New C util `brightness-notify` (or similar) adjusts backlight via `brightnessctl` and emits a dunst notification with the new level
- [ ] **BRIGHT-02** — Wired into dwm keybindings for `XF86MonBrightnessUp` and `XF86MonBrightnessDown`
- [ ] **BRIGHT-03** — `brightnessctl` added to `install.sh` runtime deps on both Arch and Artix

**Pillar 4 — Shareability (public release readiness)**

- [ ] **REL-01** — README overhaul: project purpose, supported distros, install instructions, keybinding reference, troubleshooting
- [ ] **REL-02** — Screenshots in README showing dwm bar, dmenu, dunst notifications on a real desktop
- [ ] **REL-03** — Install instructions verified from a clean Arch VM and a clean Artix VM (or documented as "manually tested on the author's machines" if VMs are out of scope)

### Out of Scope

<!-- Explicit boundaries with reasoning. -->

- **AUR PKGBUILD** — The user chose README overhaul over packaging. Users clone the repo and run `install.sh`; no `yay -S suckless-environment` path this milestone. Revisit post-v2.
- **CI build check** — Not selected. Build correctness verified by running `install.sh` locally before release.
- **Version tags / CHANGELOG** — Not selected. Can be adopted later without code changes if the project gains users.
- **Wayland support** — Suckless toolchain (dwm, st, dmenu, slock) is X11-native. Porting would be a rewrite, not a milestone.
- **Void Linux support** — Earlier CLAUDE.md mentioned Void; confirmed as legacy copy. Void uses runit, adding a third init path is out of scope.
- **Ubuntu/Fedora/Debian support** — These distros don't have the suckless packages or expect a different install flow; INST-04 fails loudly on them rather than faking support.
- **Deleting `scripts/` this milestone** — User chose deprecation, not deletion; removal deferred to next milestone after C parity has burned in.
- **Deduplicating `drw.c`/`drw.h` between `dwm/` and `dmenu/`** — They are byte-identical but upstream suckless keeps them separate; matching upstream reduces patch friction.
- **systemd unit files for utilities** — `loginctl` already works on both inits; no need for systemd-specific services.
- **Custom X11 overlay bar for brightness OSD** — User chose dunst OSD; custom Xlib overlay is heavier and less consistent with the rest of the environment.
- **On-low-battery auto-suspend** — Not selected in the battery scope questions; users can configure this at the systemd/OpenRC layer if they want it.
- **Battery triggering brightness dimming** — User chose "two separate features" over integration; battery and brightness remain independent.

## Context

**Brownfield state** — The tree already contains working dwm, st, dmenu, slstatus, dunst setups plus C versions of all four former shell scripts (in `utils/`) and C scaffolding for battery/screenshot notifications. The codebase is mapped in `.planning/codebase/` — see `CONCERNS.md` for the full drift/debt list.

**Key drift to fix** (from CONCERNS.md and the scan findings):
- `CLAUDE.md` lists `slock` as a dep but `utils/dmenu-session/dmenu-session.c:33-36` uses `betterlockscreen`; `scripts/dmenu-session:13` still references `slock`.
- `CLAUDE.md` says `pkexec cpupower` for CPU profiles; real code at `utils/dmenu-cpupower/dmenu-cpupower.c:60-61` uses `powerprofilesctl`.
- `install.sh:13` hardcodes `power-profiles-daemon-openrc` — works on Artix, breaks on Arch.
- `.gitignore:17` has typo `.plannig` → `.planning/` is not actually ignored.
- `dwm/p1.rej` patch-reject file checked into tree.
- `battery-notify` exists but is reportedly non-functional today.

**Existing convention baseline** (from `CONVENTIONS.md`): K&R C99 with tabs, `config.def.h → config.h` compile-time config, `die()`/`warn()` error handling, platform guards via `#if defined(__linux__)`. New utils must match this style.

**No test suite exists** — validation is manual (install, run, eyeball). Per `TESTING.md`, this is consistent with suckless practice; we rely on compiler warnings and `install.sh` as the integration check. Do not introduce a heavy test framework this milestone.

## Constraints

- **Platform**: Arch Linux (systemd + elogind-less) and Artix Linux (OpenRC + elogind) only — no Wayland, no other distros, no BSD
- **Init-agnostic runtime**: user-facing session/power actions go through `loginctl` (works on elogind + systemd-logind); only `install.sh` knows about systemctl vs rc-service
- **Language**: C99 for utilities, POSIX sh for install scripts — no bash-isms, no Python, no frameworks
- **Display server**: X11 — direct Xlib/XFixes usage where needed
- **Style**: Suckless — minimal dependencies, compile-time `config.h` customization, no runtime config files
- **Dependencies (runtime)**: dwm/st/dmenu/slstatus + betterlockscreen (AUR), power-profiles-daemon, lxpolkit, maim, xclip, dunst, brightnessctl (new)
- **Dependencies (build)**: `base-devel`, libxft, libxinerama, freetype2, fontconfig, xorg headers
- **Install target**: binaries to `/usr/local/bin/`, man pages to `/usr/local/share/man/`
- **No privilege escalation in utilities**: `dmenu-cpupower` uses `powerprofilesctl` (unprivileged), brightness uses `brightnessctl` with its setuid helper — no `pkexec`, no `sudo` in user-facing tools

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Single `install.sh` with `/etc/os-release` detection (not separate per-distro scripts) | One source of truth, less duplication, fewer paths to rot | — Pending |
| Two separate features for battery + brightness (not one integrated) | Independent utilities follow suckless single-purpose convention; either can fail without taking down the other | — Pending |
| Keep `scripts/` as deprecated fallback this milestone, delete next | C utils are new; keep escape hatch until users confirm parity | — Pending |
| `brightnessctl` backend over `xbacklight` or raw `/sys/class/backlight` | `xbacklight` is flaky on modern laptops; raw sysfs needs udev rules or setuid; `brightnessctl` already handles both via its own helper | — Pending |
| Dunst OSD for brightness (not custom Xlib overlay) | Matches how battery-notify surfaces info; no new X11 code to maintain; consistent UX across utilities | — Pending |
| README overhaul over PKGBUILD/CI/CHANGELOG for "public-ready" | User will onboard others via README first; packaging is deferrable; versioning costs more than it buys at this stage | — Pending |
| Fail loudly in `install.sh` on unsupported distros rather than best-effort | Silent mis-installs are worse than a clear error; matches suckless "refuse to guess" philosophy | — Pending |
| No test framework — install.sh is the integration check | Matches suckless practice (TESTING.md); adding Cmocka/Check for this scope is premature | — Pending |

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
*Last updated: 2026-04-14 after initialization*
