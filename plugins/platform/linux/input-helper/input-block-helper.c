/*
 * input-block-helper.c - setuid daemon for EVIOCGRAB-based input blocking
 *
 * Copyright (c) 2026 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * Runs as root (setuid), listens on a Unix domain socket, and processes
 * "block" / "unblock" commands from the Veyon Server.
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/input.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/run/veyon/input-block.sock"
#define BACKLOG 5

static volatile sig_atomic_t g_running = 1;
static int g_blocked = 0;
static int *g_fds = NULL;
static int g_fd_count = 0;

static void handle_signal(int sig)
{
	(void)sig;
	g_running = 0;
}

static void grab_devices(void)
{
	if (g_blocked)
		return;

	DIR *dir = opendir("/dev/input");
	if (dir == NULL)
		return;

	/* First pass: count event* entries */
	struct dirent *entry;
	int max_devices = 0;
	while ((entry = readdir(dir)) != NULL)
	{
		if (strncmp(entry->d_name, "event", 5) == 0)
			max_devices++;
	}

	if (max_devices == 0)
	{
		closedir(dir);
		return;
	}

	g_fds = (int *)calloc((size_t)max_devices, sizeof(int));
	if (g_fds == NULL)
	{
		closedir(dir);
		return;
	}

	rewinddir(dir);

	char path[PATH_MAX];
	int grabbed = 0;
	while ((entry = readdir(dir)) != NULL)
	{
		if (strncmp(entry->d_name, "event", 5) != 0)
			continue;

		snprintf(path, sizeof(path), "/dev/input/%s", entry->d_name);
		const int fd = open(path, O_RDWR | O_NONBLOCK);
		if (fd < 0)
			continue;

		if (ioctl(fd, EVIOCGRAB, (void *)1L) < 0)
		{
			close(fd);
			continue;
		}

		g_fds[grabbed++] = fd;
	}
	closedir(dir);

	g_fd_count = grabbed;
	g_blocked = (grabbed > 0);
}

static void release_devices(void)
{
	if (g_blocked == 0)
		return;

	for (int i = 0; i < g_fd_count; i++)
	{
		ioctl(g_fds[i], EVIOCGRAB, (void *)0L);
		close(g_fds[i]);
	}
	free(g_fds);
	g_fds = NULL;
	g_fd_count = 0;
	g_blocked = 0;
}

static int create_socket(void)
{
	const int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0)
	{
		perror("socket");
		return -1;
	}

	unlink(SOCKET_PATH);

	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		perror("bind");
		close(fd);
		return -1;
	}

	/* Allow any user to connect */
	chmod(SOCKET_PATH, 0666);

	if (listen(fd, BACKLOG) < 0)
	{
		perror("listen");
		close(fd);
		return -1;
	}

	return fd;
}

static void handle_client(int client_fd)
{
	char buf[16];
	memset(buf, 0, sizeof(buf));

	const ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
	if (n <= 0)
		return;

	/* Remove trailing newline */
	if (buf[n - 1] == '\n')
		buf[n - 1] = '\0';

	if (strcmp(buf, "block") == 0)
	{
		grab_devices();
		(void)!write(client_fd, g_blocked ? "ok\n" : "err\n", 3);
	}
	else if (strcmp(buf, "unblock") == 0)
	{
		release_devices();
		(void)!write(client_fd, "ok\n", 3);
	}
	else
	{
		(void)!write(client_fd, "err\n", 4);
	}
}

int main(void)
{
	/* Must be invoked with effective UID root (setuid) */
	if (geteuid() != 0)
	{
		fprintf(stderr, "input-block-helper: not running as root (euid=%d)\n",
			geteuid());
		return 1;
	}

	/* Ignore SIGPIPE */
	signal(SIGPIPE, SIG_IGN);

	/* Handle SIGTERM/SIGINT for clean shutdown */
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handle_signal;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);

	/* Ensure /run/veyon exists */
	mkdir("/run/veyon", 0755);

	const int server_fd = create_socket();
	if (server_fd < 0)
		return 1;

	while (g_running)
	{
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(server_fd, &fds);

		struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
		const int ret = select(server_fd + 1, &fds, NULL, NULL, &tv);
		if (ret < 0)
		{
			if (errno == EINTR)
				continue;
			break;
		}

		if (ret > 0 && FD_ISSET(server_fd, &fds))
		{
			const int client_fd = accept(server_fd, NULL, NULL);
			if (client_fd >= 0)
			{
				handle_client(client_fd);
				close(client_fd);
			}
		}
	}

	release_devices();
	close(server_fd);
	unlink(SOCKET_PATH);

	return 0;
}
