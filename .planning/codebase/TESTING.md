# Testing Patterns

**Analysis Date:** 2026-04-09

## Test Framework

**Runner:**
- No automated testing framework detected
- All testing is manual and integration-focused

**Assertion Library:**
- None; suckless projects emphasize runtime correctness over unit tests

**Run Commands:**
```bash
make clean install        # Build and install (implicit testing)
make                      # Build only
make clean                # Remove build artifacts
```

No dedicated test commands. Verification happens through:
1. Compilation without warnings
2. Manual runtime testing with real X11 environment
3. Cross-platform testing (Linux, OpenBSD, FreeBSD variants)

## Test File Organization

**Location:**
- No test files exist in codebase
- Testing is done externally via user interaction
- Single exception: `dmenu/stest.c` is a file testing utility (not a unit test framework)

**Naming:**
- N/A (no test files)

**Structure:**
- Suckless philosophy: simple code that works > complicated test suites
- Preference for correctness by design, not test-driven development

## Test Structure

**Suite Organization:**
- No test suites
- Each tool tested independently through actual use

**Patterns:**
- Manual testing of each component
- Platform-specific builds and testing: `make clean install` on Linux, OpenBSD, FreeBSD
- Configuration changes require recompilation and reinstall

## Component Testing Approach

**slstatus components:**
- Each component (e.g., `cpu_perc()`, `ram_used()`) tested by running `slstatus` and checking status bar output
- Component functions return `NULL` on error, visible as "n/a" in status bar
- Return values formatted via `bprintf()` which writes to global buffer and returns it

**Example: cpu.c testing**
- Linux implementation reads from `/proc/stat`
- OpenBSD uses `sysctl()` with `KERN_CPTIME`
- FreeBSD uses `sysctlbyname("kern.cp_time")`
- Manual testing: run `slstatus` on each OS and verify CPU percentage appears

**dwm/dmenu/st components:**
- Tested through actual X11 window manager/menu/terminal use
- No programmatic testing

**Shell scripts (dmenu-clip, dmenu-clipd, etc.):**
- Manual testing by running scripts and checking clipboard functionality
- `dmenu-clipd` tested by pasting text and checking cache directory
- `dmenu-clip` tested by selecting from clipboard menu

## Mocking

**Framework:** None

**Patterns:**
- No mocking in suckless tools
- Integration testing only—functions call real system APIs

**What to Mock:**
- Nothing; direct system calls are expected

**What NOT to Mock:**
- System calls should never be mocked; testing should use real `/proc`, `/sys`, sysctl, etc.

## Fixtures and Factories

**Test Data:**
- No fixtures or factories exist
- Component functions accept string arguments for paths/interfaces
- Example: `cpu_freq()` reads from `CPU_FREQ` macro which points to `/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq`

**Location:**
- Not applicable

## Coverage

**Requirements:** None enforced

**View Coverage:**
- No coverage tools used
- Code review and manual testing used instead

## Test Types

**Unit Tests:**
- Not used
- Suckless philosophy: each component function is small enough to be verified by inspection
- Simple algorithms (reading files, parsing output, formatting strings) don't require unit tests

**Integration Tests:**
- Manual testing of full system
- Running `slstatus` and checking status bar output
- Running dwm and checking window management behavior
- Running dmenu and testing menu selection

**E2E Tests:**
- Not formally defined
- End-to-end testing is the primary testing method
- User-driven: does the tool work when used?

## Testing Philosophy

**Suckless Approach:**
- Code simplicity over test coverage
- Small functions (typically <50 lines) are self-evidently correct
- Platform-specific code enclosed in preprocessor blocks for easy code review
- Compilation is first gate: code must compile without warnings
- Manual testing is primary verification method

**Verification Steps:**
1. Build: `make clean install` in each component directory
2. Compile without warnings or errors
3. Manual runtime testing in real environment
4. Cross-platform testing on Linux, OpenBSD, FreeBSD where applicable

**Example Testing Flow for slstatus:**
```bash
cd slstatus
make clean              # Remove old build
make                    # Compile (must succeed with no warnings)
sudo make install       # Install to /usr/local/bin
slstatus                # Run and verify output in X11 environment
# Check status bar text includes CPU%, RAM, battery, etc.
```

## Error Handling Testing

**Error Scenarios:**
- Functions return `NULL` on failure (file not found, parsing error, system call failure)
- Display shows "n/a" (from `unknown_str` in config) when component returns `NULL`
- Manual verification: try reading non-existent file, verify "n/a" appears

**Example from `ram_free()` in `components/ram.c`:**
```c
const char *
ram_free(const char *unused)
{
	uintmax_t free;
	FILE *fp;

	if (!(fp = fopen("/proc/meminfo", "r")))
		return NULL;  /* File not found → returns NULL → displays "n/a" */

	if (lscanf(fp, "MemFree:", "%ju kB", &free) != 1) {
		fclose(fp);
		return NULL;  /* Parse error → returns NULL → displays "n/a" */
	}

	fclose(fp);
	return fmt_human(free * 1024, 1024);
}
```

## Platform-Specific Testing

**Linux Testing:**
- Read from `/proc/stat`, `/proc/meminfo`, `/sys` filesystem
- Example: `cpu.c` uses `pscanf()` to read `/proc/stat`
- Test by running on actual Linux system and verifying output

**OpenBSD Testing:**
- Use `sysctl()` system calls with mib arrays
- Example: `cpu.c` uses `sysctl(mib, 2, &freq, &size, NULL, 0)`
- Test by running on actual OpenBSD system

**FreeBSD Testing:**
- Use `sysctlbyname()` for named sysctl access
- Example: `cpu.c` uses `sysctlbyname("hw.clockrate", &freq, ...)`
- Test by running on actual FreeBSD system

**Build Verification:**
- Conditional compilation ensures only relevant code compiles on each OS
- Compilation failure indicates platform incompatibility
- Successfully building = passing test

## Shell Script Testing

**dmenu-clip and dmenu-clipd:**
- Manual testing by pasting text and checking clipboard menu
- Verify cache directory creation: `ls -la ~/.cache/dmenu-clipboard`
- Verify newest entries appear first in menu

**Example testing `dmenu-clipd`:**
```bash
# Start daemon in background
~/.local/bin/dmenu-clipd &
sleep 1

# Paste text (via Ctrl+V or xclip)
echo "test content" | xclip -selection clipboard

# Check cache was created
ls -la ~/.cache/dmenu-clipboard | head -5

# Verify daemon didn't crash
ps aux | grep dmenu-clipd
```

**Install and build validation:**
```bash
cd /path/to/suckless-environment
./install.sh                # Build and install everything
# Manual testing follows:
# - Launch dwm (Mod4+Return to open terminal)
# - Test key bindings
# - Paste to test dmenu-clipd
# - Run dmenu-cpupower to test dmenu integration
```

## Known Testing Gaps

**Untested Components:**
- WiFi functions: `wifi_essid()`, `wifi_perc()` require specific interface and signal availability
- Battery functions: `battery_perc()`, `battery_state()` only work on systems with batteries
- Keyboard indicators: Platform and layout dependent

**Testing Limitation:**
- Suckless tools designed for minimal dependencies and simplicity
- Unit testing framework would contradict design philosophy
- Component correctness verified through code inspection, not automated tests

## Quality Assurance

**Code Review:**
- Manual review of changes
- Check for K&R style adherence
- Verify platform-specific code properly guarded
- Ensure error handling returns `NULL` consistently

**Compilation Standards:**
- All code must compile without warnings
- CFLAGS typically include: `-Wall -Wextra -std=c99` or similar
- Compiler warnings treated as errors in practice

**Integration Validation:**
- Full tool tested on target platform after build
- Configuration changes require recompilation to take effect
- User testing in actual X11 environment is final verification

---

*Testing analysis: 2026-04-09*
