CFLAGS += -O3 -g

ifeq ($(PRIV),1)
CFLAGS += -DPRIV_IFLUSH -DPRIV_FLUSH
endif

run: test_sc
	./test_sc

test_sc: test_sc.c lib_arch_sc_loongarch64.h ../cacheutils.h
	$(CC) $(CFLAGS) test_sc.c -o test_sc

clean:
	rm test_sc
