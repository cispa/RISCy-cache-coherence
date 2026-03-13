CFLAGS += -O3 -g
HOST_CC ?= gcc
HOST_CFLAGS ?= -O3 -g

ifeq ($(THEAD),1)
CFLAGS += -DTHEAD
endif
ifeq ($(RDCYCLE),1)
CFLAGS += -DRDCYCLE
endif

all: inconsistency

run: inconsistency
	./inconsistency

inconsistency: inconsistency.c
	$(CC) $(CFLAGS) -o inconsistency inconsistency.c -lm

inconsistency_x86: inconsistency_x86.c
	$(HOST_CC) $(HOST_CFLAGS) -o inconsistency_x86 inconsistency_x86.c

clean:
	rm -f inconsistency inconsistency_x86
