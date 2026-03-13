CFLAGS += -O3 -g

.PHONY: all run clean always

all: test_sc test_icache_leakage

run: test_sc
	./test_sc

test_sc: always
	$(CC) $(CFLAGS) -o test_sc test_sc.c

test_icache_leakage: always
	$(CC) $(CFLAGS) -o test_icache_leakage test_icache_leakage.c

always:

clean:
	rm -f test_sc test_icache_leakage
