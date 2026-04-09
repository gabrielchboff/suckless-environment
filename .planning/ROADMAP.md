# Roadmap: Suckless Environment v2

## Overview

This roadmap converts six shell scripts into native C utilities for a dwm-based Arch Linux desktop. The work flows from a shared foundation library, through simple one-shot utilities that validate patterns, to interactive dmenu utilities, and finally the most complex piece: an event-driven clipboard daemon. Each phase delivers complete, testable functionality and validates patterns needed by subsequent phases.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [ ] **Phase 1: Foundation** - Shared C library, Makefile convention, and install pipeline
- [ ] **Phase 2: One-Shot Utilities** - Battery monitor and screenshot notifier (simplest binaries, validate build and exec patterns)
- [ ] **Phase 3: Interactive Utilities** - Session manager, power profiles, and clipboard picker via dmenu
- [ ] **Phase 4: Clipboard Daemon** - Event-driven X11 clipboard monitor with full selection protocol

## Phase Details

### Phase 1: Foundation
**Goal**: Every subsequent utility can be built against a shared library with a consistent Makefile convention, and install.sh compiles and installs everything
**Depends on**: Nothing (first phase)
**Requirements**: FOUND-01, FOUND-02, FOUND-03, FOUND-04
**Success Criteria** (what must be TRUE):
  1. A test program linking common/util.c can call die(), warn(), pscanf(), exec_wait(), and exec_detach() and compile without errors
  2. A test program linking common/dmenu.c can open a dmenu pipe, write items, and read a selection back using dmenu_open() and dmenu_select()
  3. Running `make` in any utility directory produces a binary using the shared config.mk convention
  4. Running install.sh compiles all utilities and places binaries into ~/.local/bin
**Plans**: 2 plans

Plans:
- [x] 01-01-PLAN.md — Build system (config.mk + Makefile) and shared utility library (util.h/util.c) with test program
- [x] 01-02-PLAN.md — dmenu pipe library (dmenu.c), dmenu test program, and install.sh integration

### Phase 2: One-Shot Utilities
**Goal**: Users receive battery low alerts and screenshot-copied notifications from native C binaries, validating the foundation's exec and sysfs patterns
**Depends on**: Phase 1
**Requirements**: BATT-01, BATT-02, BATT-03, BATT-04, SCRN-01, SCRN-02, SCRN-03
**Success Criteria** (what must be TRUE):
  1. When battery is at or below 20% and discharging, running battery-notify produces a dunst notification
  2. Running battery-notify a second time while still below threshold does NOT produce a duplicate notification (state file prevents re-alert)
  3. When battery rises above threshold or starts charging, the state file is cleared so future drops trigger a new alert
  4. Selecting a screen area with screenshot-notify captures it to clipboard and shows a dunst "Image copied to clipboard" notification
  5. Canceling the area selection (pressing Escape) produces no notification and exits cleanly
**Plans**: 2 plans

Plans:
- [x] 02-01-PLAN.md — Battery-notify: sysfs reading, state file logic, critical dunst notification
- [x] 02-02-PLAN.md — Screenshot-notify: maim|xclip pipeline, cancel detection, success notification

### Phase 3: Interactive Utilities
**Goal**: Users can manage sessions, power profiles, and clipboard history through dmenu-driven C utilities that replace the existing shell scripts
**Depends on**: Phase 1
**Requirements**: SESS-01, SESS-02, SESS-03, SESS-04, SESS-05, POWER-01, POWER-02, POWER-03, POWER-04, CLIP-01, CLIP-02, CLIP-03, CLIP-04
**Success Criteria** (what must be TRUE):
  1. User can lock the screen via dmenu-session and betterlockscreen activates with blur effect (no duplicate instances spawned)
  2. User can logout, reboot, or shutdown via dmenu-session with a confirmation dialog preventing accidental execution
  3. User can see the current power profile in the dmenu prompt and switch between performance/balanced/power-saver
  4. User can browse clipboard history newest-first in dmenu, see truncated previews, and select an entry to restore it to the clipboard
  5. All three utilities accept extra arguments that are passed through to dmenu (e.g., custom font, color overrides)
**Plans**: TBD
**UI hint**: yes

Plans:
- [ ] 03-01: TBD
- [ ] 03-02: TBD
- [ ] 03-03: TBD

### Phase 4: Clipboard Daemon
**Goal**: Clipboard history is captured automatically via an event-driven daemon that replaces the polling shell script, completing the full clipboard workflow
**Depends on**: Phase 1, Phase 3 (dmenu-clip reads the cache dmenu-clipd writes)
**Requirements**: CLIPD-01, CLIPD-02, CLIPD-03, CLIPD-04, CLIPD-05
**Success Criteria** (what must be TRUE):
  1. After starting dmenu-clipd, copying text in any application automatically creates a cache entry without any polling (XFixes event-driven)
  2. Duplicate clipboard content is not stored (FNV-1a hash dedup), and cache never exceeds 50 entries (LRU pruning removes oldest)
  3. Only one instance of dmenu-clipd can run at a time (flock enforcement); attempting a second instance exits immediately
  4. Sending SIGTERM to dmenu-clipd shuts it down cleanly (no orphaned X connections, no corrupted cache)
  5. End-to-end: copy text in an app, open dmenu-clip, see the entry, select it, paste it in another app -- the full clipboard workflow works
**Plans**: TBD

Plans:
- [ ] 04-01: TBD
- [ ] 04-02: TBD

## Progress

**Execution Order:**
Phases execute in numeric order: 1 -> 2 -> 3 -> 4

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Foundation | 0/2 | Planning complete | - |
| 2. One-Shot Utilities | 0/2 | Planning complete | - |
| 3. Interactive Utilities | 0/0 | Not started | - |
| 4. Clipboard Daemon | 0/0 | Not started | - |
