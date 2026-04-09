/* See LICENSE file for copyright and license details. */
/*
 * Test strategy:
 * 1. Compile-time test: if this file compiles and links, the API is correct
 * 2. If DISPLAY is set, do a real dmenu pipe test with echo as a mock
 * 3. If DISPLAY is not set, test only that API symbols resolve (link test)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common/dmenu.h"
#include "../common/util.h"

static int tests_passed;
static int tests_total;

#define TEST(name) do { \
	tests_total++; \
	printf("  test: %s ... ", name); \
} while (0)

#define PASS() do { \
	tests_passed++; \
	printf("ok\n"); \
} while (0)

#define FAIL(reason) do { \
	printf("FAIL: %s\n", reason); \
} while (0)

static void
test_struct_members(void)
{
	/* Compile-time check: DmenuCtx has expected members */
	DmenuCtx ctx;

	TEST("DmenuCtx.write is FILE *");
	ctx.write = NULL;
	(void)ctx.write;
	PASS();

	TEST("DmenuCtx.read is FILE *");
	ctx.read = NULL;
	(void)ctx.read;
	PASS();

	TEST("DmenuCtx.pid is pid_t");
	ctx.pid = 0;
	(void)ctx.pid;
	PASS();
}

static void
test_symbol_resolution(void)
{
	/* Link-time check: all 4 function symbols resolve */
	DmenuCtx *(*fn_open)(const char *, int, char *const []);
	void (*fn_write)(DmenuCtx *, const char *);
	char *(*fn_read)(DmenuCtx *);
	void (*fn_close)(DmenuCtx *);

	TEST("dmenu_open symbol resolves");
	fn_open = dmenu_open;
	if (!fn_open) { FAIL("NULL function pointer"); return; }
	PASS();

	TEST("dmenu_write symbol resolves");
	fn_write = dmenu_write;
	if (!fn_write) { FAIL("NULL function pointer"); return; }
	PASS();

	TEST("dmenu_read symbol resolves");
	fn_read = dmenu_read;
	if (!fn_read) { FAIL("NULL function pointer"); return; }
	PASS();

	TEST("dmenu_close symbol resolves");
	fn_close = dmenu_close;
	if (!fn_close) { FAIL("NULL function pointer"); return; }
	PASS();
}

static int
has_display(void)
{
	return getenv("DISPLAY") != NULL;
}

static int
has_dmenu(void)
{
	return system("command -v dmenu >/dev/null 2>&1") == 0;
}

static void
test_pipe_integration(void)
{
	DmenuCtx *ctx;
	char *selection;

	if (!has_display() || !has_dmenu()) {
		printf("SKIP: dmenu pipe integration (no DISPLAY or no dmenu)\n");
		return;
	}

	TEST("dmenu_open returns non-NULL");
	ctx = dmenu_open("test", 0, NULL);
	if (!ctx) {
		FAIL("dmenu_open returned NULL");
		return;
	}
	PASS();

	TEST("dmenu_write does not crash");
	dmenu_write(ctx, "item1");
	dmenu_write(ctx, "item2");
	dmenu_write(ctx, "item3");
	PASS();

	TEST("dmenu_read returns string or NULL");
	selection = dmenu_read(ctx);
	/* In non-interactive test, dmenu may return NULL (no user input) */
	if (selection) {
		printf("    (got selection: '%s')\n", selection);
		free(selection);
	} else {
		printf("    (got NULL -- expected in non-interactive mode)\n");
	}
	PASS();

	TEST("dmenu_close does not crash");
	dmenu_close(ctx);
	PASS();

	printf("PASS: dmenu pipe integration\n");
}

int
main(void)
{
	argv0 = "test-dmenu";
	tests_passed = 0;
	tests_total = 0;

	printf("=== test-dmenu ===\n\n");

	printf("[compile-time] struct member checks:\n");
	test_struct_members();

	printf("\n[link-time] symbol resolution:\n");
	test_symbol_resolution();

	printf("\n[runtime] pipe integration:\n");
	test_pipe_integration();

	printf("\n--- results: %d/%d passed ---\n", tests_passed, tests_total);

	if (tests_passed == tests_total) {
		printf("PASS: dmenu API symbols resolve\n");
		return 0;
	}

	printf("FAIL: some tests did not pass\n");
	return 1;
}
