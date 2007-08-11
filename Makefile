CC	= gcc
CFLAGS	= -g -Wall -I/usr/X11R6/include
LDFLAGS =
TARGETS	= AudioCompress
INSTALL = install-cmdline

### Different platforms need different options...
### Linux
DYNAMIC	=  -shared -Wl,-soname -Wl,$(@)

### MacOS X (Jaguar with Fink)
#CFLAGS	= -I/sw/include
#LDFLAGS	= -L/sw/lib
#DYNAMIC	= -dynamiclib

### Comment out these lines if you don't have X11 installed or
### otherwise want to build it without X11
CFLAGS 	+= -DUSE_X
LDFLAGS += -L/usr/X11R6/lib -lX11

### Comment out these lines if you don't want to build the XMMS plugin
### (or don't have the XMMS development headers installed)
TARGETS += libcompress.so
INSTALL += install-xmms
CFLAGS	+= `xmms-config --cflags`
LDFLAGS	+= `xmms-config --libs`

### Comment out these lines if you don't want to include EsounD functionality
### (or don't have the EsounD development headers installed)
CFLAGS 	+= -DUSE_ESD `esd-config --cflags`
LDFLAGS += `esd-config --libs`

all:	$(TARGETS)

install: $(INSTALL)

install-xmms: libcompress.so
	install -m 755 libcompress.so `xmms-config --effect-plugin-dir`

install-cmdline: AudioCompress
	install -m 755 AudioCompress /usr/local/bin

clean:
	rm -f $(TARGETS) *.o *.so

libcompress.so: xmms-glue.o compress.o
	$(CC) $(LDFLAGS) ${DYNAMIC} -o $(@) $(^)

AudioCompress: AudioCompress.o compress.o
	$(CC) $(LDFLAGS) -o $(@) $(^)

xmms-glue.o: xmms-glue.c
	$(CC) $(CFLAGS) `xmms-config --cflags` -c $*.c

.c.o:
	$(CC) $(CFLAGS) -c $*.c

dep:
	$(CXX) $(CFLAGS) -M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
