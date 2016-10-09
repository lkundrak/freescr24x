/* Log and forward IO. */

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>

#include "iowrap.h"

static void
dump (const void *data, int sz, const char *tag)
{
	int i;
	unsigned char *buf = (unsigned char *)data;

	printf ("%s [%d]:", tag, sz);
	for (i = 0; i < sz; i++)
		printf (" %02x", buf[i]);
	printf ("\n");
}

ssize_t
fake_write (int fd, const void *buf, size_t count)
{
	dump (buf, count, "write");
	return real_write (fd, buf, count);
}

ssize_t
fake_read (int fd, void *buf, size_t count)
{
	int ret;

	ret = real_read (fd, buf, count);
	dump (buf, ret, "read");
	return ret;
}

int fake_ioctl (int fd, unsigned long request, char *argp)
{
	int ret;

	ret = real_ioctl (fd, request, argp);
	printf ("ioctl [%d]: %02d\n", request, ret);
	return ret;
}
