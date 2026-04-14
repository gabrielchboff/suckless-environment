/* See LICENSE file for copyright and license details. */
#include <glob.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../common/util.h"
#include "config.h"

#define CAP_PATH     "/sys/class/power_supply/%s/capacity"
#define STAT_PATH    "/sys/class/power_supply/%s/status"
#define SYSFS_PATH   "/sys/class/power_supply/%s"

enum tier { TierNormal, TierLow, TierCritical, TierAC };

static int read_tier(const char *battery, int *cap, char *status, size_t slen);
static int find_battery(char *battery, size_t blen);
static enum tier calculate_tier(int cap, const char *status);
static void save_tier(enum tier t);
static int load_tier(void);
static void send_notification(enum tier t, int cap);

static int
read_tier(const char *battery, int *cap, char *status, size_t slen)
{
	char path[PATH_MAX];

	if (snprintf(path, sizeof(path), CAP_PATH, battery) < 0)
		return -1;
	if (pscanf(path, "%d", cap) != 1)
		return -1;

	if (snprintf(path, sizeof(path), STAT_PATH, battery) < 0)
		return -1;
	if (pscanf(path, "%15[a-zA-Z ]", status) != 1)
		return -1;

	(void)slen;
	return 0;
}

static int
find_battery(char *battery, size_t blen)
{
	glob_t gl;
	int ret = -1;

	/* glob for BAT* under power_supply */
	if (glob("/sys/class/power_supply/BAT*", GLOB_NOSORT, NULL, &gl) != 0)
		return -1;

	if (gl.gl_pathc == 0) {
		globfree(&gl);
		return -1;
	}

	/* Extract battery name from full path */
	const char *path = gl.gl_pathv[0];
	const char *name = strrchr(path, '/');
	if (name)
		name++;
	else
		name = path;

	ret = snprintf(battery, blen, "%s", name);
	globfree(&gl);

	return ret < 0 ? -1 : 0;
}

static enum tier
calculate_tier(int cap, const char *status)
{
	/* Charging or full → AC tier */
	if (strstr(status, "Charging") || strstr(status, "Full"))
		return TierAC;

	/* Critical tier: cap <= 5% */
	if (cap <= CRITICAL_THRESHOLD)
		return TierCritical;

	/* Low tier: cap <= 20% */
	if (cap <= LOW_THRESHOLD)
		return TierLow;

	return TierNormal;
}

static void
save_tier(enum tier t)
{
	FILE *fp = fopen(STATE_FILE, "w");
	if (fp) {
		fprintf(fp, "%d\n", t);
		fclose(fp);
	}
}

static int
load_tier(void)
{
	int tier = -1;
	FILE *fp = fopen(STATE_FILE, "r");
	if (fp) {
		if (fscanf(fp, "%d", &tier) != 1)
			tier = -1;
		fclose(fp);
	}
	return tier;
}

static void
send_notification(enum tier t, int cap)
{
	char body[64];
	const char *cmd[16];
	int i = 0;

	snprintf(body, sizeof(body), "Battery at %d%%", cap);

	if (t == TierLow) {
		cmd[i++] = "notify-send";
		cmd[i++] = "-u";
		cmd[i++] = "normal";
		cmd[i++] = "-r";
		cmd[i++] = "9001";
		cmd[i++] = "Battery Low";
	} else if (t == TierCritical) {
		cmd[i++] = "notify-send";
		cmd[i++] = "-u";
		cmd[i++] = "critical";
		cmd[i++] = "-r";
		cmd[i++] = "9002";
		cmd[i++] = "-p";
		cmd[i++] = "Battery Critical";
	} else {
		return;
	}
	cmd[i++] = body;
	cmd[i++] = NULL;

	exec_detach((const char *const *)cmd);
}

int
main(int argc, char *argv[])
{
	char battery[64];
	int cap;
	char status[16];
	enum tier current, previous;

	(void)argc;
	argv0 = argv[0];
	setup_sigchld();

	/* Auto-detect battery via glob */
	if (find_battery(battery, sizeof(battery)) < 0) {
		/* No battery found (desktop) - exit silently */
		return 0;
	}

	/* Read current state */
	if (read_tier(battery, &cap, status, sizeof(status)) < 0)
		return 1;

	current = calculate_tier(cap, status);
	previous = load_tier();

	/* State machine: fire only on TIER TRANSITION */
	if (current != previous) {
		int should_notify = 0;

		if (current == TierAC) {
			/* AC connected - re-arm for next discharge, notify user */
			save_tier(current);
			/* Could notify "charging" but not required by spec */
		} else if (previous == TierAC && current != TierAC) {
			/* Just disconnected from AC - normal notification behavior */
			save_tier(current);
			should_notify = 1;
		} else if (current == TierLow && previous != TierLow && previous != TierCritical) {
			/* Entering Low tier */
			save_tier(current);
			should_notify = 1;
		} else if (current == TierCritical && previous != TierCritical) {
			/* Entering Critical tier */
			save_tier(current);
			should_notify = 1;
		} else if (current == TierNormal) {
			/* Re-arming: Normal from Low after charging */
			if (previous == TierLow && cap >= REARM_THRESHOLD) {
				save_tier(current);
				should_notify = 0; /* No notification on re-arm */
			} else if (previous == TierCritical && cap >= LOW_THRESHOLD) {
				/* Re-arming from critical - require reaching LOW_THRESHOLD */
				save_tier(current);
				should_notify = 0;
			}
		}

		if (should_notify)
			send_notification(current, cap);
	}

	return 0;
}