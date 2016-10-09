# Run old userspace with new kernel via translation and logging shim

LIBCCID_ifdLogLevel=255 \
LD_PRELOAD=$PWD/xlat24x.so:$PWD/iolog.so \
IFD=$PWD/../scr24x_v4.2.6_Release/scr24x_2.6.x_v4.2.6/proprietary/Release/libSCR24x.so.4.2.1 \
	pcscd -c $PWD/ifdshim.conf -f -d
