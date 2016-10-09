/* Translate old ABI calls to new. */

#include <stdint.h>
#include <sys/types.h>
#include <errno.h>

#include <string.h>
#include <stdio.h>

#include "iowrap.h"

#define CMD_WRITE_DATA          13
#define CMD_READ_DATA           14
#define CMD_POLLED_MODE         15
#define CMD_GET_IRQSTATUS       81
#define CMD_SET_REVISION        82
#define CMD_BOOTROM_STATUS      83

struct data {
	unsigned char ucTxBuffer[2048];
	uint32_t dwTxLength;
	unsigned char ucRxBuffer[2048];
	uint32_t dwRxLength;
};

ssize_t
fake_write (int fd, const void *buf, size_t count)
{
	errno = ENOTSUP;
	return -1;
}

ssize_t
fake_read (int fd, void *buf, size_t count)
{
	struct data *packet = (struct data *)buf;
	char res[10];
	int rb;

#if 0
	/*cough*/
	if (count != 1) {
		errno = EINVAL;
		return -1;
	}
#endif

	rb = real_write (fd, "\x65\x00\x00\x00\x00\x00\x01\x00\x00\x00", 10);
	if (rb == -1)
		return -1;
	if (rb != 10) {
		errno = EIO;
		return -1;
	}

	rb = real_read (fd, res, sizeof(res));
	if (rb == -1)
		return -1;
	if (rb != sizeof (res)) {
		errno = EIO;
		return -1;
	}

	packet->ucRxBuffer[0] = (res[7] != 2);

	return sizeof(struct data);;
}

int fake_ioctl (int fd, unsigned long request, char *argp)
{
	struct data *packet = (struct data *)argp;
	static struct data out;

printf ("IOCTL %d\n", request);
	switch (request) {
	case CMD_WRITE_DATA:
		if (out.dwRxLength)
			memset (&out, 0, sizeof(out));

		/* READER_GETINFO */
		if (packet->dwTxLength == 11
		    && packet->ucTxBuffer[0] == '\x6b'
		    && packet->ucTxBuffer[10] == '\x00') {
			printf ("SKIP GI\n");
			out.dwRxLength = 23;
			memcpy (out.ucRxBuffer,
			        "\x83\x0d\x00\x00\x00\x00\x01\x00\x00\x00"
			        "\x02\x04\x07\x03\x00\x00\x00\x01\x00\x00\x00\x00\x01",
			        out.dwRxLength);
			out.ucRxBuffer[6] = packet->ucTxBuffer[6];
			return packet->dwTxLength;
		}

		/* READER_GETCHIPREV */
		if (packet->dwTxLength == 11
		    && packet->ucTxBuffer[0] == '\x6b'
		    && packet->ucTxBuffer[10] == '\x13') {
			printf ("SKIP GCR\n");
			out.dwRxLength = 11;
			memcpy (out.ucRxBuffer,
//			        "\x83\x01\x00\x00\x00\x00\x02\x01\x00\x00\x01",
			        "\x83\x01\x00\x00\x00\x00\x02\x00\x00\x00\x00",
			        out.dwRxLength);
			out.ucRxBuffer[6] = packet->ucTxBuffer[6];
			return packet->dwTxLength;
		}

#if 0
		/* READER_SETTRANSPORT */
		if (   packet->dwTxLength == 12
		    && packet->ucTxBuffer[0] == '\x6b'
		    && packet->ucTxBuffer[10] == '\x14'
		    && packet->ucTxBuffer[11] == '\x01') {
			printf ("SKIP ST\n");
			return packet->dwTxLength;
		}
#endif

		if (packet->dwTxLength != real_write (fd, packet->ucTxBuffer, packet->dwTxLength)) {
			errno = EIO;
			return -1;
		}

		return packet->dwTxLength;
	case CMD_READ_DATA:
		if (out.dwRxLength) {
			printf ("SKIPPED GI\n");
			memcpy (packet, &out, sizeof(out));
			memset (&out, 0, sizeof(out));
			return packet->dwRxLength;
		}
		packet->dwRxLength = real_read (fd, packet->ucRxBuffer, sizeof(packet->ucRxBuffer));
		if (packet->dwRxLength < 0) {
			packet->dwRxLength = 0;
			errno = EIO;
			return -1;
		}
		return packet->dwRxLength;
	case CMD_POLLED_MODE:
		return 0;
	case CMD_GET_IRQSTATUS:
		return 0;
	case CMD_SET_REVISION:
		return 0;
		// XXX!
		return real_ioctl (fd, request, argp);
	}

printf ("XIOCTL %d\n", request);
	errno = -ENOTSUP;
	return -1;
}
