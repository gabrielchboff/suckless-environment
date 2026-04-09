/* See LICENSE file for copyright and license details. */
#ifndef DMENU_H
#define DMENU_H

#include <stdio.h>
#include <unistd.h>

typedef struct {
	FILE *write;    /* Parent writes menu items here */
	FILE *read;     /* Parent reads selection from here */
	pid_t pid;      /* dmenu child PID */
} DmenuCtx;

/* Open dmenu with given prompt. lines > 0 enables vertical list (-l N).
 * Extra args forwarded to dmenu.
 * Returns handle for write/read operations, or NULL on failure. */
DmenuCtx *dmenu_open(const char *prompt, int lines, char *const extra_argv[]);

/* Write a single menu item (appends newline). */
void dmenu_write(DmenuCtx *ctx, const char *item);

/* Close write end (signals EOF to dmenu), read user selection.
 * Returns allocated string (caller frees) or NULL on cancel/Escape. */
char *dmenu_read(DmenuCtx *ctx);

/* Wait for dmenu to exit, free resources. */
void dmenu_close(DmenuCtx *ctx);

#endif /* DMENU_H */
