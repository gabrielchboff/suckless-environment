/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../common/dmenu.h"
#include "../common/util.h"

static int
confirm(const char *prompt, char *const extra_argv[])
{
	DmenuCtx *ctx;
	char *sel;
	int yes;

	ctx = dmenu_open(prompt, 2, extra_argv);
	dmenu_write(ctx, "no");
	dmenu_write(ctx, "yes");
	sel = dmenu_read(ctx);
	dmenu_close(ctx);

	yes = (sel && strcmp(sel, "yes") == 0);
	free(sel);
	return yes;
}

static void
action_lock(void)
{
	const char *display;
	const char *pgrep_cmd[] = { "pgrep", "-f", "betterlockscreen", NULL };
	const char *lock_cmd[] = {
		"betterlockscreen", "-l", "--display", "1",
		"--blur", "0.8", NULL
	};

	/* Validate DISPLAY before spawning X11-dependent betterlockscreen */
	display = getenv("DISPLAY");
	if (!display || !*display) {
		warn("DISPLAY not set");
		return;
	}

	/* Prevent duplicate betterlockscreen instances.
	 * Use -f (not -x) because "betterlockscreen" is 16 chars,
	 * exceeding Linux's 15-char comm field (TASK_COMM_LEN). */
	if (exec_wait(pgrep_cmd) == 0)
		return;

	exec_detach(lock_cmd);
}

static void
action_logout(char *const extra_argv[])
{
	const char *kill_slstatus[] = { "pkill", "-x", "slstatus", NULL };
	const char *kill_clipd[] = { "pkill", "-x", "dmenu-clipd", NULL };
	const char *kill_dunst[] = { "pkill", "-x", "dunst", NULL };
	const char *kill_dwm[] = { "pkill", "-x", "dwm", NULL };

	if (!confirm("logout?", extra_argv))
		return;

	/* Kill daemons in order: slstatus, dmenu-clipd, dunst, then dwm.
	 * dwm is killed LAST because it is the session leader --
	 * when dwm exits, the X session tears down. */
	exec_wait(kill_slstatus);
	exec_wait(kill_clipd);
	exec_wait(kill_dunst);
	exec_wait(kill_dwm);
}

static void
action_reboot(char *const extra_argv[])
{
	const char *reboot_cmd[] = { "systemctl", "reboot", NULL };

	if (!confirm("reboot?", extra_argv))
		return;

	/* exec_detach because systemctl reboot does not return */
	exec_detach(reboot_cmd);
}

static void
action_shutdown(char *const extra_argv[])
{
	const char *poweroff_cmd[] = { "systemctl", "poweroff", NULL };

	if (!confirm("shutdown?", extra_argv))
		return;

	/* exec_detach because systemctl poweroff does not return */
	exec_detach(poweroff_cmd);
}

int
main(int argc, char *argv[])
{
	DmenuCtx *ctx;
	char *sel;

	(void)argc;
	argv0 = argv[0];
	setup_sigchld();

	ctx = dmenu_open("session", 4, argv + 1);
	dmenu_write(ctx, "logout");
	dmenu_write(ctx, "lock");
	dmenu_write(ctx, "reboot");
	dmenu_write(ctx, "shutdown");
	sel = dmenu_read(ctx);
	dmenu_close(ctx);

	if (!sel)
		return 0;

	if (strcmp(sel, "lock") == 0)
		action_lock();
	else if (strcmp(sel, "logout") == 0)
		action_logout(argv + 1);
	else if (strcmp(sel, "reboot") == 0)
		action_reboot(argv + 1);
	else if (strcmp(sel, "shutdown") == 0)
		action_shutdown(argv + 1);

	free(sel);
	return 0;
}
