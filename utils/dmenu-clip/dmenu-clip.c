/* See LICENSE file for copyright and license details. */
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../common/dmenu.h"
#include "../common/util.h"
#include "config.h"

struct entry {
	char hash[NAME_MAX + 1];       /* Hash filename from cache dir */
	char preview[MAX_PREVIEW + 1]; /* Truncated preview text */
	char path[PATH_MAX];           /* Full path to cache file */
	time_t mtime;
};

static int
cmp_mtime_desc(const void *a, const void *b)
{
	const struct entry *ea = a;
	const struct entry *eb = b;

	if (ea->mtime > eb->mtime)
		return -1;
	if (ea->mtime < eb->mtime)
		return 1;
	return 0;
}

static void
build_preview(const char *path, char *preview, size_t prevsz)
{
	FILE *fp;
	size_t n, i;

	preview[0] = '\0';

	fp = fopen(path, "r");
	if (!fp)
		return;

	n = fread(preview, 1, prevsz - 1, fp);
	fclose(fp);
	preview[n] = '\0';

	/* Replace newlines and tabs with spaces */
	for (i = 0; i < n; i++) {
		if (preview[i] == '\n' || preview[i] == '\t')
			preview[i] = ' ';
	}
}

static void
restore_clipboard(const char *filepath)
{
	pid_t pid;
	int fd;

	pid = fork();
	if (pid < 0)
		die("fork:");

	if (pid == 0) {
		fd = open(filepath, O_RDONLY);
		if (fd < 0)
			_exit(1);
		dup2(fd, STDIN_FILENO);
		close(fd);
		execlp("xclip", "xclip", "-selection", "clipboard", NULL);
		_exit(127);
	}

	waitpid(pid, NULL, 0);
}

static int
get_cache_dir(char *buf, size_t bufsz)
{
	const char *cache, *home;
	int ret;

	cache = getenv("XDG_CACHE_HOME");
	if (cache && cache[0] != '\0') {
		ret = snprintf(buf, bufsz, "%s/%s", cache, CACHE_DIR_NAME);
	} else {
		home = getenv("HOME");
		if (!home)
			die("HOME not set");
		ret = snprintf(buf, bufsz, "%s/.cache/%s", home, CACHE_DIR_NAME);
	}

	if (ret < 0 || (size_t)ret >= bufsz)
		return -1;
	return 0;
}

int
main(int argc, char *argv[])
{
	char cache_dir[PATH_MAX];
	DIR *dp;
	struct dirent *de;
	struct stat st;
	struct entry entries[MAX_ENTRIES];
	int count, i, ret;
	DmenuCtx *ctx;
	char *sel;

	(void)argc;
	argv0 = argv[0];
	setup_sigchld();

	if (get_cache_dir(cache_dir, sizeof(cache_dir)) < 0)
		die("cache dir path too long");

	dp = opendir(cache_dir);
	if (!dp)
		return 0; /* No cache directory -- clean exit */

	count = 0;
	while ((de = readdir(dp)) != NULL && count < MAX_ENTRIES) {
		/* Skip . and .. */
		if (de->d_name[0] == '.')
			continue;

		ret = snprintf(entries[count].path, sizeof(entries[count].path),
		               "%s/%s", cache_dir, de->d_name);
		if (ret < 0 || (size_t)ret >= sizeof(entries[count].path))
			continue;

		/* Use lstat to detect symlinks (T-03-06 mitigation) */
		if (lstat(entries[count].path, &st) < 0)
			continue;
		if (S_ISLNK(st.st_mode))
			continue;
		if (!S_ISREG(st.st_mode))
			continue;

		snprintf(entries[count].hash, sizeof(entries[count].hash),
		         "%.*s", (int)(sizeof(entries[count].hash) - 1),
		         de->d_name);
		entries[count].mtime = st.st_mtime;
		build_preview(entries[count].path,
		              entries[count].preview,
		              sizeof(entries[count].preview));
		count++;
	}
	closedir(dp);

	if (count == 0)
		return 0; /* Empty cache -- clean exit */

	qsort(entries, count, sizeof(struct entry), cmp_mtime_desc);

	ctx = dmenu_open("clipboard", 10, argv + 1);
	if (!ctx)
		die("dmenu_open:");

	for (i = 0; i < count; i++)
		dmenu_write(ctx, entries[i].preview);

	sel = dmenu_read(ctx);
	dmenu_close(ctx);

	if (!sel)
		return 0;

	/* Match selection back to entry (newest-first handles duplicate previews) */
	for (i = 0; i < count; i++) {
		if (strcmp(sel, entries[i].preview) == 0) {
			restore_clipboard(entries[i].path);
			break;
		}
	}

	free(sel);
	return 0;
}
