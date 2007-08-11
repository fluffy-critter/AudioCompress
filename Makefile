CC	= gcc
CFLAGS	= -g -Wall -Werror
LDFLAGS =
TARGETS	= AudioCompress

### Uncomment these lines if you want to include EsounD functionality
#CFLAGS 	+= -DUSE_ESD `esd-config --cflags`
#LDFLAGS += `esd-config --libs`

all:	$(TARGETS)

clean:
	rm -f $(TARGETS) *.o *.so

AudioCompress: AudioCompress.o compress.o
	$(CC) $(LDFLAGS) -o $(@) $(^)

.c.o:
	$(CC) $(CFLAGS) -c $*.c

dep:
	$(CXX) $(CFLAGS) -M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
