/* Emulate a "pcmcia_block" device. */

#include <stdint.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "iowrap.h"

struct ccid {
	uint8_t type;
	uint32_t len;
	uint8_t slot;
	uint8_t seq;
	uint8_t data[3];
	char body[0];
} __attribute__((packed));

static void
dump (void *data, int sz)
{
	int i;
	unsigned char *buf = (unsigned char *)data;

	for (i = 0; i < sz; i++)
		printf ("%02x ", buf[i]);
}

static void
dumpccid (struct ccid *data, const char *tag)
{
	printf ("ccidemu: %s: ", tag);
	dump (data, sizeof(struct ccid));
	printf ("| ");
	dump (data->body, le32toh (data->len));
	printf ("\n");
}

ssize_t
fake_write (int fd, const void *buf, size_t count)
{
	struct ccid *in = (struct ccid *)buf;

	if (count < sizeof(*in)) {
		errno = EINVAL;
		return -1;
	}

	if (count != sizeof(struct ccid) + le32toh (in->len)) {
		errno = EINVAL;
		return -1;
	}

	if (pending) {
		errno = ENOSPC;
		return -1;
	}

	pending = malloc (count);
	if (!pending) {
		errno = ENOMEM;
		return -1;
	}

	dumpccid (in, "host");

	memcpy (pending, buf, count);
	return count;
}

ssize_t
fake_read (int fd, void *buf, size_t count)
{
	struct ccid *in = (struct ccid *)pending;
	struct ccid *out = (struct ccid *)buf;
	int len;

	if (!pending) {
		errno = EINVAL;
		return -1;
	}

	if (count < sizeof(struct ccid)) {
		errno = ENOSPC;
		goto out;
	}

	len = count - sizeof(struct ccid);

	out->slot = in->slot;
	out->seq = in->seq;
	out->data[0] = out->data[1] = out->data[2] = 0;

	if (in->type == 0x65) {
		out->type = 0x81;
		out->data[0] = 0x02; // absent
		out->data[0] = 0x01; // present
		len = 0;
	} else if (in->type == 0x62) {
		// 62 00 00 00 00 00 04 00 00 00
		out->type = 0x80;
		if (len < 13) {
			errno = ENOSPC;
			goto out;
		}
		len = 13;
		memcpy (out->body, "\x3b\x7e\x94\x00\x00\x80\x25\xa0\x00\x00\x00\x28\x56", 13);
		//\x80\x10\x08\x00\x01\x14\d3
	} else if (in->type == 0x6c) {
		// 6c 00 00 00 00 00 6c 00 00 00
		// reset
		out->type = 0x82;
		if (len < 5) {
			errno = ENOSPC;
			goto out;
		}
		len = 5;
		memcpy (out->body, "\x11\x00\x00\x0a\x00", 5);
	} else if (in->type == 0x6f) {
		// \x6f\x05\x00\x00\x00\x00\x62\x00\x00\x00\x12\x34\x56\x07\x00
		// submit icc
		out->type = 0x80;
		if (len < 2) {
			errno = ENOSPC;
			goto out;
		}
		len = 2;
		memcpy (out->body, "\x6a\x83", 2);
	} else {
		out->type = 0xfb; // HW_ERROR;
		len = 0;
	}

	out->len = htole32 (len);

	dumpccid (out, "ccid");

	errno = 0;
out:
	free (pending);
	pending = NULL;

	if (errno)
		return -1;
	return sizeof(struct ccid) + len;
}

int fake_ioctl (int fd, unsigned long request, char *argp)
{
        errno = ENOSYS;
        return -1;
}
