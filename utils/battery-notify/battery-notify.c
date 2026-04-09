/* See LICENSE file for copyright and license details. */
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../common/util.h"
#include "config.h"

#define CAP_PATH  "/sys/class/power_supply/%s/capacity"
#define STAT_PATH "/sys/class/power_supply/%s/status"

static int
read_battery(int *cap, char *status, size_t slen)
{
	char path[PATH_MAX];

	if (snprintf(path, sizeof(path), CAP_PATH, BATTERY) < 0)
		return -1;
	if (pscanf(path, "%d", cap) != 1)
		return -1;

	if (snprintf(path, sizeof(path), STAT_PATH, BATTERY) < 0)
		return -1;
	if (pscanf(path, "%15[a-zA-Z ]", status) != 1)
		return -1;

	(void)slen;
	return 0;
}

int
main(int argc, char *argv[])
{
	int cap;
	char status[16];
	char body[32];
	FILE *fp;

	(void)argc;
	argv0 = argv[0];
	setup_sigchld();

	if (read_battery(&cap, status, sizeof(status)) < 0)
		die("failed to read battery status");

	if (cap <= THRESHOLD && strcmp(status, "Discharging") == 0) {
		if (access(STATE_FILE, F_OK) != 0) {
			snprintf(body, sizeof(body), "Battery at %d%%", cap);
			const char *cmd[] = {
				"notify-send", "-u", "critical",
				"Battery Low", body, NULL
			};
			exec_detach((const char *const *)cmd);

			fp = fopen(STATE_FILE, "w");
			if (fp)
				fclose(fp);
		}
	} else {
		unlink(STATE_FILE);
	}

	return 0;
}
