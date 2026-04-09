# Technology Stack

**Project:** Suckless Environment v2 -- Native C Utilities
**Researched:** 2026-04-09

## Design Principle

Every library choice follows the suckless philosophy: use the smallest API that solves the problem, link only what is necessary, and match the existing dwm/dmenu/slstatus Makefile conventions (explicit `-l` flags, `pkg-config` where needed, C99, `-Os`).

## Recommended Stack

### X11 / Clipboard (dmenu-clipd, dmenu-clip)

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| libX11 (`-lX11`) | 1.8.13 | Core X11 connection, atoms, event loop | Already used by dwm/dmenu/slstatus. Required for `XInternAtom`, `XNextEvent`, `XGetSelectionOwner` | HIGH -- verified on system |
| libXfixes (`-lXfixes`) | 6.0.2 | Clipboard change notification via `XFixesSelectSelectionInput` | Eliminates 0.5s poll loop. The correct X11 mechanism for clipboard monitoring. Used by clipnotify, SDL, and every serious clipboard manager. One function call registers interest; events arrive through the normal `XNextEvent` loop | HIGH -- verified on system, header at `/usr/include/X11/extensions/Xfixes.h` |

**Pattern:** The clipboard daemon registers `XFixesSelectionNotifyMask` on `XA_CLIPBOARD` and `XA_PRIMARY`, then blocks on `XNextEvent`. When a `XFixesSelectionNotify` arrives, it reads the clipboard content via `XConvertSelection` / `SelectionNotify` handshake, hashes it, and stores it. Reference implementation: [clipnotify](https://github.com/cdown/clipnotify) (~80 lines of C).

**Headers needed:**
```c
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xfixes.h>
```

**Link flags:** `-lX11 -lXfixes`

### D-Bus / Power Profiles (dmenu-cpupower)

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| libsystemd / sd-bus (`-lsystemd`) | 260 | D-Bus client for power-profiles-daemon | Already present on every systemd Arch system. No extra dependency. Pure C API. Cleaner and smaller than libdbus-1. Provides `sd_bus_get_property_string` and `sd_bus_set_property` which are exactly what we need | HIGH -- verified on system, header at `/usr/include/systemd/sd-bus.h` |

**NOT libdbus:** libdbus-1 is the older, lower-level D-Bus binding. sd-bus is systemd's modern replacement -- cleaner API, better error handling, smaller code. Since Arch already runs systemd and `systemd-libs` is installed, sd-bus adds zero new dependencies.

**D-Bus Interface (verified via `busctl introspect`):**
- **Bus:** system bus
- **Service:** `org.freedesktop.UPower.PowerProfiles` (canonical) or `net.hadess.PowerProfiles` (legacy alias, both work on this system)
- **Object path:** `/org/freedesktop/UPower/PowerProfiles`
- **Interface:** `org.freedesktop.UPower.PowerProfiles`
- **`ActiveProfile` property:** read/write, type `s`, values: `"performance"`, `"balanced"`, `"power-saver"`

**C pattern for getting the profile:**
```c
sd_bus *bus = NULL;
char *profile = NULL;
sd_bus_open_system(&bus);
sd_bus_get_property_string(bus,
    "org.freedesktop.UPower.PowerProfiles",
    "/org/freedesktop/UPower/PowerProfiles",
    "org.freedesktop.UPower.PowerProfiles",
    "ActiveProfile",
    NULL, &profile);
// profile is now "performance", "balanced", or "power-saver"
free(profile);
sd_bus_unref(bus);
```

**C pattern for setting the profile:**
```c
sd_bus_set_property(bus,
    "org.freedesktop.UPower.PowerProfiles",
    "/org/freedesktop/UPower/PowerProfiles",
    "org.freedesktop.UPower.PowerProfiles",
    "ActiveProfile",
    NULL, "s", "power-saver");
```

**Link flags:** `-lsystemd` (via `pkg-config --libs libsystemd`)

### Notifications (battery-notify, screenshot-notify)

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| sd-bus direct D-Bus (`-lsystemd`) | 260 | Send notifications to dunst via `org.freedesktop.Notifications.Notify` | Avoids libnotify and its glib2/gdk-pixbuf/gio dependency chain. We already link libsystemd for power profiles. One D-Bus method call replaces the entire libnotify API for our use case | HIGH -- D-Bus interface verified via `busctl` |

**Why NOT libnotify:** libnotify pulls in glib2, gdk-pixbuf2, gio-2.0, and ~15 transitive shared libraries. Its `pkg-config --cflags` alone adds 9 include paths. This violates suckless philosophy. For our utilities that just need to fire a simple text notification, the D-Bus call is ~20 lines of C vs. libnotify's 5 lines, but with zero additional dependencies (sd-bus is already needed for power profiles).

**D-Bus Notify interface (verified on this system):**
- **Bus:** session bus
- **Service:** `org.freedesktop.Notifications` (dunst)
- **Object path:** `/org/freedesktop/Notifications`
- **Interface:** `org.freedesktop.Notifications`
- **Method:** `Notify` -- signature `susssasa{sv}i` returns `u`
  - `s` app_name
  - `u` replaces_id (0 = new)
  - `s` app_icon
  - `s` summary
  - `s` body
  - `as` actions (empty array for no actions)
  - `a{sv}` hints (empty dict for defaults)
  - `i` expire_timeout (-1 = server default)

**C pattern:**
```c
sd_bus *bus = NULL;
sd_bus_open_user(&bus);  // session bus for notifications
sd_bus_call_method(bus,
    "org.freedesktop.Notifications",
    "/org/freedesktop/Notifications",
    "org.freedesktop.Notifications",
    "Notify",
    NULL, NULL,
    "susssasa{sv}i",
    "battery-notify",  // app_name
    (uint32_t)0,       // replaces_id
    "",                 // icon
    "Low Battery",     // summary
    "Battery at 20%",  // body
    0,                 // empty actions array
    0,                 // empty hints dict
    (int32_t)-1);      // default timeout
sd_bus_unref(bus);
```

**Link flags:** `-lsystemd` (already needed)

### Battery Monitoring (battery-notify)

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| Linux sysfs (`/sys/class/power_supply/`) | kernel ABI | Read battery capacity and status | Standard kernel interface. No library needed -- just `open()`/`read()`/`close()` on pseudo-files. Stable ABI guaranteed by the kernel | HIGH -- verified: `/sys/class/power_supply/BAT1/capacity` returns percentage, `/sys/class/power_supply/BAT1/status` returns charging state |

**Important system detail:** This machine has `BAT1` (not `BAT0`). The utility should scan `/sys/class/power_supply/` for entries with `type == "Battery"` rather than hardcoding `BAT0`.

**Files to read:**
- `capacity` -- integer 0-100, current charge percentage
- `status` -- string: "Charging", "Discharging", "Full", "Not charging"

### Content Hashing (dmenu-clipd)

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| OpenSSL EVP API (`-lcrypto`) | 3.6.1 | MD5 hash for clipboard content dedup | Already installed. The legacy `MD5()` function works but is deprecated in OpenSSL 3.x; use `EVP_MD_CTX` / `EVP_DigestInit_ex` / `EVP_DigestUpdate` / `EVP_DigestFinal_ex` with `EVP_md5()` for forward compatibility | MEDIUM -- works but consider alternative below |

**Lighter alternative: FNV-1a hash inline.** For clipboard dedup, cryptographic strength is irrelevant. A 64-bit FNV-1a hash is ~10 lines of inline C, zero dependencies, and faster than MD5. Output as hex string for filenames. Recommended over OpenSSL for this use case.

```c
static uint64_t fnv1a(const char *data, size_t len) {
    uint64_t hash = 0xcbf29ce484222325ULL;
    for (size_t i = 0; i < len; i++) {
        hash ^= (uint8_t)data[i];
        hash *= 0x100000001b3ULL;
    }
    return hash;
}
```

**Recommendation: Use FNV-1a, skip OpenSSL entirely.** One fewer `-l` flag. The existing shell scripts use `md5sum` but there is no compatibility requirement -- this is a rewrite.

### Process Execution (dmenu-session, screenshot-notify)

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| POSIX `fork()`/`execvp()`/`waitpid()` | POSIX.1-2008 | Launch dmenu, betterlockscreen, maim, xclip, systemctl | Standard POSIX. No library. The suckless way -- dwm itself uses `fork()/execvp()` for spawning | HIGH |
| `popen()` | POSIX.1-2008 | Read dmenu selection output | Simpler than pipe/fork/exec for capturing stdout from dmenu | HIGH |

**For dmenu interaction pattern:**
```c
FILE *fp = popen("dmenu -p 'session' -l 4", "r");
// write choices to dmenu via a pipe, read selection
```

**For fire-and-forget commands (betterlockscreen, systemctl):**
```c
if (fork() == 0) {
    execvp(argv[0], argv);
    _exit(1);
}
```

### Build System

| Technology | Purpose | Why | Confidence |
|------------|---------|-----|------------|
| Per-utility `Makefile` | Build each utility independently | Matches dwm/dmenu/slstatus pattern. Each utility is self-contained: own `config.mk`, own `Makefile`, own install target | HIGH |
| `config.mk` include pattern | Centralize compiler/linker flags per utility | Suckless convention. Allows per-utility customization of CFLAGS/LDFLAGS | HIGH |
| C99 (`-std=c99`) | Language standard | Matches existing suckless builds. Sufficient for all APIs used | HIGH |
| `-Os -pedantic -Wall` | Compiler flags | Matches existing suckless builds | HIGH |

**Makefile pattern (matching existing `dwm/config.mk`):**
```makefile
VERSION = 1.0

PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

# Per-utility: only the libs this tool needs
LIBS = -lX11 -lXfixes    # for dmenu-clipd
# LIBS = -lsystemd        # for dmenu-cpupower
# LIBS = -lsystemd        # for battery-notify (notifications)
# LIBS = (none)           # for dmenu-session (pure POSIX)

CFLAGS = -std=c99 -pedantic -Wall -Os -D_DEFAULT_SOURCE -DVERSION=\"$(VERSION)\"
LDFLAGS = $(LIBS)
CC = cc
```

## Dependency Summary by Utility

| Utility | Libraries | System Packages | New Dependencies |
|---------|-----------|----------------|-----------------|
| dmenu-clipd | `-lX11 -lXfixes` | libx11, libxfixes | **None** (both already installed for dwm) |
| dmenu-clip | `-lX11` | libx11 | **None** |
| dmenu-cpupower | `-lsystemd` | systemd-libs | **None** (already installed) |
| dmenu-session | (none) | (none) | **None** (pure POSIX + exec betterlockscreen) |
| battery-notify | `-lsystemd` | systemd-libs | **None** (sysfs + sd-bus notifications) |
| screenshot-notify | `-lsystemd` | systemd-libs | **None** (exec maim/xclip + sd-bus notifications) |

**Total new system packages needed: zero.** Every library is already present on this Arch system as a dependency of dwm, systemd, or the existing tool chain.

## Alternatives Considered

| Category | Recommended | Alternative | Why Not |
|----------|-------------|-------------|---------|
| Clipboard events | libXfixes `XFixesSelectSelectionInput` | Polling xclip every 0.5s | Current approach. Wastes CPU, misses fast changes, shell-dependent |
| Clipboard events | libXfixes | `xcb-xfixes` (XCB) | XCB is more "correct" but dwm/dmenu use Xlib. Mixing would be inconsistent. Xlib XFixes works fine |
| D-Bus | sd-bus (libsystemd) | libdbus-1 | Lower-level API, more boilerplate, worse error handling. sd-bus is the modern systemd replacement |
| D-Bus | sd-bus (libsystemd) | Exec `powerprofilesctl` directly | Adds process spawn overhead, harder error handling, but viable as fallback |
| D-Bus | sd-bus (libsystemd) | gdbus (GLib D-Bus) | Pulls in entire GLib. Absolute non-starter for suckless |
| Notifications | sd-bus direct D-Bus call | libnotify | Pulls in glib2 + gdk-pixbuf2 + gio + 15 transitive libs. Violates suckless philosophy |
| Notifications | sd-bus direct D-Bus call | Exec `notify-send` or `dunstify` | Process spawn for something we can do in-process with sd-bus we already link. Acceptable fallback |
| Notifications | sd-bus direct D-Bus call | dunstctl (dunst-specific) | Non-standard, ties to dunst internally. D-Bus Notifications spec is the correct abstraction |
| Hashing | FNV-1a inline | OpenSSL MD5 | Crypto overkill for dedup. Adds `-lcrypto` dependency. FNV-1a is faster and dependency-free |
| Hashing | FNV-1a inline | CRC32 | FNV-1a has better distribution for string data and is equally simple to implement |
| Build system | Individual Makefiles | CMake / Meson | Alien to suckless ecosystem. Would be the only non-Makefile build in the project |
| Process mgmt | `fork()/execvp()` | `system()` | `system()` invokes `/bin/sh`. We are specifically eliminating shell dependencies. `fork()/exec()` is the suckless way |

## Runtime Dependencies (External Commands)

These are exec'd at runtime, not linked:

| Command | Package | Used By | Purpose |
|---------|---------|---------|---------|
| `dmenu` | (built from source) | dmenu-clip, dmenu-cpupower, dmenu-session | Selection UI |
| `betterlockscreen` | betterlockscreen (AUR) | dmenu-session | Lock screen |
| `maim` | maim | screenshot-notify | Area screenshot capture |
| `xclip` | xclip | screenshot-notify | Copy screenshot to clipboard |
| `systemctl` | systemd | dmenu-session | Reboot/shutdown |

## Installation

No new packages to install. All build dependencies are already satisfied:

```bash
# Verify (all should already be installed)
pacman -Q libx11 libxfixes systemd-libs

# Build dependencies (for development headers -- already present if dwm builds)
# libx11 and libxfixes headers come with the base packages on Arch
# systemd-libs headers come with systemd
```

## Compile Commands (per utility)

```bash
# dmenu-clipd (clipboard daemon)
cc -std=c99 -pedantic -Wall -Os -o dmenu-clipd dmenu-clipd.c -lX11 -lXfixes

# dmenu-clip (clipboard selector)
cc -std=c99 -pedantic -Wall -Os -o dmenu-clip dmenu-clip.c -lX11

# dmenu-cpupower (power profile selector)
cc -std=c99 -pedantic -Wall -Os -o dmenu-cpupower dmenu-cpupower.c -lsystemd

# dmenu-session (session menu)
cc -std=c99 -pedantic -Wall -Os -o dmenu-session dmenu-session.c

# battery-notify (one-shot battery alert)
cc -std=c99 -pedantic -Wall -Os -o battery-notify battery-notify.c -lsystemd

# screenshot-notify (capture + notify)
cc -std=c99 -pedantic -Wall -Os -o screenshot-notify screenshot-notify.c -lsystemd
```

## Sources

- [libXfixes header (Xfixes.h)](https://github.com/D-Programming-Deimos/libX11/blob/master/c/X11/extensions/Xfixes.h) -- XFixes extension API
- [clipnotify source](https://github.com/cdown/clipnotify) -- Reference implementation of XFixes clipboard monitoring in C
- [sd-bus official docs](https://www.freedesktop.org/software/systemd/man/latest/sd-bus.html) -- systemd D-Bus client API
- [sd_bus_set_property man page](https://man.archlinux.org/man/sd_bus_set_property.3.en) -- Property get/set functions
- [power-profiles-daemon D-Bus API](https://freedesktop-team.pages.debian.net/power-profiles-daemon/gdbus-org.freedesktop.UPower.PowerProfiles.html) -- Interface reference
- [Arch Linux libnotify 0.8.8](https://archlinux.org/packages/extra/x86_64/libnotify/) -- Package details showing heavy deps
- [Desktop Notifications Specification](https://specifications.freedesktop.org/notification/notification-spec-latest.html) -- Freedesktop notification D-Bus protocol
- [Lennart Poettering: The new sd-bus API](https://0pointer.net/blog/the-new-sd-bus-api-of-systemd.html) -- sd-bus design rationale
- [Linux power supply sysfs](https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-class-power) -- Battery sysfs ABI documentation
- [dunstify vs notify-send](https://github.com/dunst-project/dunst/issues/1118) -- Feature comparison
