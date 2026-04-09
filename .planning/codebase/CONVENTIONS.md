# Coding Conventions

**Analysis Date:** 2026-04-09

## Naming Patterns

**Files:**
- C source files: lowercase with underscore separation (e.g., `cpu.c`, `datetime.c`, `netspeeds.c`)
- Header files: uppercase for config headers (`config.h`, `config.def.h`); lowercase for utility headers (`util.h`, `slstatus.h`)
- Shell scripts: lowercase with hyphens (e.g., `dmenu-clip`, `dmenu-clipd`, `dmenu-cpupower`)
- Component modules: organized in `components/` directory with individual `.c` files per feature

**Functions:**
- Snake_case with descriptive names: `get_gid()`, `cpu_freq()`, `battery_perc()`, `netspeed_rx()`
- Function pointers in structs follow same convention: `(*func)(const char *)`
- Static helper functions prefixed with context: `load_uvmexp()`, `pagetok()`
- Parameter names either descriptive or `unused` when intentionally not used (e.g., `cpu_freq(const char *unused)`)

**Variables:**
- Snake_case for local and static variables: `free_pages`, `total_size`, `polling_interval`
- Single letter or abbreviated names acceptable for loop counters and mathematical operations: `i`, `n`, `sum`, `freq`
- Prefix `last_` for tracking previous state: `last_sum`, `last_buffer`
- Global state variables in uppercase prefixed with context: `CPU_FREQ`, `CACHE_DIR`

**Types:**
- Standard POSIX types preferred: `FILE`, `time_t`, `uintmax_t`, `struct timespec`
- Custom structs defined inline where needed (e.g., `struct arg` in `slstatus.c`)
- No typedef abuse—use `struct` keyword directly in most cases
- Enum names descriptive and grouped by purpose (e.g., `enum { SchemeNorm, SchemeSel }`)

**Constants:**
- #define macro names in UPPERCASE: `MAXLEN`, `MAX_PREVIEW`, `MAX_ENTRIES`, `CACHE_DIR`
- Color/style constants in UPPERCASE: `LEN(x)`, `WIDTH(X)`, `HEIGHT(X)`
- Version macros: `VERSION_MAJOR`, `VERSION_MINOR`

## Code Style

**Formatting:**
- K&R style braces: opening brace on same line, closing on new line
- Indentation: tabs (default suckless standard)
- Line length: generally fits in 80 columns; pragmatic exceptions allowed for long format strings or paths
- No trailing whitespace

**Example:**
```c
static void
terminate(const int signo)
{
	if (signo != SIGUSR1)
		done = 1;
}
```

**Linting:**
- No enforced linter; code follows suckless conventions manually
- Code review based on clarity and POSIX compliance
- Warning suppression avoided—warnings indicate problems to fix

**Spacing:**
- Single space after control flow keywords: `if (`, `while (`, `for (`
- No space between function name and parenthesis for calls: `printf(...)`, `fopen(...)`
- Space around operators: `a + b`, `x = 5`, `n != 1`
- Multiple statements rare; typically one per line

## Import Organization

**Order:**
1. Standard C library headers: `<stdio.h>`, `<stdlib.h>`, `<string.h>`, `<errno.h>`
2. POSIX system headers: `<unistd.h>`, `<sys/types.h>`, `<sys/sysctl.h>`, `<pwd.h>`
3. X11 headers (if used): `<X11/Xlib.h>`, `<X11/Xft/Xft.h>`, `<X11/extensions/Xinerama.h>`
4. Local project headers: `"util.h"`, `"slstatus.h"`, `"drw.h"`

**Example from `cpu.c`:**
```c
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../slstatus.h"
#include "../util.h"
```

**Conditional Includes:**
- Platform-specific headers guarded by preprocessor conditionals: `#if defined(__linux__)`
- Allows single codebase across Linux, OpenBSD, FreeBSD

**Path Aliases:**
- Relative paths used for local includes: `"../util.h"`, `"../../util.h"`
- No alias system; paths reflect actual directory structure

## Error Handling

**Patterns:**
- Return `NULL` on error from value-returning functions: `pscanf()`, `fmt_human()`, `bprintf()`
- Use `die()` for fatal errors that should exit program: `die("usage: %s [-v] [-s] [-1]", argv0)`
- Use `warn()` for non-fatal errors that allow continuation: `warn("vsnprintf: Output truncated")`
- Check return values explicitly: `if (!(fp = fopen(path, "r"))) return NULL`

**Error Messages:**
- Include context function name: `warn("strftime: Result string exceeds buffer size")`
- Include specific reason: `die("XOpenDisplay: Failed to open display")`
- Message format: `function_name: description` or `system_call: description`

**Null Checks:**
- Inverted logic common: `if (!(pw = getpwuid(geteuid()))) return NULL;`
- Guard allocations: all `malloc`/`fopen` results checked immediately

## Logging

**Framework:** Direct `fprintf(stderr, ...)` or `printf()` calls

**Patterns:**
- Error output to stderr via `warn()` function in `util.c`
- Info messages can go to stdout (not used much in suckless tools)
- `verr()` helper centralizes error formatting

**Example:**
```c
void
warn(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	verr(fmt, ap);
	va_end(ap);
}
```

## Comments

**When to Comment:**
- Explain non-obvious algorithms or workarounds: See `cpu.c` line 31 for CPU stats parsing
- Clarify platform-specific code: `/* in kHz */`, `/* in MHz */`
- Mark configuration: `/* Tokyo Night colors for status2d */`
- Indicate known limitations: `/* FIXME not sure if I have to send these events, too */`

**Documentation Format:**
- Copyright header in every file: `/* See LICENSE file for copyright and license details. */`
- Multi-line comments for design notes at top of files (e.g., `dwm.c` lines 1-22)
- Single-line comments for inline explanations: `/* cpu user nice system idle iowait irq softirq */`

**JSDoc/TSDoc:**
- Not used; C doesn't have standard format
- Function signatures serve as documentation via header files (`.h` files)

## Function Design

**Size:** 
- Functions typically 20-50 lines; longer functions often indicate complex platform-specific logic
- Some exceptions in component modules for platform conditionals (up to 100+ lines for multi-platform support)
- Complexity preferred over premature abstraction

**Parameters:**
- Keep minimal: 1-3 parameters typical
- Use `const char *` for format strings and paths
- Unused parameters marked with `unused` identifier: `cpu_freq(const char *unused)`
- No variadic functions except utility wrappers (`warn()`, `die()`, `bprintf()`)

**Return Values:**
- Functions return `const char *` for display strings
- Return `NULL` on failure (universal pattern)
- Return `int` for status codes (usually count or error indicator)
- Multiple outputs achieved through pointer parameters (e.g., `memcpy(b, a, sizeof(b))`)

**Example from `slstatus.c`:**
```c
static void
difftimespec(struct timespec *res, struct timespec *a, struct timespec *b)
{
	res->tv_sec = a->tv_sec - b->tv_sec - (a->tv_nsec < b->tv_nsec);
	res->tv_nsec = a->tv_nsec - b->tv_nsec +
	               (a->tv_nsec < b->tv_nsec) * 1E9;
}
```

## Module Design

**Exports:**
- Component modules export single or related set of functions via header files
- `slstatus.h` declares all public component functions
- Static helper functions kept private with `static` keyword
- Naming exports: `battery_perc()`, `battery_state()`, `cpu_freq()`, etc.

**Barrel Files:**
- Header file aggregation used: `slstatus.h` collects all component declarations
- Avoids circular dependencies by centralizing declarations

**Internal Organization:**
- Each component (e.g., `cpu.c`) fully self-contained
- Platform-specific code enclosed in `#if defined(...)` blocks
- Shared utilities in `util.c` and included via `util.h`

**Example Component Structure (`components/cpu.c`):**
```c
/* Copyright and includes */
#include "../slstatus.h"
#include "../util.h"

/* Platform-specific implementations */
#if defined(__linux__)
	const char * cpu_freq(const char *unused) { ... }
	const char * cpu_perc(const char *unused) { ... }
#elif defined(__OpenBSD__)
	/* Different implementation */
#elif defined(__FreeBSD__)
	/* Different implementation */
#endif
```

## Configuration Pattern

**Config Files:**
- Default configuration in `config.def.h`: contains all defaults
- User config created from defaults: `config.h` (copied from `.def.h`)
- Configuration loaded at compile time via `#include "config.h"`
- No runtime config files

**Example from `slstatus/config.h`:**
```c
const unsigned int interval = 1000;
static const char unknown_str[] = "n/a";
#define MAXLEN 2048

static const struct arg args[] = {
	{ cpu_perc, "^c#7aa2f7^ ^c#a9b1d6^ %s%%", NULL },
	{ ram_used, "^c#16161e^ | ^c#bb9af7^ ^c#a9b1d6^ %s", NULL },
	/* ... */
};
```

## Macro Conventions

**Utility Macros:**
- `LEN(x)` for array length: `#define LEN(x) (sizeof(x) / sizeof((x)[0]))`
- Comparison macros: `MIN()`, `MAX()`
- Simple abstractions for code clarity

**Build Macros:**
- Conditional compilation used extensively: `#if defined(__linux__)`
- Platform gates allow single codebase
- No feature flags—only OS/architecture detection

---

*Convention analysis: 2026-04-09*
