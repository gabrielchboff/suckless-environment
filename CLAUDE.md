<!-- GSD:project-start source:PROJECT.md -->
## Project

**Suckless Environment v2**

A hardened, C-native utility suite for a dwm-based desktop environment targeting Arch Linux (systemd) and Artix Linux (OpenRC). Replaces fragile shell scripts with compiled C programs that integrate with dmenu, dunst, and X11 — and extends coverage with missing features (working battery alerts, brightness control). Built to be shareable as a public dotfiles project, not just personal config.

**Core Value:** **One install.sh works on both Arch and Artix, and every user-facing utility is a fast, safe C program that works out of the box.** If distro detection fails, or the C utilities crash, the project has failed its purpose.

### Constraints

- **Platform**: Arch Linux (systemd + elogind-less) and Artix Linux (OpenRC + elogind) only — no Wayland, no other distros, no BSD
- **Init-agnostic runtime**: user-facing session/power actions go through `loginctl` (works on elogind + systemd-logind); only `install.sh` knows about systemctl vs rc-service
- **Language**: C99 for utilities, POSIX sh for install scripts — no bash-isms, no Python, no frameworks
- **Display server**: X11 — direct Xlib/XFixes usage where needed
- **Style**: Suckless — minimal dependencies, compile-time `config.h` customization, no runtime config files
- **Dependencies (runtime)**: dwm/st/dmenu/slstatus + betterlockscreen (AUR), power-profiles-daemon, lxpolkit, flameshot, xclip, dunst, brightnessctl (new)
- **Dependencies (build)**: `base-devel`, libxft, libxinerama, freetype2, fontconfig, xorg headers
- **Install target**: binaries to `/usr/local/bin/`, man pages to `/usr/local/share/man/`
- **No privilege escalation in utilities**: `dmenu-cpupower` uses `powerprofilesctl` (unprivileged), brightness uses `brightnessctl` with its setuid helper — no `pkexec`, no `sudo` in user-facing tools
<!-- GSD:project-end -->

<!-- GSD:stack-start source:codebase/STACK.md -->
## Technology Stack

## Languages
- C (C99) - All user-facing utilities and suckless tools. Every C source is compiled with `-std=c99 -pedantic -Wall`. See `utils/config.mk:8`, `dwm/config.mk:31`, `slstatus/config.mk:15`.
- POSIX sh - Thin legacy glue and dmenu launchers. See `scripts/` (now "reference only" per `install.sh:55-57`) and `dmenu/dmenu_run`, `dmenu/dmenu_path`, `dmenu/dmenu_run_desktop`, `dmenu/dmenu_path_desktop`.
- None. No Python, Lua, Rust, or other languages are present.
## Runtime
- X11 display server (Xlib). Required by dwm, st, dmenu, slstatus, and `dmenu-clipd`. Wayland is not supported.
- Linux (Arch or Artix). `sysfs`/`procfs` paths are hard-coded in several places (`/sys/class/power_supply`, `/sys/devices/system/cpu`, `/proc/stat`). See `slstatus/components/cpu.c:10` and `utils/battery-notify/battery-notify.c:10-11`.
- `elogind` (Artix / OpenRC) or `systemd-logind` (Arch) — power/session actions route through `loginctl`, never `systemctl` or OpenRC service calls. See `utils/dmenu-session/dmenu-session.c:77,89`.
- `pacman` (Arch/Artix) drives all distribution dependencies (`install.sh:16`).
- AUR helper (`yay` or `paru`) is auto-detected for AUR packages `brave-bin` and `betterlockscreen` (`install.sh:19-29`).
## Frameworks
- dwm 6.8 — window manager (`dwm/config.mk:2`, `dwm/dwm.c`).
- st 0.9.3 — terminal emulator (`st/config.mk:2`, `st/st.c`, `st/x.c`).
- dmenu 5.4 — menu (`dmenu/config.mk:2`, `dmenu/dmenu.c`).
- slstatus 1.1 — status monitor (`slstatus/config.mk:2`, `slstatus/slstatus.c`).
- dunst — notification daemon (installed by `install.sh:13`, config at `dunst/dunstrc`).
- fcitx5 — input method framework, launched in `.xprofile` (see `/home/void/.config/suckless-environment/.xprofile:14` and `dwm-start` replaces xprofile in practice; see `dwm-start:14`).
- lxpolkit — PolicyKit authentication agent (`dwm-start:13`, replaces the `polkit-gnome` mentioned in earlier revisions).
- power-profiles-daemon (Artix variant `power-profiles-daemon-openrc`) — replaces the older `cpupower` + `pkexec` pipeline. The C `dmenu-cpupower` shells out to `powerprofilesctl` (`utils/dmenu-cpupower/dmenu-cpupower.c:60-61`).
- betterlockscreen (AUR) — lock screen used by `action_lock()` in `utils/dmenu-session/dmenu-session.c:33-36`.
- POSIX `make` with the suckless `config.mk` + `config.h` idiom. Every tree follows the same pattern:
- `utils/Makefile` builds the in-house C utilities as a single project, linking each binary against `common/util.o` and (where relevant) `common/dmenu.o`. See `utils/Makefile:14-16`.
- Hand-rolled test harness (no framework). Tests live in `utils/tests/test-util.c` and `utils/tests/test-dmenu.c` and run via `make -C utils test` (`utils/Makefile:107-109`).
## Key Dependencies
- `base-devel` — gcc, make, binutils (`install.sh:10`).
- `libxft`, `libxinerama`, `freetype2`, `fontconfig` — font rendering and multi-monitor support (`install.sh:10`, used by every tool's `config.mk`).
- `xorg-server`, `xorg-xinit` — display server and session bootstrap (`install.sh:10`).
- `libX11` — linked by dwm, st, dmenu, slstatus, `dmenu-clipd` (`dwm/config.mk:26`, `slstatus/config.mk:19`, `utils/Makefile:11`).
- `libXft` + `libfontconfig` + `libfreetype2` — text rendering for dwm, st, dmenu (`dwm/config.mk:18`, `st/config.mk:20-21`, `dmenu/config.mk:16`).
- `libXinerama` — multi-monitor support for dwm and dmenu (`dwm/config.mk:14`, `dmenu/config.mk:12`).
- `libXfixes` — clipboard change notifications; linked only into `dmenu-clipd` (`utils/Makefile:11,64`).
- `libutil`, `libm`, `librt` — required by st for PTY/timers (`st/config.mk:19`).
- `flameshot` — screenshot capture (region/full/annotate, clipboard copy + save). Bound to `XK_Print` in `dwm/config.def.h:101` via `flameshot gui`.
- `xclip` — clipboard I/O. Invoked by `utils/dmenu-clip/dmenu-clip.c:75`.
- `xorg-xbacklight` — brightness keys via `xbacklight -inc/-dec 5` (`dwm/config.def.h:7-8`).
- `libpulse` / `pactl` — volume keys via `pactl set-sink-volume` (`dwm/config.def.h:9-11`).
- `pamixer` — volume percent for the status bar (`slstatus/config.def.h:76`).
- `xsel`, `xdotool` — listed in `install.sh:13`; not referenced directly in source, carried as convenience tools.
- `feh` — wallpaper in `dwm-start:24`.
- `thunar` — file manager, spawned via keybind `MODKEY+e` (`dwm/config.def.h:102`).
- `ttf-iosevka-nerd` — UI font used by dwm, dmenu, slstatus, dunst (`dwm/config.def.h:24-25`, `dunst/dunstrc:23`).
- `notify-send` (libnotify) — used by `battery-notify` and `brightness-notify` to emit dunst notifications (`utils/battery-notify/battery-notify.c:51-52`).
- `brave-bin` (AUR) — launched via `MODKEY+Shift+b` (`dwm/config.def.h:103`).
## Configuration
- Per-tool `config.h` (generated from `config.def.h`) is the only user-editable source. `dwm/config.def.h`, `st/config.def.h`, `dmenu/config.def.h`, `slstatus/config.def.h`, `utils/battery-notify/config.def.h`, `utils/dmenu-clip/config.def.h`, `utils/dmenu-clipd/config.def.h`.
- No runtime config files — every behavior is compiled in.
- Suckless tools install to `/usr/local/bin` (see `PREFIX` in each `config.mk`).
- In-house `utils/` install to `$HOME/.local/bin` (`utils/config.mk:5`). This is why `$HOME/.local/bin` is prepended to `PATH` in `dwm-start:3`.
- `~/.config/dunst/dunstrc` (from `dunst/dunstrc`, `install.sh:63-65`).
- `~/.xprofile` (from `.xprofile`, `install.sh:67-68`) — note this file is nearly empty; the real session bootstrap lives in `dwm-start`.
- `~/.local/bin/dwm-start` (from `dwm-start`, `install.sh:59-61`).
- `${XDG_CACHE_HOME:-$HOME/.cache}/dmenu-clipboard/` — clipboard history managed by `dmenu-clipd` (`utils/dmenu-clipd/dmenu-clipd.c:61,66`).
- `/tmp/battery-notified` — debounce marker for battery alerts (`utils/battery-notify/config.def.h:10`).
## Platform Requirements
- C99-compliant `cc` (defaults to gcc via `base-devel`). `CC = cc` is set in every `config.mk`.
- POSIX `make`; `slstatus/Makefile:3` and `st/Makefile:3` declare `.POSIX:`.
- `pkg-config` is used by st for fontconfig/freetype flags (`st/config.mk:13`).
- Development headers from `libxft`, `libxinerama`, `freetype2`, `fontconfig`. X11 includes expected at `/usr/X11R6/include` (every `config.mk`).
- Arch Linux (systemd) or Artix Linux (OpenRC + elogind). `install.sh:13` currently hard-codes `power-profiles-daemon-openrc` — on Arch this is a drift issue; the package is `power-profiles-daemon` without the `-openrc` suffix.
- X11 session started via `startx`/`xdm`; `dwm-start` is the intended `.xinitrc` target (prepends `$HOME/.local/bin` to `PATH`, runs `lxpolkit`, `fcitx5`, `dmenu-clipd`, `dunst`, the `slstatus` loop, `feh`, then `exec dwm`).
- Suckless tool binaries + man pages → `/usr/local/bin`, `/usr/local/share/man/man1` via `sudo make clean install` in each of `dwm/`, `st/`, `dmenu/`, `slstatus/`.
- In-house utility binaries → `~/.local/bin` via `make -C utils install`.
- Session scripts and configs as listed above.
<!-- GSD:stack-end -->

<!-- GSD:conventions-start source:CONVENTIONS.md -->
## Conventions

## Naming Patterns
- Suckless source files: lowercase, short, no prefix: `dwm/dwm.c`, `dwm/drw.c`, `dwm/util.c`, `st/st.c`, `st/x.c`, `dmenu/dmenu.c`, `slstatus/slstatus.c`.
- slstatus components: metric name only, no prefix or group: `slstatus/components/cpu.c`, `slstatus/components/ram.c`, `slstatus/components/battery.c`.
- In-house utilities: one directory per binary, main source matches binary name: `utils/battery-notify/battery-notify.c`, `utils/dmenu-session/dmenu-session.c`.
- Shared code under `utils/common/`: `util.c`/`util.h`, `dmenu.c`/`dmenu.h`.
- Config headers: `config.def.h` (committed defaults) and `config.h` (user copy, generated by the build). Verified at `dwm/Makefile:16-17`, `slstatus/Makefile:40-41`, and `utils/Makefile:31,59,66` (per-utility `config.h` deps).
- Test files in `utils/tests/` prefixed `test-`: `utils/tests/test-util.c`, `utils/tests/test-dmenu.c`.
- snake_case, lowercase, verb-first: `pscanf`, `exec_wait`, `exec_detach`, `setup_sigchld`, `sigchld_handler` (`utils/common/util.c:53,72,89,113,105`).
- Component functions in slstatus follow the pattern `<subject>_<aspect>`: `cpu_perc`, `cpu_freq`, `battery_perc`, `battery_state`, `netspeed_rx`, `netspeed_tx`, `ram_used`, `wifi_essid` (`slstatus/slstatus.h:4-85`).
- Static helpers inside a utility use short verbs: `verr`, `build_preview`, `restore_clipboard`, `get_cache_dir`, `acquire_lock`, `fnv1a`, `hash_to_filename`, `is_whitespace_only`, `setup_x11`, `wait_selection_notify`, `get_clipboard_text`, `run_event_loop` (`utils/common/util.c:15`, `utils/dmenu-clip/dmenu-clip.c:36,59,82`, `utils/dmenu-clipd/dmenu-clipd.c:46,74,95,108,114,127,159,173`).
- Action entry points in `dmenu-session` follow `action_<verb>`: `action_lock`, `action_logout`, `action_reboot`, `action_shutdown` (`utils/dmenu-session/dmenu-session.c:28,54,74,86`).
- Unused parameters in slstatus are literally named `unused` (not `(void)unused` casts): `const char *cpu_freq(const char *unused)` (`slstatus/components/cpu.c:13,25,55,75,113,129`).
- snake_case for local and file-static state: `cache_dir`, `lock_fd`, `done`, `dpy`, `win`, `xfixes_event_base` (`utils/dmenu-clipd/dmenu-clipd.c:30-39`).
- Short single-letter names are acceptable for loop counters, math, and pipe fds: `i`, `n`, `fd[2]`, `sum`, `a`, `b` (`slstatus/components/cpu.c:27-48`).
- Prefix `last_` or duplicate arrays `a`/`b` for previous-tick state in slstatus: `slstatus/components/cpu.c:27` (`static long double a[7]; long double b[7];`).
- `argv0` is a single file-scope `char *` exposed via `extern` in the shared util header for prefixing error output (`utils/common/util.h:9`, `utils/common/util.c:13`). Every `main()` sets it first: `argv0 = argv[0];` (`utils/battery-notify/battery-notify.c:41`, `utils/dmenu-session/dmenu-session.c:105`, `utils/dmenu-clipd/dmenu-clipd.c:372`).
- Structs declared with `struct` keyword inline, typedefs used sparingly.
- Enum lists are descriptive and grouped by purpose, one group per line: `enum { CurNormal, CurResize, CurMove, CurLast };` (`dwm/dwm.c:76-85`).
- ALL_UPPERCASE: `LEN`, `MAXLEN`, `MAX_ENTRIES`, `MAX_PREVIEW`, `CACHE_DIR_NAME`, `BATTERY`, `THRESHOLD`, `STATE_FILE`, `CAP_PATH`, `STAT_PATH`, `CONVERT_RETRIES`, `CONVERT_DELAY`, `FNV_OFFSET_BASIS`, `FNV_PRIME`, `TAGMASK`, `MODKEY`.
- Verified in `utils/common/util.h:7`, `slstatus/config.def.h:10`, `utils/dmenu-clip/config.def.h:4,7,10`, `utils/battery-notify/battery-notify.c:10-11`, `utils/dmenu-clipd/dmenu-clipd.c:23-28`.
## Code Style
- No linter. No `.editorconfig`. No `clang-format`. Style is maintained by convention and rejected at review time.
- Indentation is **hard tabs**. Confirmed in every `.c` file inspected (e.g., `utils/common/util.c`, `dwm/dwm.c`, `slstatus/slstatus.c`).
- Line length: generally fits 80 columns in utility code; upstream dwm/st allow long lines for macro definitions and format strings (e.g., `dwm/config.def.h:79-82` runs past 200 chars — kept as-is when readability demands).
- No trailing whitespace. `.gitignore:21` ignores editor backup files (`*.swp`, `*~`, `*.orig`).
- K&R for blocks: opening brace on same line as statement.
- **Function definitions use BSD/Allman style**: return type on its own line, function name at column 0, opening brace on its own line.
- Single-statement bodies may omit braces: `if (!fp) return;` (`utils/dmenu-clip/dmenu-clip.c:45-46`).
- Single space after control-flow keywords: `if (`, `while (`, `for (`, `switch (`, `return (`.
- No space between function name and parenthesis: `printf(...)`, `fopen(...)`, `dmenu_open(prompt, 2, extra_argv)`.
- Space around binary operators: `ret < 0`, `hash *= FNV_PRIME`, `buf[n] = '\0'`, `i + 1`.
- Pointer asterisk hugs the identifier: `const char *path`, `DmenuCtx *ctx`, `char *argv[]`. Function return-type asterisks sit against the type: `char *` (`utils/common/dmenu.h:17,24`).
- One statement per line. Comma-separated multiple declarations appear only for tightly-related locals (`size_t i, n;`, `int fd[2];`).
- `-std=c99 -pedantic -Wall -Wextra -Wno-unused-parameter -Os` in `utils/config.mk:8` and `slstatus/config.mk:15`.
- `-std=c99 -pedantic -Wall -Wno-deprecated-declarations -Os` in `dwm/config.mk:31`.
- `LDFLAGS = -s` strips binaries (`utils/config.mk:9`). Debug builds replace the CFLAGS line with `-g -O0`; see commented template at `dwm/config.mk:30`.
- `-Wno-unused-parameter` is the only warning silencer. Everything else is treated as an error to fix.
- `-D_DEFAULT_SOURCE` is set so POSIX extensions (`strdup`, `fileno`, `setsid`) compile under `-std=c99 -pedantic` (`utils/config.mk:8`, `slstatus/config.mk:14`).
## Import Organization
- No aliasing; every include uses a real relative path. `../common/util.h` from `utils/battery-notify/battery-notify.c:7`, `../util.h` from `slstatus/components/cpu.c:7`.
- `config.h` is always included *after* the system headers and is expected to exist locally in the same directory as the consuming `.c` (`utils/battery-notify/battery-notify.c:8`, `utils/dmenu-clip/dmenu-clip.c:14`, `utils/dmenu-clipd/dmenu-clipd.c:21`).
- slstatus inverts the normal rule: `slstatus/slstatus.c:24` includes `config.h` **after** file-scope globals are declared, because the generated `config.h` references symbols in `slstatus.h`. Do not move this include.
## Error Handling
| Behavior | `utils/common/util.c` | `slstatus/util.c` | `dwm/util.c` |
|---|---|---|---|
| Prepends `argv0:` to output | yes (`:19`) | no | no |
| Appends `strerror(errno)` when fmt ends with `:` | yes (via `perror(NULL)`, `:23-28`) | yes (via `perror(NULL)`, `:18-23`) | yes (via `strerror(saved_errno)`, `:22-24`) |
| Saves errno before use | no (relies on perror) | no | yes (`saved_errno = errno;`, `:16`) |
| Exit code of `die` | 1 | 1 | 1 |
| Has `ecalloc` helper | no | no | yes (`:30-37`) |
| Has `warn` | yes | yes | no |
- Trailing-colon convention: end `fmt` with `:` when you want `strerror(errno)` appended. Example: `die("fork:");` produces `"argv0: fork: Interrupted system call"` with utils/common, `"fork: ..."` with slstatus/dwm. See `utils/common/util.c:31-38`, `utils/common/dmenu.c:51,55`.
- Use `die()` for fatal errors that justify immediate exit. Use `warn()` for recoverable issues. `dwm/util.c` exposes only `die()` — recoverable errors are handled inline with `fputs(..., stderr)` (`dwm/dwm.c:2670`).
- Value-returning helpers return `NULL` or `-1` on failure, never exit. Example: `pscanf` returns `-1` (`utils/common/util.c:53-70`, `slstatus/util.c:124-141`), component functions return `NULL` (`slstatus/components/cpu.c:19,35,38,44`).
- Every `malloc`/`fopen`/`fork`/`pipe` return value is checked immediately after the call — no delayed checks. Examples: `utils/common/dmenu.c:35-56,78-94`, `utils/common/util.c:60-63,78-84`.
- `snprintf` truncation is always checked when the output feeds a syscall: `if (ret < 0 || (size_t)ret >= bufsz) return -1;` (`utils/dmenu-clip/dmenu-clip.c:98-99`, `utils/dmenu-clipd/dmenu-clipd.c:69-70,81-82`).
- `args[i].func` returning `NULL` is substituted with the global `unknown_str = "n/a"` (`slstatus/slstatus.c:91-92`, `slstatus/config.def.h:7`).
- `fmt_human` warns and returns `NULL` on invalid base (`slstatus/util.c:112-115`).
## Logging
- No logging framework. No syslog. No log files. All diagnostics go to stderr via `die()`/`warn()`.
- Format convention: `"function_name: description"` or `"syscall 'arg':"` (trailing colon triggers errno append).
- Examples:
- Output is not captured at runtime. Daemons started from `dwm-start` inherit the session's stderr; utilities spawned by dwm `spawn()` have stderr inherited from dwm.
## Comments
- Why, not what. Platform-specific units: `/* in kHz */`, `/* in MHz */` (`slstatus/components/cpu.c:17,66`).
- Non-obvious algorithms: `/* cpu user nice system idle iowait irq softirq */` above the `pscanf` in `slstatus/components/cpu.c:31`.
- Security/portability rationales: `/* Use lstat to detect symlinks (T-03-06 mitigation) */` (`utils/dmenu-clip/dmenu-clip.c:137`), `/* Use -f (not -x) because "betterlockscreen" is 16 chars, exceeding Linux's 15-char comm field (TASK_COMM_LEN). */` (`utils/dmenu-session/dmenu-session.c:46-47`).
- Known limitations marked explicitly: `/* FIXME not sure if I have to send these events, too */` style (search `dwm/dwm.c` for `FIXME`).
- Cross-references to spec/planning artifacts are allowed: `(D-01, D-02)`, `(T-03-06 mitigation)` — these are phase IDs from `.planning/`.
- `/* ... */` only — no `//` comments. Verified with no `//` matches in utils tree.
- Multi-line prose block comments at the top of a file for design notes (`dwm/dwm.c:1-22`, `utils/tests/test-dmenu.c:2-7`).
- Inline comments sit on the same line when short, otherwise on the previous line.
## Configuration Pattern
| Tool | Default | User copy | Generated by |
|---|---|---|---|
| dwm | `dwm/config.def.h` | `dwm/config.h` | `dwm/Makefile:16-17` |
| dmenu | `dmenu/config.def.h` | `dmenu/config.h` | `dmenu/Makefile:14-15` |
| st | `st/config.def.h` | `st/config.h` | `st/Makefile:12-13` |
| slstatus | `slstatus/config.def.h` | `slstatus/config.h` | `slstatus/Makefile:40-41` |
| battery-notify | `utils/battery-notify/config.def.h` | `utils/battery-notify/config.h` | manual `cp` (no rule) |
| dmenu-clip | `utils/dmenu-clip/config.def.h` | `utils/dmenu-clip/config.h` | manual `cp` (no rule) |
| dmenu-clipd | `utils/dmenu-clipd/config.def.h` | `utils/dmenu-clipd/config.h` | manual `cp` (no rule) |
- `config.def.h` is committed. `config.h` is **not** in `.gitignore` (verified — `.gitignore` only lists build artifacts and editor backups), so if `config.h` is modified it will show up in `git status`. Treat this as intentional — it is the user-local customization layer.
- `config.h` must exist before the `.o` is compiled. Suckless Makefiles express this as an explicit dep (`slstatus/Makefile:35`, `dwm/Makefile:14`, `utils/Makefile:31,59,66`).
- The utils tree does **not** yet auto-generate `config.h` from `config.def.h` — developers copy it by hand. Committed `config.h` files exist in the tree for all three utils that have them (checked via Grep).
- Keys in `config.def.h` are `#define` macros (`MAXLEN`, `BATTERY`, `MAX_ENTRIES`, …) or `static const` arrays (`keys[]`, `tags[]`, `args[]`). No runtime parsing.
## Platform Guards
#if defined(__linux__)
#elif defined(__OpenBSD__)
#elif defined(__FreeBSD__)
#endif
- `#ifdef XINERAMA` at `dwm/dwm.c:39-41` — wrapped around the Xinerama include; `XINERAMAFLAGS = -DXINERAMA` is set in `dwm/config.mk:15`.
- `#ifdef __OpenBSD__` at `dwm/dwm.c:2675-2678` — calls `pledge()` on OpenBSD. Harmless on Linux.
## Function Design
- `main()` in `dmenu-session.c` is ~30 lines (`utils/dmenu-session/dmenu-session.c:99-130`).
- `main()` in `slstatus.c` is ~90 lines (`slstatus/slstatus.c:48-135`) — contains the full status-tick loop.
- `dmenu_open()` is the longest helper at ~85 lines (`utils/common/dmenu.c:14-99`) — justified by the pipe+fork+argv-build complexity.
- 1–3 parameters typical. Pattern: input first, output buffer + size last. `build_preview(const char *path, char *preview, size_t prevsz)` (`utils/dmenu-clip/dmenu-clip.c:37`), `get_cache_dir(char *buf, size_t bufsz)` (`utils/dmenu-clip/dmenu-clip.c:83`, `utils/dmenu-clipd/dmenu-clipd.c:54`).
- `const char *` for strings the function reads but does not modify.
- `char *const argv[]` for argv-like vectors passed to exec helpers (`utils/common/util.h:14`).
- No variadic functions in application code — only `die`, `warn`, `pscanf`, `esnprintf`, `bprintf` — and they all funnel through `va_start`/`va_end`.
- `int` for status (0 = success, -1 = failure, or direct `exec_wait` passthrough).
- `const char *` for strings backed by a static buffer — the slstatus component signature: `const char *func(const char *arg)` (`slstatus/slstatus.h`). Returning `NULL` means "no data."
- `char *` (non-const) for allocated strings the caller must free. Example: `dmenu_read` returns `malloc`-owned memory (`utils/common/dmenu.h:22-24`, `utils/common/dmenu.c:119-137`).
- Struct pointers (`DmenuCtx *`) for lifecycle-managed handles.
## Module Design
- Two modules: `util.{c,h}` (error + exec + pscanf + sigchld) and `dmenu.{c,h}` (DmenuCtx API).
- Every utility links both object files: `utils/Makefile:7-8` defines `COMMON_OBJ` and `DMENU_OBJ`; every utility binary rule ends with `$(COMMON_OBJ)` and, if it needs dmenu, `$(DMENU_OBJ)` (`utils/Makefile:28-67`).
- `dmenu-clipd` skips `DMENU_OBJ` because it doesn't launch dmenu (`utils/Makefile:62-64`).
- Components are plug-ins. Each `.c` file in `slstatus/components/` exports one or more functions declared in `slstatus/slstatus.h:4-85`.
- Adding a new metric = drop `components/foo.c` → add `const char *foo(const char *)` to `slstatus.h` → add `components/foo` to the `COM` list in `slstatus/Makefile:8-30` → reference it in the `args[]` table in `slstatus/config.def.h:70-78`.
- Include guards in utils: `#ifndef UTIL_H / #define UTIL_H / ... / #endif` (`utils/common/util.h:2-18`, `utils/common/dmenu.h:2-29`).
- No `#pragma once`.
- Public headers declare only what consumers need. Static helpers stay in the `.c` file with `static` storage.
- Upstream suckless headers (`slstatus/slstatus.h`, `dwm/util.h`) use section comments to group declarations.
## Macros and Helpers
- `LEN(x)` array length (`utils/common/util.h:7`, replicated in every `config.h`/`util.h` pair). Standard suckless helper:
- Do-while wrappers for multi-statement macros: `test-dmenu.c` uses the idiom for `TEST`/`PASS`/`FAIL` (`utils/tests/test-dmenu.c:18-31`).
- No `MIN`/`MAX` in utils tree — the single arithmetic comparison (`cmp_mtime_desc` in `utils/dmenu-clip/dmenu-clip.c:24-34`) inlines the check.
- dwm ships `MAX` and `MIN` as inline utilities in `dwm/util.h` (upstream suckless convention).
- No feature flags. Conditional compilation is purely `__linux__ / __OpenBSD__ / __FreeBSD__ / XINERAMA`.
## Concurrency and Process Hygiene
- `SIGCHLD` is the default way to reap spawned children. Every utility that spawns calls `setup_sigchld()` at the top of `main()` (`utils/battery-notify/battery-notify.c:42`, `utils/dmenu-session/dmenu-session.c:106`, `utils/dmenu-cpupower/dmenu-cpupower.c:65`, `utils/dmenu-clip/dmenu-clip.c:117`).
- Exception: `dmenu-clipd` does **not** use `setup_sigchld` because it reaps via explicit `waitpid` when the child is a known single process.
- `SIGTERM`/`SIGINT` are handled by setting a `volatile sig_atomic_t done = 1;` flag, checked in the main loop (`slstatus/slstatus.c:21,26-31`, `utils/dmenu-clipd/dmenu-clipd.c:38,46-51`).
- `dmenu-clipd` explicitly sets `sa.sa_flags = 0` (no `SA_RESTART`) so syscalls unblock on signal (`utils/dmenu-clipd/dmenu-clipd.c:388`).
- Detached child processes call `setsid()` before `execvp` (`utils/common/util.c:98`, `dwm/dwm.c:2008`).
- Single-instance enforcement via `flock(LOCK_EX|LOCK_NB)` (`utils/dmenu-clipd/dmenu-clipd.c:84-92`).
<!-- GSD:conventions-end -->

<!-- GSD:architecture-start source:ARCHITECTURE.md -->
## Architecture

## Pattern Overview
- No runtime config files. Configuration is compiled in via `config.h` (generated from `config.def.h` on first build). Every tool's Makefile enforces this pattern.
- No framework, no scripting runtime, no daemons beyond what the user starts in `dwm-start`.
- Two in-tree codebases: **vendored upstream suckless tools** (`dwm/`, `st/`, `dmenu/`, `slstatus/`) each with their own `Makefile`, `config.mk`, `util.c`, and `drw.c` — and **in-house utilities** (`utils/`) that share a common `util.c`/`dmenu.c` and build as a single project.
- Utilities delegate interactive UI to `dmenu` via a reusable C wrapper (`utils/common/dmenu.{c,h}`) instead of embedding X11 input handling.
- Session/power actions route through `loginctl` so the same binary works on systemd (Arch) and elogind (Artix).
## Layers
- Purpose: Order in which X clients and daemons start when the X session begins.
- Location: `dwm-start`, `.xprofile`.
- Contains: background launches of `lxpolkit`, `fcitx5`, `dmenu-clipd`, `dunst`; the `slstatus` supervisor loop; `feh` wallpaper; `exec dwm`.
- Depends on: `$HOME/.local/bin` on `PATH` (`dwm-start:3`).
- Used by: `xinit`/`startx`/display manager.
- Purpose: Own the root window; arrange clients; dispatch keybinds; draw the bar.
- Location: `dwm/dwm.c` (2685 lines), `dwm/drw.c` (drawing primitives), `dwm/config.h` (compiled-in config).
- Contains: `Client` and `Monitor` structs (`dwm/dwm.c:104-117,131-148`), event handler table indexed by `XEvent.type`, layouts (`tile`, `monocle`, floating; `dwm/config.def.h:57-62`), keybind + mouse button tables (`dwm/config.def.h:85-148`).
- Depends on: Xlib, Xft, Xinerama, XFT fonts via `drw`, EWMH + systray atoms.
- Used by: every other X client on the session.
- Purpose: User-facing terminal window; shell host.
- Location: `st/st.c` (VT/PTY state machine), `st/x.c` (X11 rendering and event input), `st/config.h`.
- Contains: PTY fork, ANSI/VT100 parser, glyph cache, selection handling.
- Depends on: X11, Xft, libutil (PTY), libm, librt.
- Used by: spawned from dwm via `MODKEY+Return` (`dwm/config.def.h:83,88`).
- Purpose: Present a filterable list of strings, return the user's choice on stdout.
- Location: `dmenu/dmenu.c`, `dmenu/drw.c`, plus the shell launchers `dmenu_run*` and `dmenu_path*`.
- Contains: item filtering, text input, vertical/horizontal layout, keyboard grab, selection output.
- Depends on: Xlib, Xft, Xinerama; stdin/stdout.
- Used by: dwm keybindings and in-house C utilities via `utils/common/dmenu.c`.
- Purpose: Periodically collect system metrics and write a status string to the root window name.
- Location: `slstatus/slstatus.c` (main loop + args table), `slstatus/components/*.c` (22 components), `slstatus/config.h` (the format string table).
- Contains: `struct arg { func, fmt, args }` table (`slstatus/slstatus.c:14-18`), component functions returning `const char *` or `NULL`, `bprintf`/`fmt_human` helpers (`slstatus/util.c:79-122`).
- Depends on: Xlib (for `XStoreName`), procfs, sysfs.
- Used by: dwm reads the root window name and renders the bar.
- Purpose: Render popup notifications.
- Location: External binary; config at `dunst/dunstrc`.
- Contains: urgency-based styling (low/normal/critical), placement, timeout rules.
- Depends on: Xlib, libnotify D-Bus interface.
- Used by: `utils/battery-notify`, `utils/brightness-notify`, and any application using `notify-send`. Screenshots go through `flameshot` directly (not a utils/ binary).
- Purpose: Session/power menu, CPU profile picker, clipboard daemon + browser, battery alerts, brightness OSD.
- Location: `utils/{battery-notify,brightness-notify,dmenu-session,dmenu-cpupower,dmenu-clip,dmenu-clipd}/*.c`.
- Shared code: `utils/common/util.{c,h}` (die/warn, pscanf, exec helpers, SIGCHLD setup) and `utils/common/dmenu.{c,h}` (dmenu IPC wrapper).
- Depends on: `dmenu` binary (launched as a child), `xclip`, `notify-send`, `loginctl`, `betterlockscreen`, `powerprofilesctl`.
- Used by: dwm keybinds, `dwm-start` daemon list, external hotkeys (battery-notify is designed to be called from a cron/timer or future hotkey).
- Purpose: Historical reference implementations of the above.
- Location: `scripts/dmenu-clip`, `scripts/dmenu-clipd`, `scripts/dmenu-cpupower`, `scripts/dmenu-session`.
- Status: Kept as reference only — not installed. `install.sh:55-57` explicitly notes they are superseded by the C utilities (which use betterlockscreen and powerprofilesctl).
## Data Flow
- dwm: in-memory linked lists of `Client`s per `Monitor`; focus history via `snext` stack pointer (`dwm/dwm.c:113-114`).
- slstatus: stateless each tick; CPU component keeps a file-static `long double a[7]` buffer across ticks for delta calc (`slstatus/components/cpu.c:27-49`).
- dmenu-clipd: state lives on disk; no in-memory cache beyond the current selection.
- dmenu-clip: rebuilds its entry list each invocation from the cache dir.
## Key Abstractions
- Purpose: An X11 window managed by dwm.
- Location: `dwm/dwm.c:104-117`.
- Pattern: Fixed struct with forward pointers (`next`, `snext`), tag bitmask, geometry, behavior flags (`isfloating`, `isfullscreen`, `isfixed`, `neverfocus`).
- Purpose: A physical output; owns a list of clients, selected layout, bar geometry.
- Location: `dwm/dwm.c:131-148`.
- Pattern: Linked list of monitors; each monitor has dual tag sets (`tagset[2]`) and `seltags` toggle for quick "last tag" swap.
- Purpose: Row in the status bar table; pluggable component descriptor.
- Location: `slstatus/slstatus.c:14-18`.
- Pattern:
- Purpose: Read one system metric, return a `const char *` pointer into a static buffer or `NULL` on failure.
- Location: `slstatus/components/*.c`; declarations in `slstatus/slstatus.h`.
- Pattern: `const char *func(const char *arg)` — arg is NULL for metrics that don't need one (e.g., `cpu_perc`). Result points into `buf[1024]` via `bprintf()` (`slstatus/util.c:79-90`). Never allocates.
- Purpose: Tile strategy.
- Location: `dwm/dwm.c` (`Layout` typedef near client defs), `dwm/config.def.h:57-62`.
- Pattern: `{ symbol, arrange_fn }`. `arrange_fn == NULL` means floating.
- Purpose: Drawable abstraction over X11 + Xft.
- Location: `dwm/drw.{c,h}`, `dmenu/drw.{c,h}` — **byte-identical** copies maintained in both trees. Verified via `diff -q` (no output).
- Pattern: Create once (`drw_create`), allocate color schemes (`drw_scm_create`), draw rects/text, `drw_map` to flush to the real window.
- Purpose: Map input event to function pointer + `Arg`.
- Location: `dwm/dwm.c:119-124` (`Key`), `dwm/dwm.c:94-100` (`Button`).
- Pattern: Struct with modifier mask, keysym/button, `void (*func)(const Arg *)`, and an `Arg` union (`int i`, `unsigned ui`, `float f`, `const void *v`).
- Purpose: Bidirectional handle to a child `dmenu` process.
- Location: `utils/common/dmenu.h:8-12`.
- Pattern:
- Purpose: In-memory row for one clipboard cache file.
- Location: `utils/dmenu-clip/dmenu-clip.c:16-21`.
- Pattern: Fixed-size `hash[NAME_MAX+1]`, `preview[MAX_PREVIEW+1]`, `path[PATH_MAX]`, plus `mtime`. Bounded arrays — no dynamic allocation in the hot path.
## Entry Points
- `dwm/dwm.c:2662` — parses `-v`, opens display, `checkotherwm()`, `setup()`, `scan()`, `run()`, optional restart via `execvp(argv[0], argv)`, `cleanup()`.
- `st/x.c` — st's real entry point (st.c is the engine, x.c has main).
- `dmenu/dmenu.c:760` — parses many single-letter flags, reads stdin, grabs keyboard, draws, writes selection.
- `slstatus/slstatus.c:48` — uses the `ARGBEGIN`/`ARGEND` macro from `slstatus/arg.h:8-26` to parse `-v/-s/-1`, installs signal handlers, opens display unless `-s`, enters the status loop.
- `utils/battery-notify/battery-notify.c:33` — one-shot: read, check, notify, exit.
- `utils/dmenu-session/dmenu-session.c:99` — show menu, dispatch to `action_lock/logout/reboot/shutdown`.
- `utils/dmenu-cpupower/dmenu-cpupower.c:54` — read current profile, show menu, validate, `powerprofilesctl set`.
- `utils/dmenu-clip/dmenu-clip.c:104` — scan cache, show menu, restore via xclip.
- `utils/dmenu-clipd/dmenu-clipd.c:367` — acquire flock, set signal handlers, init X11+XFixes, event loop.
- `dwm-start` is the intended `.xinitrc`. It is installed to `$HOME/.local/bin/dwm-start` by `install.sh:59-61`.
## Error Handling
- Shared `die()`/`warn()` implementations in two places with the same semantics:
- Trailing colon convention: if `fmt` ends with `:`, `verr()` appends the current `strerror(errno)` via `perror(NULL)`. Seen at `utils/common/util.c:23-28`.
- Allocation guards: every `malloc`/`fopen`/`fork` is checked (`utils/common/util.c:79-85`, `utils/common/dmenu.c:35-56`).
- `pscanf()` wrapper centralizes "open, scanf, close, return count" for sysfs/procfs reads (`utils/common/util.c:53-70`, duplicated in `slstatus/util.c:124-141`).
- `esnprintf()` in slstatus treats truncation as an error and `warn()`s (`slstatus/util.c:48-64`).
- `die()` on X failures is universal: `XOpenDisplay`, `XFixesQueryExtension`, `XStoreName`, etc.
- No retry loops. No exponential backoff. If something fundamental fails, the process exits and the user/supervisor restarts it (for `slstatus`, that's the shell loop in `dwm-start:19-22`).
- slstatus components return `NULL` on read failure; the main loop substitutes `unknown_str = "n/a"` (`slstatus/slstatus.c:91-92`, `slstatus/config.def.h:7`).
- `dmenu-clipd` falls back from UTF8_STRING to STRING when a clipboard owner can't produce UTF-8 (`utils/dmenu-clipd/dmenu-clipd.c:195-207`).
- `dmenu-cpupower` prints `"unknown"` in the prompt if `powerprofilesctl get` fails (`utils/dmenu-cpupower/dmenu-cpupower.c:68-69`).
## Cross-Cutting Concerns
- Every utility that spawns children installs a `SIGCHLD` handler that reaps with `waitpid(-1, NULL, WNOHANG)` in a loop (`utils/common/util.c:105-123`). Called explicitly via `setup_sigchld()` at `main()` top.
- Detached children use `setsid()` to divorce from the controlling tty (`utils/common/util.c:98`, `dwm/dwm.c:2008`).
- dwm's `spawn()` closes the X connection in the child before `execvp` so leaked fds don't confuse the new process (`dwm/dwm.c:2006-2007`).
- `dmenu-clipd` uses `flock(LOCK_EX|LOCK_NB)` on `<cache>/.lock`. Second instance exits with "already running" (`utils/dmenu-clipd/dmenu-clipd.c:75-93`).
- `dmenu-session` prevents duplicate lock screens with `pgrep -f betterlockscreen` (`utils/dmenu-session/dmenu-session.c:45-49`).
- All dynamic path builds go through `snprintf` + length check (`utils/dmenu-clipd/dmenu-clipd.c:80-82`, `utils/dmenu-clip/dmenu-clip.c:98-99,132-135`).
- `dmenu-clip` uses `lstat` (not `stat`) and rejects symlinks + non-regular files to prevent a malicious symlink in the cache dir from leaking arbitrary files into the clipboard (`utils/dmenu-clip/dmenu-clip.c:137-143`).
- `dmenu-clipd` opens cache entries with mode `0600` (`utils/dmenu-clipd/dmenu-clipd.c:309`).
- Input validation: `dmenu-cpupower` whitelists selections before passing to `powerprofilesctl` (`utils/dmenu-cpupower/dmenu-cpupower.c:85-90`).
- All diagnostics go to stderr via `warn()`/`die()`. No log files. No syslog.
- On a typical session, stderr from user utilities is not captured (they're launched by dwm `spawn`, which does not redirect). Daemons launched from `dwm-start` inherit the session's stderr.
- `SIGTERM`/`SIGINT` handled cooperatively by `dmenu-clipd` (sets `done = 1`, drops out of event loop cleanly, closes display, releases flock) — `utils/dmenu-clipd/dmenu-clipd.c:46-51,386-390,397-404`.
- `slstatus` handles `SIGINT`/`SIGTERM` for clean exit and `SIGUSR1` as a forced-refresh trigger (`slstatus/slstatus.c:28-30,75-80`).
- slstatus components wrap platform-specific code in `#if defined(__linux__) / __OpenBSD__ / __FreeBSD__` blocks (e.g., `slstatus/components/cpu.c:9`). Even though this project targets Linux, the upstream multi-platform gates are kept.
- dwm has an `#ifdef __OpenBSD__ pledge(...)` call (`dwm/dwm.c:2675-2678`) — harmless on Linux but retained from upstream.
- Patched into dwm via `dwm-status2d-systray-6.4.diff` (`dwm/patches/`). Systray positioning/pinning is tunable in `dwm/config.def.h:17-21`.
<!-- GSD:architecture-end -->

<!-- GSD:skills-start source:skills/ -->
## Project Skills

No project skills found. Add skills to any of: `.claude/skills/`, `.agents/skills/`, `.cursor/skills/`, or `.github/skills/` with a `SKILL.md` index file.
<!-- GSD:skills-end -->

<!-- GSD:workflow-start source:GSD defaults -->
## GSD Workflow Enforcement

Before using Edit, Write, or other file-changing tools, start work through a GSD command so planning artifacts and execution context stay in sync.

Use these entry points:
- `/gsd-quick` for small fixes, doc updates, and ad-hoc tasks
- `/gsd-debug` for investigation and bug fixing
- `/gsd-execute-phase` for planned phase work

Do not make direct repo edits outside a GSD workflow unless the user explicitly asks to bypass it.
<!-- GSD:workflow-end -->



<!-- GSD:profile-start -->
## Developer Profile

> Profile not yet configured. Run `/gsd-profile-user` to generate your developer profile.
> This section is managed by `generate-claude-profile` -- do not edit manually.
<!-- GSD:profile-end -->
