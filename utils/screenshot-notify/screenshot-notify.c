/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../common/util.h"

static int
capture_to_clipboard(void)
{
	int pipefd[2];
	pid_t maim_pid, xclip_pid;
	int status;

	if (pipe(pipefd) < 0)
		die("pipe:");

	/* Child 1: maim -s writes PNG to pipe */
	maim_pid = fork();
	if (maim_pid < 0)
		die("fork:");
	if (maim_pid == 0) {
		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[1]);
		execlp("maim", "maim", "-s", NULL);
		_exit(127);
	}

	/* Child 2: xclip reads PNG from pipe */
	xclip_pid = fork();
	if (xclip_pid < 0)
		die("fork:");
	if (xclip_pid == 0) {
		close(pipefd[1]);
		dup2(pipefd[0], STDIN_FILENO);
		close(pipefd[0]);
		execlp("xclip", "xclip", "-selection", "clipboard",
		       "-t", "image/png", NULL);
		_exit(127);
	}

	/* Parent: close both pipe ends to ensure xclip gets EOF */
	close(pipefd[0]);
	close(pipefd[1]);

	/* Wait for maim to get its exit code (cancel detection) */
	waitpid(maim_pid, &status, 0);

	/* Wait for xclip to finish */
	waitpid(xclip_pid, NULL, 0);

	return (WIFEXITED(status) && WEXITSTATUS(status) == 0) ? 0 : 1;
}

int
main(int argc, char *argv[])
{
	(void)argc;
	argv0 = argv[0];

	setup_sigchld();

	if (capture_to_clipboard() == 0) {
		const char *const notify[] = {
			"notify-send", "Screenshot",
			"Image copied to clipboard", NULL
		};
		exec_detach(notify);
	}

	return 0;
}
