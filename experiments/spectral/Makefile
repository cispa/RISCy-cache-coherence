CFLAGS += -O3 -g

ifeq ($(FR),1)
CFLAGS += -DATTACK=FLUSHRELOAD
else
CFLAGS += -DATTACK=ARCHSC
endif

ifeq ($(PRIV),1)
CFLAGS += -DPRIV_IFLUSH -DPRIV_FLUSH
endif

ifeq ($(THEAD),1)
CFLAGS += -DTHEAD
endif
ifeq ($(RDCYCLE),1)
CFLAGS += -DRDCYCLE
endif
ifeq ($(MFENCE_8),1)
CFLAGS += -DMFENCE_8
endif

all: poc

run: poc
	./poc

poc: main.c
	$(CC) $(CFLAGS) main.c -o poc

poc_oryon: main_oryon.c
	$(CC) $(CFLAGS) main_oryon.c -o poc_oryon

poc_loongson: main_loongson.c
	$(CC) $(CFLAGS) main_loongson.c -o poc_loongson

clean:
	rm -f poc poc_oryon poc_loongson

