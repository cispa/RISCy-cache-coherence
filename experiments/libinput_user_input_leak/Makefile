CFLAGS += -O3 -g

ifeq ($(FR),1)
CFLAGS += -DATTACK=FLUSHRELOAD
else
CFLAGS += -DATTACK=ARCHSC
endif
ifeq ($(DEBUG),1)
CFLAGS += -DDEBUG
endif

probe_addr: probe_addr.c

scan: scan.c

clean:
	rm -f probe_addr scan main curses prime
