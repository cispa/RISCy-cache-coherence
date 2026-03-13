CFLAGS += -O3 -g

ifeq ($(THEAD),1)
CFLAGS += -DTHEAD
endif
ifeq ($(RDCYCLE),1)
CFLAGS += -DRDCYCLE
endif

all: b1_icache_oracle_prevalence

run: b1_icache_oracle_prevalence
	./b1_icache_oracle_prevalence

b1_icache_oracle_prevalence: b1_icache_oracle_prevalence.c
	$(CC) $(CFLAGS) -o b1_icache_oracle_prevalence b1_icache_oracle_prevalence.c -lm

plot:
	python plot.py

clean:
	rm -f b1_icache_oracle_prevalence
