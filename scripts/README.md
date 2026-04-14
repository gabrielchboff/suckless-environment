# Shell Scripts (DEPRECATED)

These shell scripts are deprecated and will be removed in the next milestone.
The C utilities in `utils/` are the supported implementation.

## Deprecation Notice

The shell scripts in this directory are provided for **reference only**.
They are NOT installed by `install.sh`.

**C utilities are installed to `~/.local/bin`** and are the supported implementation.

## Mapping Table

| Shell Script | C Utility | Status |
|--------------|----------|--------|
| scripts/dmenu-session | utils/dmenu-session/dmenu-session.c | ACTIVE |
| scripts/dmenu-cpupower | utils/dmenu-cpupower/dmenu-cpupower.c | ACTIVE |
| scripts/dmenu-clip | utils/dmenu-clip/dmenu-clip.c | ACTIVE |
| scripts/dmenu-clipd | utils/dmenu-clipd/dmenu-clipd.c | ACTIVE |

## Notes

- **Lock**: `dmenu-session` uses `betterlockscreen` (not `slock`)
- **CPU**: `dmenu-cpupower` uses `powerprofilesctl` (not `pkexec cpupower`)
- **Clipboard**: Both use the same cache directory (`${XDG_CACHE_HOME}/dmenu-clipboard`)
- **Daemon**: `dmenu-clipd` runs as X11 daemon using XFixes (C version) vs polling (shell version)

## Why C?

The C utilities provide:
- Faster execution
- No shell interpreter dependency
- Proper X11 event handling (dmenu-clipd)
- Single-instance enforcement (flock)
- Better UTF-8 clipboard fallback handling
- Unprivileged CPU governor control (powerprofilesctl)