/* A test bench.
 * LD_PRELOAD=$PWD/xlat24x.so:$PWD/ccidemu.so ./testbench */

#include <string.h>
#include <stdio.h>
#include <ifdhandler.h>

UCHAR buf[2048];
DWORD len;

static void clb ()
{
	len = sizeof (buf);
	memset (buf, 0, len);
}

SCARD_IO_HEADER req_hdr, res_hdr;;
UCHAR req[2048], res[2048];
DWORD req_len, res_len;

#define REQ(p) \
	do { \
        	memset (res, 0, sizeof(res)); \
		res_len = sizeof(res); \
        	memset (req, 0, sizeof(req)); \
		req_len = sizeof((p)) - 1; \
        	memcpy (req, (p), req_len); \
	} while (0);

int
main ()
{
	IFDHCreateChannel (0, 0);

	clb ();
	IFDHGetCapabilities (0, TAG_IFD_ATR, &len, buf);
	clb ();
	IFDHGetCapabilities (0, TAG_IFD_SLOTNUM, &len, buf);
	clb ();
	IFDHGetCapabilities (0, TAG_IFD_SIMULTANEOUS_ACCESS, &len, buf);
	clb ();
	IFDHGetCapabilities (0, TAG_IFD_THREAD_SAFE, &len, buf);
	clb ();
	IFDHGetCapabilities (0, TAG_IFD_SLOTS_NUMBER, &len, buf);
	clb ();
	IFDHGetCapabilities (0, TAG_IFD_SLOT_THREAD_SAFE, &len, buf);
	clb ();
	IFDHGetCapabilities (0, TAG_IFD_POLLING_THREAD, &len, buf);

	IFDHICCPresence (0);

	clb ();
	IFDHPowerICC (0, IFD_POWER_UP, buf, &len);
	clb ();
	IFDHPowerICC (0, IFD_POWER_DOWN, buf, &len);
	clb ();
	IFDHPowerICC (0, IFD_RESET, buf, &len);

	IFDHSetProtocolParameters (0, SCARD_PROTOCOL_T0, 0, 0, 0, 0);
	IFDHSetProtocolParameters (0, SCARD_PROTOCOL_T0, IFD_NEGOTIATE_PTS1, 0, 0, 0);
	IFDHSetProtocolParameters (0, SCARD_PROTOCOL_T0, IFD_NEGOTIATE_PTS2, 0, 0, 0);
	IFDHSetProtocolParameters (0, SCARD_PROTOCOL_T0, IFD_NEGOTIATE_PTS3, 0, 0, 0);
	IFDHSetProtocolParameters (0, SCARD_PROTOCOL_T0,
	                           IFD_NEGOTIATE_PTS1 | IFD_NEGOTIATE_PTS2 | IFD_NEGOTIATE_PTS3,
	                           0x10, 0x20, 0x30);
	IFDHSetProtocolParameters (0, SCARD_PROTOCOL_T1, 0, 0, 0, 0);
	IFDHSetProtocolParameters (0, SCARD_PROTOCOL_T1,
	                           IFD_NEGOTIATE_PTS1 | IFD_NEGOTIATE_PTS2 | IFD_NEGOTIATE_PTS3,
	                           0x10, 0x20, 0x30);

	req_hdr.Protocol = 0;
	req_hdr.Length = 0;
	res_hdr.Protocol = 0;
	res_hdr.Length = 0;

	REQ("\x66\x66\x66\x66");
	IFDHTransmitToICC (0, req_hdr, req, req_len, res, &res_len, &res_hdr);

	req_hdr.Protocol = 1;
	REQ("\x66\x66\x66\x66");
	IFDHTransmitToICC (0, req_hdr, req, req_len, res, &res_len, &res_hdr);

	/*{
		REQ("");
		char b[] = "\x01\x02\x01\x02\x03\x01\x02\x03\x04";
		IFDHControl (0, (DWORD)b, req, req_len, res, sizeof(res), &res_len);
	}*/

	IFDHSetCapabilities (0, 0, 0, 0);

	IFDHCloseChannel (0);

	return 0;
}

