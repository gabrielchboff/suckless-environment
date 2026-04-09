---
status: complete
phase: 02-one-shot-utilities
source: [02-01-SUMMARY.md, 02-02-SUMMARY.md]
started: 2026-04-09T18:00:00Z
updated: 2026-04-09T18:05:00Z
---

## Current Test

[testing complete]

## Tests

### 1. Battery Low Notification
expected: Run `battery-notify` when battery ≤20% discharging — dunst critical notification appears. If above 20% or charging, exits silently.
result: skipped

### 2. Duplicate Prevention
expected: Run `battery-notify` twice in a row (while still below threshold). Second run should NOT produce a notification. Check `/tmp/battery-notified` exists.
result: pass

### 3. State File Clears on Recovery
expected: Charge battery above 20% or plug in charger, then run `battery-notify`. The state file `/tmp/battery-notified` should be removed.
result: pass

### 4. Screenshot Capture + Notification
expected: Run `screenshot-notify`, select an area on screen. Expected: dunst shows "Image copied to clipboard". Paste in an app — the captured image appears.
result: pass

### 5. Screenshot Cancel
expected: Run `screenshot-notify`, press Escape to cancel area selection. Expected: no notification, clean exit.
result: pass

## Summary

total: 5
passed: 4
issues: 0
pending: 0
skipped: 1
blocked: 0

## Gaps

[none]
