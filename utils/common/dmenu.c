/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "dmenu.h"
#include "util.h"

#define DMENU_BUFSIZ 1024

DmenuCtx *
dmenu_open(const char *prompt, int lines, char *const extra_argv[])
{
	int to_dmenu[2];
	int from_dmenu[2];
	int extra_count;
	int base;
	char **argv;
	char linesbuf[12];
	int i;
	pid_t pid;
	DmenuCtx *ctx;

	/* Count extra argv entries */
	extra_count = 0;
	if (extra_argv)
		while (extra_argv[extra_count])
			extra_count++;

	/* Build argv: "dmenu", "-p", prompt, ["-l", N], [extras...], NULL */
	base = 3 + (lines > 0 ? 2 : 0);
	argv = malloc((base + extra_count + 1) * sizeof(char *));
	if (!argv)
		die("malloc:");
	argv[0] = "dmenu";
	argv[1] = "-p";
	argv[2] = (char *)prompt;
	if (lines > 0) {
		snprintf(linesbuf, sizeof(linesbuf), "%d", lines);
		argv[3] = "-l";
		argv[4] = linesbuf;
	}
	for (i = 0; i < extra_count; i++)
		argv[base + i] = extra_argv[i];
	argv[base + extra_count] = NULL;

	/* Create two pipes for bidirectional communication */
	if (pipe(to_dmenu) < 0)
		die("pipe:");
	if (pipe(from_dmenu) < 0) {
		close(to_dmenu[0]);
		close(to_dmenu[1]);
		die("pipe:");
	}

	pid = fork();
	if (pid < 0)
		die("fork:");

	if (pid == 0) {
		/* Child: redirect stdin/stdout through pipes */
		close(to_dmenu[1]);
		close(from_dmenu[0]);
		dup2(to_dmenu[0], STDIN_FILENO);
		dup2(from_dmenu[1], STDOUT_FILENO);
		close(to_dmenu[0]);
		close(from_dmenu[1]);
		execvp("dmenu", argv);
		_exit(127);
	}

	/* Parent: close unused pipe ends */
	close(to_dmenu[0]);
	close(from_dmenu[1]);

	ctx = malloc(sizeof(DmenuCtx));
	if (!ctx)
		die("malloc:");
	ctx->write = fdopen(to_dmenu[1], "w");
	if (!ctx->write) {
		close(to_dmenu[1]);
		close(from_dmenu[0]);
		free(ctx);
		die("fdopen:");
	}
	ctx->read = fdopen(from_dmenu[0], "r");
	if (!ctx->read) {
		fclose(ctx->write);
		close(from_dmenu[0]);
		free(ctx);
		die("fdopen:");
	}
	ctx->pid = pid;

	free(argv);
	return ctx;
}

void
dmenu_write(DmenuCtx *ctx, const char *item)
{
	fprintf(ctx->write, "%s\n", item);
	fflush(ctx->write);
}

char *
dmenu_read(DmenuCtx *ctx)
{
	char *buf;

	/* Close write end first to signal EOF to dmenu */
	if (ctx->write) {
		fclose(ctx->write);
		ctx->write = NULL;
	}

	buf = malloc(DMENU_BUFSIZ);
	if (!buf)
		die("malloc:");

	if (!fgets(buf, DMENU_BUFSIZ, ctx->read)) {
		free(buf);
		return NULL;
	}

	/* Strip trailing newline */
	buf[strcspn(buf, "\n")] = '\0';

	/* Empty string after stripping means no selection */
	if (buf[0] == '\0') {
		free(buf);
		return NULL;
	}

	return buf;
}

void
dmenu_close(DmenuCtx *ctx)
{
	if (ctx->write)
		fclose(ctx->write);
	if (ctx->read)
		fclose(ctx->read);
	waitpid(ctx->pid, NULL, 0);
	free(ctx);
}
