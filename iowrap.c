/* Hijack and overload read/write/ioctl */

#define _GNU_SOURCE 1

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>
#include <dlfcn.h>
#include <errno.h>
#include <string.h>
#include <poll.h>

#include "iowrap.h"

static const char device[] = "/dev/scr24x0";
static int dev_fd = -1;
void *pending;

/* Standard symbols */

__attribute__ ((visibility ("default")))
int
__xstat (int ver, const char *pathname, struct stat *buf)
{
	static int (*libc___xstat)(int, const char *, struct stat *);

	if (libc___xstat == NULL)
		libc___xstat = dlsym (RTLD_NEXT, "__xstat");
	if (libc___xstat == NULL) {
		errno = EFAULT;
		return -1;
	}

	if (ver == 1 && strcmp (pathname, device) == 0) {
		memset (buf, 0, sizeof (struct stat));
		return 0;
	}

	return libc___xstat (ver, pathname, buf);
}

__attribute__ ((visibility ("default")))
int
open (const char *filename, int flags, ...)
{
	static int (*libc_open)(const char *, int, mode_t);
	mode_t mode = 0;

	if (flags & O_CREAT) {
		va_list args;
		va_start (args, flags);
		mode = va_arg (args, int);
		va_end (args);
	}

	if (libc_open == NULL)
		libc_open = dlsym (RTLD_NEXT, "open");
	if (libc_open == NULL)
		libc_open = dlsym (RTLD_NEXT, "open64");
	if (libc_open == NULL) {
		errno = EFAULT;
		return -1;
	}

	if (strcmp (filename, device) == 0 && dev_fd == -1) {
		dev_fd = libc_open (filename, O_RDWR, mode);
		if (dev_fd == -1)
			dev_fd = libc_open ("/dev/null", flags, mode);
		return dev_fd;
	}

	return libc_open (filename, flags, mode);
}

__attribute__ ((visibility ("default")))
int
close (int fd)
{
	static int (*libc_close)(int);

	if (libc_close == NULL)
		libc_close = dlsym (RTLD_NEXT, "close");
	if (libc_close == NULL) {
		errno = EFAULT;
		return -1;
	}

	if (dev_fd != -1 && fd == dev_fd)
		dev_fd = -1;

	return libc_close (fd);
}

ssize_t
real_write (int fd, const void *buf, size_t count)
{
	static int (*libc_write)(int, const void *, size_t);

	if (libc_write == NULL)
		libc_write = dlsym (RTLD_NEXT, "write");
	if (libc_write == NULL) {
		errno = EFAULT;
		return -1;
	}

	return libc_write (fd, buf, count);
}

__attribute__ ((visibility ("default")))
ssize_t
write (int fd, const void *buf, size_t count)
{
	if (dev_fd != -1 && fd == dev_fd)
		return fake_write (fd, buf, count);
	return real_write (fd, buf, count);
}

ssize_t
real_read (int fd, void *buf, size_t count)
{
	static int (*libc_read)(int, void *, size_t);

	if (libc_read == NULL)
		libc_read = dlsym (RTLD_NEXT, "read");
	if (libc_read == NULL) {
		errno = EFAULT;
		return -1;
	}

	return libc_read (fd, buf, count);
}

__attribute__ ((visibility ("default")))
ssize_t
read (int fd, void *buf, size_t count)
{
	if (dev_fd != -1 && fd == dev_fd)
		return fake_read (fd, buf, count);
	return real_read (fd, buf, count);
}

__attribute__ ((visibility ("default")))
int
poll (struct pollfd *fds, nfds_t nfds, int timeout)
{
	static int (*libc_poll)(struct pollfd *, nfds_t, int);

	if (   dev_fd != -1 && nfds == 1 && fds[0].fd == dev_fd
	    && fds[0].events == POLLIN && pending) {
		fds[0].revents = POLLIN;
		return 1;
	}

	if (libc_poll == NULL)
		libc_poll = dlsym (RTLD_NEXT, "poll");
	if (libc_poll == NULL) {
		errno = EFAULT;
		return -1;
	}

	return libc_poll (fds, nfds, timeout);
}

__attribute__ ((visibility ("default")))
int
select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
	static int (*libc_select)(int, fd_set *, fd_set *, fd_set *, struct timeval *);

	if (dev_fd != -1 && readfds && FD_ISSET (dev_fd, readfds) && pending) {
		FD_ZERO (readfds);
		FD_SET (dev_fd, readfds);
		if (writefds)
			FD_ZERO (writefds);
		if (exceptfds)
			FD_ZERO (exceptfds);
		return 1;
	}

	if (libc_select == NULL)
		libc_select = dlsym (RTLD_NEXT, "select");
	if (libc_select == NULL) {
		errno = EFAULT;
		return -1;
	}

	return libc_select (nfds, readfds, writefds, exceptfds, timeout);
}

int
real_ioctl(int fd, unsigned long request, char *argp)
{
	static int (*libc_ioctl)(int, unsigned long, char *);

	if (libc_ioctl == NULL)
		libc_ioctl = dlsym (RTLD_NEXT, "ioctl");
	if (libc_ioctl == NULL) {
		errno = EFAULT;
		return -1;
	}

	return libc_ioctl (fd, request, argp);
}

__attribute__ ((visibility ("default")))
int
ioctl(int fd, unsigned long request, char *argp)
{
	if (dev_fd != -1 && fd == dev_fd)
		return fake_ioctl (fd, request, argp);
	return real_ioctl (fd, request, argp);
}

int
open64 (const char *, int, ...) __attribute__ ((alias ("open")));
