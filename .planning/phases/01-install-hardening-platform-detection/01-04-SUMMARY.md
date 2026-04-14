---
phase: 01-install-hardening-platform-detection
plan: 04
type: execute
subsystem: hygiene
tags:
  - hygiene
  - gitignore
  - cleanup
dependency_graph:
  requires: []
  provides:
    - Clean .gitignore
    - Removed stale patch reject file
  affects:
    - Phase 1 complete
tech_stack:
  added: []
  patterns:
    - Git rm for tracked file deletion
    - Line-level gitignore editing
key_files:
  created: []
  modified:
    - .gitignore (removed .plannig typo line)
    - dwm/p1.rej (removed from git index)
decisions:
  - "D-19: Remove p1.rej (all hunks verified applied to dwm.c)"
  - "D-20: Remove .plannig typo from .gitignore"
  - "D-21: .claude line left untouched (out of scope)"
metrics:
  duration: ~1 minute
  completed: 2026-04-14
  tasks: 2
---

# Phase 1 Plan 4: Repo Hygiene Summary

## One-Liner

Removed stale `dwm/p1.rej` patch reject file and fixed `.gitignore` typo (`.plannig` → removed, preserving `.planning/` and `.claude`).

## What Was Built

### Task 1: Remove dwm/p1.rej

**Pre-flight check (belt-and-braces):**
Verified all 6 hunks from p1.rej are applied to dwm/dwm.c:
1. `static int drawstatusbar(Monitor *m, int bh, char* text);` — FOUND
2. `static char stext[1024];` — FOUND
3. `for (i = 0; i < LENGTH(colors) + 1; i++)` — FOUND
4. `drawstatusbar(Monitor *m, int bh, char*` — FOUND
5. `drawstatusbar(m, bh, stext)` — FOUND
6. `ecalloc(LENGTH(colors) + 1, sizeof(Clr *))` — FOUND

**Action:**
```bash
git rm dwm/p1.rej
```

Result: File removed from both working tree and git index.

### Task 2: Fix .gitignore typo

**Before (23 lines):**
```
.plannig      ← typo line (line 17)
.claude       ← should stay (line 18)
...
.planning/    ← correct entry (line 23)
```

**After (22 lines):**
```
.claude       ← line 17 (no longer line 18)
...
.planning/    ← line 22 (no longer line 23)
```

**Diff:**
```diff
-.plannig
```

Exactly one line removed. `.planning/` and `.claude` preserved untouched.

## Verification Results

**Task 1 (p1.rej):**
- ✓ File removed from working tree: `! test -f dwm/p1.rej`
- ✓ Not tracked in git: `! git ls-files dwm/p1.rej`
- ✓ dwm/dwm.c anchors still present (no accidental revert)

**Task 2 (.gitignore):**
- ✓ No `.plannig` line: `! grep -qE '^\.plannig$' .gitignore`
- ✓ `.planning/` preserved: `grep -qE '^\.planning/$' .gitignore`
- ✓ `.claude` preserved: `grep -qE '^\.claude$' .gitignore`
- ✓ Line count: 22 (one fewer than original 23)
- ✓ Minimal diff: exactly one deletion

## Note on *.rej

Did NOT add `*.rej` to .gitignore — per RESEARCH §Repo Hygiene, this was "recommended but not in success criteria #7" and is out of scope for this plan.

## Deviations

None — plan executed exactly as written.

## MIG-04 Status

**MIG-04:** Close ROADMAP Phase 1 success criterion #7 — **CLOSED**

Both requirements met:
- ✓ `dwm/p1.rej` no longer exists in the working tree
- ✓ `.gitignore` correctly ignores `.planning/` with no typo

## Auth Gates

None — purely local git operations.

## Known Stubs

None.

## Threat Flags

None — threat model implemented:
- T-01-23 (accidental hunk deletion) — mitigated by pre-flight grep check
- T-01-24 (overreaching gitignore edit) — mitigated by exact line count and preserved lines
- T-01-25 (un-ignoring files) — accepted (`.plannig` matched nothing)
