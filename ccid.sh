# Run CCID driver with logging shim

LIBCCID_ifdLogLevel=255 \
LD_PRELOAD=$PWD/iolog.so \
IFD=$PWD/../src/CCID/src/.libs/libccidpcmcia.so \
	pcscd -c $PWD/ifdshim.conf -f -d
