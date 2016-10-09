/* Trace IFD calls.
 * IFD=libSCR24x.so pcscd */

#include <sys/types.h>
#include <sys/stat.h>

#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <ifdhandler.h>

static void *
call (const char *symbol)
{
	static const char *lib;
	static void *lib_handle = NULL;
	void *sym;

	if (lib_handle == NULL) {
		lib = getenv ("IFD");
		if (!lib) {
			printf ("IFD unset\n");
			exit (1);
		}
		lib_handle = dlopen (lib, RTLD_NOW | RTLD_LOCAL);
	}
	if (lib_handle == NULL) {
		printf ("%s: %s\n", lib, dlerror ());
		exit (1);
	}
	
	sym = dlsym (lib_handle, symbol);
	if (sym == NULL) {
		printf ("%s: %s\n", symbol, dlerror ());
		exit (1);
	}

	return sym;
}

static void
dumpbuf (const char *name, PUCHAR buf, DWORD len)
{
	printf ("ifdshim:   %s:", name);
	while (len--)
		printf (" [%02x]", *buf++);
	printf ("\n");
}

static void
dumphdr (const char *name, PSCARD_IO_HEADER hdr)
{
	printf ("ifdshim:   %s: Protocol=%ld Length=%ld\n",
	         name, hdr->Protocol, hdr->Length);
}

static const char *
err (RESPONSECODE res)
{
	switch (res) {
	case IFD_SUCCESS: return "IFD_SUCCESS";
	case IFD_ERROR_TAG: return "IFD_ERROR_TAG";
	case IFD_ERROR_SET_FAILURE: return "IFD_ERROR_SET_FAILURE";
	case IFD_ERROR_VALUE_READ_ONLY: return "IFD_ERROR_VALUE_READ_ONLY";
	case IFD_ERROR_PTS_FAILURE: return "IFD_ERROR_PTS_FAILURE";
	case IFD_ERROR_NOT_SUPPORTED: return "IFD_ERROR_NOT_SUPPORTED";
	case IFD_PROTOCOL_NOT_SUPPORTED: return "IFD_PROTOCOL_NOT_SUPPORTED";
	case IFD_ERROR_POWER_ACTION: return "IFD_ERROR_POWER_ACTION";
	case IFD_ERROR_SWALLOW: return "IFD_ERROR_SWALLOW";
	case IFD_ERROR_EJECT: return "IFD_ERROR_EJECT";
	case IFD_ERROR_CONFISCATE: return "IFD_ERROR_CONFISCATE";
	case IFD_COMMUNICATION_ERROR: return "IFD_COMMUNICATION_ERROR";
	case IFD_RESPONSE_TIMEOUT: return "IFD_RESPONSE_TIMEOUT";
	case IFD_NOT_SUPPORTED: return "IFD_NOT_SUPPORTED";
	case IFD_ICC_PRESENT: return "IFD_ICC_PRESENT";
	case IFD_ICC_NOT_PRESENT: return "IFD_ICC_NOT_PRESENT";
	case IFD_NO_SUCH_DEVICE: return "IFD_NO_SUCH_DEVICE";
	}

	printf ("[unknown error %ld]", res);
	return "[UNKNOWN_ERROR]";
}

RESPONSECODE
IFDHCreateChannel (DWORD Lun, DWORD Channel)
{
	RESPONSECODE res;

	printf ("ifdshim: IFDHCreateChannel called\n");
	res = ((RESPONSECODE (*)(DWORD, DWORD))
		call ("IFDHCreateChannel"))(Lun, Channel);
	printf ("ifdshim: IFDHCreateChannel returned [%ld] [%ld] -> %s\n", Lun, Channel, err (res));

	return res;
}

RESPONSECODE
IFDHGetCapabilities (DWORD Lun, DWORD Tag, PDWORD Length, PUCHAR Value)
{
	RESPONSECODE res;

	printf ("ifdshim: IFDHGetCapabilities called\n");
	res = ((RESPONSECODE (*)(DWORD, DWORD, PDWORD, PUCHAR))
		call ("IFDHGetCapabilities"))(Lun, Tag, Length, Value);
	printf ("ifdshim: IFDHGetCapabilities returned [%ld] [%ld] -> [%ld] [%p] %s\n",
		Lun, Tag, *Length, Value, err (res));

	return res;
}

RESPONSECODE
IFDHICCPresence (DWORD Lun)
{
	RESPONSECODE res;

	printf ("ifdshim: IFDHICCPresence called\n");
	res = ((RESPONSECODE (*)(DWORD))
		call ("IFDHICCPresence"))(Lun);
	printf ("ifdshim: IFDHICCPresence returned [%ld] -> %s\n",
		Lun, err (res));

	return res;
}

RESPONSECODE
IFDHPowerICC (DWORD Lun, DWORD Action, PUCHAR Atr, PDWORD AtrLength)
{
	RESPONSECODE res;

	printf ("ifdshim: IFDHPowerICC called\n");
	res = ((RESPONSECODE (*)(DWORD, DWORD, PUCHAR, PDWORD))
		call ("IFDHPowerICC"))(Lun, Action, Atr, AtrLength);
	printf ("ifdshim: IFDHPowerICC returned [%ld] [%ld] -> [%p] [%ld] %s\n",
		Lun, Action, Atr, *AtrLength, err (res));

	return res;
}

RESPONSECODE
IFDHSetProtocolParameters (DWORD Lun, DWORD Protocol, UCHAR Flags,
                           UCHAR PTS1, UCHAR PTS2, UCHAR PTS3)
{
	RESPONSECODE res;

	printf ("ifdshim: IFDHSetProtocolParameters called\n");
	res = ((RESPONSECODE (*)(DWORD, DWORD, UCHAR, UCHAR, UCHAR, UCHAR))
		call ("IFDHSetProtocolParameters"))(Lun, Protocol, Flags, PTS1, PTS2, PTS3);
	printf ("ifdshim: IFDHSetProtocolParameters returned [%ld] [%ld] [%02x] [%02x] [%02x] [%02x] -> %s\n",
		Lun, Protocol, Flags, PTS1, PTS2, PTS3, err (res));

	return res;
}

RESPONSECODE
IFDHTransmitToICC (DWORD Lun, SCARD_IO_HEADER SendPci, PUCHAR TxBuffer,
                   DWORD TxLength, PUCHAR RxBuffer, PDWORD RxLength,
                   PSCARD_IO_HEADER RecvPci)
{
	RESPONSECODE res;

	printf ("ifdshim: IFDHTransmitToICC called\n");
	res = ((RESPONSECODE (*)(DWORD, SCARD_IO_HEADER, PUCHAR, DWORD, PUCHAR, PDWORD, PSCARD_IO_HEADER))
		call ("IFDHTransmitToICC"))(Lun, SendPci, TxBuffer, TxLength, RxBuffer, RxLength, RecvPci);
	printf ("ifdshim: IFDHTransmitToICC returned [%ld] [%p] [%p] [%ld] [%p] -> [%ld] [%p] %s\n",
		Lun, &SendPci, TxBuffer, TxLength, RxBuffer, *RxLength, RecvPci, err (res));
	dumphdr ("SendPci", &SendPci);
	dumpbuf ("TxBuffer", TxBuffer, TxLength);
	dumpbuf ("RxBuffer", RxBuffer, *RxLength);
	dumphdr ("RecvPci", RecvPci);

	return res;
}

RESPONSECODE
IFDHCloseChannel (DWORD Lun)
{
	RESPONSECODE res;

	printf ("ifdshim: IFDHCloseChannel called\n");
	res = ((RESPONSECODE (*)(DWORD))
		call ("IFDHCloseChannel"))(Lun);
	printf ("ifdshim: IFDHCloseChannel returned [%ld] -> %s\n",
		Lun, err (res));

	return res;
}

RESPONSECODE
IFDHControl (DWORD Lun, DWORD dwControlCode, PUCHAR TxBuffer, DWORD TxLength,
             PUCHAR RxBuffer, DWORD RxLength, LPDWORD pdwBytesReturned)
{
	RESPONSECODE res;

	printf ("ifdshim: IFDHTransmitToICC called\n");
	res = ((RESPONSECODE (*)(DWORD, DWORD, PUCHAR, DWORD, PUCHAR, DWORD, LPDWORD))
		call ("IFDHControl"))(Lun, dwControlCode, TxBuffer, TxLength, RxBuffer, RxLength, pdwBytesReturned);
	printf ("ifdshim: IFDHTransmitToICC returned [%ld] [%ld] [%p] [%ld] [%p] -> [%ld] [%ld] %s\n",
		Lun, dwControlCode, TxBuffer, TxLength, RxBuffer, RxLength, *pdwBytesReturned, err (res));

	return res;
}

RESPONSECODE
IFDHSetCapabilities (DWORD Lun, DWORD Tag, DWORD Length, PUCHAR Value)
{
	RESPONSECODE res;

	printf ("ifdshim: IFDHSetCapabilities called\n");
	res = ((RESPONSECODE (*)(DWORD, DWORD, DWORD, PUCHAR))
		call ("IFDHSetCapabilities"))(Lun, Tag, Length, Value);
	printf ("ifdshim: IFDHSetCapabilities returned [%ld] [%ld] -> [%ld] [%p] %s\n",
		Lun, Tag, Length, Value, err (res));

	return res;
}
