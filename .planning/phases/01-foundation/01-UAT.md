---
status: testing
phase: 01-foundation
source: [01-01-SUMMARY.md, 01-02-SUMMARY.md]
started: 2026-04-09T17:30:00Z
updated: 2026-04-09T17:30:00Z
---

## Current Test

number: 1
name: Build System Compiles
expected: |
  Run `make -C utils clean all` from the repo root.
  Expected: compiles common/util.o and common/dmenu.o without errors or warnings.
awaiting: user response

## Tests

### 1. Build System Compiles
expected: Run `make -C utils clean all` — compiles util.o and dmenu.o without errors or warnings.
result: [pending]

### 2. test-util Passes
expected: Run `make -C utils test-util && utils/test-util`. Expected: all 10 tests pass (die, warn, pscanf, exec_wait, exec_detach validation).
result: [pending]

### 3. test-dmenu Compiles and Links
expected: Run `make -C utils test-dmenu && utils/test-dmenu`. Expected: 11 tests pass (compile-time, link-time, runtime checks including dmenu pipe open/close).
result: [pending]

### 4. install.sh Builds Utilities
expected: Run `./install.sh`. Expected: the "Installing C utilities" section runs `make -C utils` successfully, then copies binaries to ~/.local/bin. No errors in the utils section.
result: [pending]

### 5. dmenu Cancel Returns NULL
expected: Run `utils/test-dmenu` — the runtime test that opens a dmenu pipe and reads from it returns NULL (since no user interaction possible in test). Validates the cancel/empty selection path.
result: [pending]

## Summary

total: 5
passed: 0
issues: 0
pending: 5
skipped: 0
blocked: 0

## Gaps

[none yet]
