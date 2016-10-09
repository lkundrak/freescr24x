/* A reversed SCR24x driver. */

/* Compatible with old ABI. */
#define TERRIBLE 1

#include <sys/ioctl.h>

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <ifdhandler.h>

#define CMD_WRITE_DATA		13
#define CMD_READ_DATA		14
#define CMD_POLLED_MODE		15
#define CMD_GET_IRQSTATUS	81
#define CMD_SET_REVISION	82
#define CMD_BOOTROM_STATUS	83

struct data {	
	unsigned char ucTxBuffer[2048];
	uint32_t dwTxLength;
	unsigned char ucRxBuffer[2048];
	uint32_t dwRxLength;
};

static void
call (int fd, struct data *packet, int op, const char *name)
{
	errno = 0;
	ioctl (fd, op, packet);
	if (errno)
		perror (name);
}

/* Set packet contents */
#define SP(p,tx,rx) \
	do { \
		memset ((p), 0, sizeof(*(p))); \
		(p)->dwTxLength = sizeof((tx)) - 1; \
		memcpy ((p)->ucTxBuffer, (tx), sizeof((tx))); \
		(p)->dwRxLength = sizeof((rx)) - 1; \
		memcpy ((p)->ucRxBuffer, (rx), sizeof((rx))); \
	} while (0);

/* Submit packet */
#define PCALL(fd,p,op) \
	do { \
		call ((fd), (p), op, #op); \
	} while (0)

/* Set and submit contents */
#define CALL(fd,op,tx,rx) \
	do { \
		struct data packet; \
		SP (&packet, (tx), (rx)); \
		PCALL ((fd), &packet, op); \
	} while (0)


static void
do_write (int fd, struct data *packet)
{
#ifdef TERRIBLE
	PCALL (fd, packet, CMD_WRITE_DATA);
#else
	errno = 0;
	uint32_t written;

	written = write (fd, packet->ucTxBuffer, packet->dwTxLength);
	if (written == -1)
		perror ("read");
	else if (written != packet->dwTxLength)
		fprintf (stderr, "read: short read\n");
#endif
}

static void
do_read (int fd, struct data *packet)
{
#ifdef TERRIBLE
	PCALL (fd, packet, CMD_READ_DATA);
#else
	errno = 0;
	packet->dwRxLength = read (fd, packet->ucRxBuffer, sizeof(packet->ucRxBuffer));
	if (packet->dwRxLength == -1)
		perror ("read");
#endif
}

/* Write/read transaction of caller owned packet */
#define PTX(fd,p) \
	do { \
		do_write ((fd), (p)); \
		SP ((p), "", ""); \
		do_read ((fd), (p)); \
	} while (0)

/* Write/read transaction */
#define TX(fd,d) \
	do { \
		struct data packet; \
		SP (&packet, (d), ""); \
		PTX((fd),&packet); \
	} while (0)

/* Write only transaction */
#define WRITE(fd,d) \
	do { \
		struct data packet; \
		SP (&packet, (d), ""); \
		do_write ((fd), &packet); \
	} while (0)

static int fd = -1;
static int counter = 0;

RESPONSECODE
IFDHCreateChannel (DWORD Lun, DWORD Channel)
{
	if (fd != -1)
		return IFD_SUCCESS; // no appropriate error status

	fd = open ("/dev/SCR24x0", O_RDWR);
	if (fd == -1) {
		if (errno == ENOENT)
			return IFD_NO_SUCH_DEVICE;
		return IFD_COMMUNICATION_ERROR;
	}

	TX (fd, "\x6B\x01\x00\x00\x00\x00\x01\x00\x00\x00\x00");
//               esc `----len-------'slt seq `----ms----'
//                                                     `--getinfo
	TX (fd, "\x6B\x01\x00\x00\x00\x00\x02\x00\x00\x00\x13"); // READER_GETCHIPREV ?
	WRITE (fd, "\x6B\x02\x00\x00\x00\x00\x03\x00\x00\x00\x14\x01");  // READER_SETTRANSPORT
	CALL (fd, CMD_SET_REVISION, "", "\x01");
	CALL (fd, CMD_BOOTROM_STATUS, "", "");
	TX (fd, "\x6B\x01\x00\x00\x00\x00\x01\x00\x00\x00\x00");
	CALL (fd, CMD_GET_IRQSTATUS, "", "");
	TX (fd, "\x6B\x01\x00\x00\x00\x00\x02\x00\x00\x00\x13");
	CALL (fd, CMD_POLLED_MODE, "", "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00");
	WRITE (fd, "\x6B\x02\x00\x00\x00\x00\x03\x00\x00\x00\x14\x01");
	CALL (fd, CMD_SET_REVISION, "", "\x01");
	TX (fd, "\x6B\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00");

	return IFD_SUCCESS;
}

RESPONSECODE
IFDHGetCapabilities (DWORD Lun, DWORD Tag, PDWORD Length, PUCHAR Value)
{
	if (Tag == TAG_IFD_ATR) {
		*Length = 0;
		return IFD_SUCCESS;
	}

	return IFD_COMMUNICATION_ERROR;
}

RESPONSECODE
IFDHICCPresence (DWORD Lun)
{
	struct data packet;

#ifdef TERRIBLE
	/* The return value makes no sense, the kernel
	 * driver lies and does no bounds checking... */
	read(fd, &packet, sizeof (packet));

	return packet.ucRxBuffer[0] ? IFD_ICC_PRESENT : IFD_ICC_NOT_PRESENT;
#else
	SP (&packet, "\x65\x00\x00\x00\x00\x00\x01\x00\x00\x00", "");
	PTX (fd, &packet);
	if (packet.ucRxBuffer[7] == 2)
		return IFD_ICC_NOT_PRESENT;
	return IFD_ICC_PRESENT;
#endif
}

RESPONSECODE
IFDHPowerICC (DWORD Lun, DWORD Action, PUCHAR Atr, PDWORD AtrLength)
{
	struct data packet;
	int len;

	switch (Action) {
	case IFD_POWER_UP:
	case IFD_RESET:
		SP (&packet, "\x62\x00\x00\x00\x00\x00\x10\x00\x00\x00", "");
		PTX (fd, &packet);
		TX (fd, "\x6C\x00\x00\x00\x00\x00\x6C\x00\x00\x00");

		len = packet.dwRxLength - 10;
		memcpy (Atr, &packet.ucRxBuffer[10], *AtrLength < len ? *AtrLength : len);
		*AtrLength = len;

		return IFD_SUCCESS;
	case IFD_POWER_DOWN:
		TX (fd, "\x63\x00\x00\x00\x00\x00\x10\x00\x00\x00");
		return IFD_COMMUNICATION_ERROR;
	}

	return IFD_COMMUNICATION_ERROR;
}

RESPONSECODE
IFDHSetProtocolParameters (DWORD Lun, DWORD Protocol, UCHAR Flags,
                           UCHAR PTS1, UCHAR PTS2, UCHAR PTS3)
{
	if (Flags != 0)
		return IFD_COMMUNICATION_ERROR;

	/* The original driver checks if ATR matches protocol. That is not cool,
          * some cards seem to lie and PCSCD knows better anyway. */
	switch (Protocol) {
	case SCARD_PROTOCOL_T0:
		counter = 0;
		TX (fd, "\x61\x05\x00\x00\x00\x00\x61\x00\x00\x00\x94\x00\x00\x00\x00");
		return IFD_SUCCESS;
	case SCARD_PROTOCOL_T1:
		counter = 0;
		TX (fd, "\x61\x07\x00\x00\x00\x00\x61\x01\x00\x00\x18\x00\x00\x00\x00\x00\x00");
		return IFD_SUCCESS;
	}

	return IFD_COMMUNICATION_ERROR;
}

RESPONSECODE
IFDHTransmitToICC (DWORD Lun, SCARD_IO_HEADER SendPci, PUCHAR TxBuffer,
                   DWORD TxLength, PUCHAR RxBuffer, PDWORD RxLength,
                   PSCARD_IO_HEADER RecvPci)
{
	struct data packet;
	int len;

	if (SendPci.Protocol == 0) {

		SP (&packet, "\x6F\x00\x00\x00\x00\x00\x6F\x00\x01\x00", "");

		if (packet.dwTxLength + TxLength > sizeof(packet.ucTxBuffer))
			return IFD_COMMUNICATION_ERROR;
		memcpy(&packet.ucTxBuffer[packet.dwTxLength], TxBuffer, TxLength);
		packet.dwTxLength += TxLength;
		packet.ucTxBuffer[1] = TxLength;

		PTX (fd, &packet);

		len = packet.ucRxBuffer[1];
		memcpy (RxBuffer, &packet.ucRxBuffer[10], len > *RxLength ? *RxLength : len);
		*RxLength = len;

		return IFD_SUCCESS;
	} else if (SendPci.Protocol == 1) {
		UCHAR csum = 0x00;
		int i;

		if (counter == 0) {
			SP (&packet, "\x6f\x05\x00\x00\x00\x00\x6f\x00\x01\x00\x00\xc1\x01\xfc\x3c", "");
			PTX (fd, &packet);
		} else if (counter == 3) {
			counter = 1;
		}

		SP (&packet, "\x6F\x00\x00\x00\x00\x00\x6F\x00\x01\x00", "");

		if (packet.dwTxLength + TxLength + 4 > sizeof(packet.ucTxBuffer))
			return IFD_COMMUNICATION_ERROR;

		csum ^= (packet.ucTxBuffer[packet.dwTxLength++] = 0x00);
		csum ^= (packet.ucTxBuffer[packet.dwTxLength++] = counter++ % 2 ? 0x40 : 0x00);
		csum ^= (packet.ucTxBuffer[packet.dwTxLength++] = TxLength);

		for (i = 0; i < TxLength; i++)
			csum ^= (packet.ucTxBuffer[packet.dwTxLength++] = TxBuffer[i]);
		packet.ucTxBuffer[packet.dwTxLength++] = csum;

		packet.ucTxBuffer[1] = TxLength + 4;

		PTX (fd, &packet);

		csum = packet.ucRxBuffer[10];
		csum ^= packet.ucRxBuffer[11];
		csum ^= (len = packet.ucRxBuffer[12]);

		if (len != packet.ucRxBuffer[1] - 4)
			return IFD_COMMUNICATION_ERROR;

		for (i = 0; i < len; i++) {
			csum ^= packet.ucRxBuffer[13 + i];
			if (len <= *RxLength)
				RxBuffer[i] = packet.ucRxBuffer[12 + i];
		}
		if (csum != packet.ucRxBuffer[13 + len])
			return IFD_COMMUNICATION_ERROR;
		*RxLength = len;

		return IFD_SUCCESS;
	}

	return IFD_COMMUNICATION_ERROR;
}

RESPONSECODE
IFDHCloseChannel (DWORD Lun)
{
 	/* No appropriate error status exists. */
	if (fd == -1)
		return IFD_SUCCESS;

	close (fd);
	fd = -1;

	return IFD_SUCCESS;
}

RESPONSECODE
IFDHControl (DWORD Lun, DWORD dwControlCode, PUCHAR TxBuffer, DWORD TxLength,
             PUCHAR RxBuffer, DWORD RxLength, LPDWORD pdwBytesReturned)
{
	/* It is unknown what the original driver uses this for. */
	return IFD_COMMUNICATION_ERROR;
}

RESPONSECODE
IFDHSetCapabilities (DWORD Lun, DWORD Tag, DWORD Length, PUCHAR Value)
{
	return IFD_COMMUNICATION_ERROR;
}
