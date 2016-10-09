FLAGS = -Wall

pcsc_CFLAGS = $(shell pkg-config --cflags-only-I libpcsclite)

ifdshim_CFLAGS += -fPIC
override ifdshim_CFLAGS += $(CFLAGS) $(pcsc_CFLAGS)
override ifdshim_LDFLAGS += -shared -ldl

free24x_CFLAGS += -fPIC
override free24x_CFLAGS += $(CFLAGS) $(pcsc_CFLAGS)
override free24x_LDFLAGS += -shared

iowrap_CFLAGS += -fPIC
override iowrap_CFLAGS += $(CFLAGS) -fvisibility=hidden
override iowrap_LDFLAGS += -shared -ldl

override testbench_CFLAGS += $(CFLAGS) $(pcsc_CFLAGS)
testbench_LDFLAGS = -Wl,-rpath=$(PWD) -L$(PWD)
override testbench_LDFLAGS += -lifdshim
#override testbench_LDFLAGS += -lfree24x

ALL = libifdshim.so libfree24x.so ccidemu.so xlat24x.so iolog.so testbench
all: $(ALL)

libifdshim.so: ifdshim.c
	$(CC) -o $@ $(ifdshim_CFLAGS) $(ifdshim_LDFLAGS) $<

libfree24x.so: free24x.c
	$(CC) -o $@ $(free24x_CFLAGS) $(free24x_LDFLAGS) $<

iowrap.o: iowrap.h iowrap.c
ccidemu.o: iowrap.h ccidemu.c
xlat24x.o: iowrap.h xlat24x.c
iolog.o: iowrap.h iolog.c

iowrap.o ccidemu.o xlat24x.o iolog.o:
	$(CC) -c -o $@ $(iowrap_CFLAGS) $(patsubst %.o,%.c,$@)

ccidemu.so: iowrap.o ccidemu.o
	$(CC) -o $@ $(iowrap_CFLAGS) $(iowrap_LDFLAGS) $^

xlat24x.so: iowrap.o xlat24x.o
	$(CC) -o $@ $(iowrap_CFLAGS) $(iowrap_LDFLAGS) $^

iolog.so: iowrap.o iolog.o
	$(CC) -o $@ $(iowrap_CFLAGS) $(iowrap_LDFLAGS) $^

testbench: libifdshim.so
testbench: testbench.c
	$(CC) -o $@ $(testbench_CFLAGS) $(testbench_LDFLAGS) $<

ifdshim.conf: ifdshim.conf.in
	sed "s#@PWD@#$(PWD)#" <$^ >$@

clean:
	rm -f $(ALL) *.o
