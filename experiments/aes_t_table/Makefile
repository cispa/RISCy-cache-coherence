CFLAGS += -O3 -g
LATEX ?= pdflatex
LATEXFLAGS ?= -interaction=nonstopmode -halt-on-error
OUT_DIR ?= out

ifeq ($(FR),1)
CFLAGS += -DATTACK=FLUSHRELOAD
else
CFLAGS += -DATTACK=ARCHSC
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
ifeq ($(MFENCE_64),1)
CFLAGS += -DMFENCE_64
endif

all: aes_t_table

run: all
	./aes_t_table

openssl_arch_dir = openssl_$(ARCH)

clean:
	rm -rf aes_t_table
	make -C $(openssl_arch_dir) clean

$(openssl_arch_dir):
	if [ ! -d $(openssl_arch_dir) ]; then \
		git clone --depth=1 --branch openssl-3.5.2 https://github.com/openssl/openssl.git $(openssl_arch_dir) && \
		sed -i 's/static const u32 Te0\[256\].*/#define Te0 (Te0_real+1)\nconst u32 Te0_real[512] __attribute__((aligned(4096))) = { 0,/g' $(openssl_arch_dir)/crypto/aes/aes_core.c && \
		sed -i 's/static const u32 Te1\[256\].*/#define Te1 (Te1_real+1)\nconst u32 Te1_real[512] __attribute__((aligned(4096))) = { 0,/g' $(openssl_arch_dir)/crypto/aes/aes_core.c && \
		sed -i 's/static const u32 Te2\[256\].*/#define Te2 (Te2_real+1)\nconst u32 Te2_real[512] __attribute__((aligned(4096))) = { 0,/g' $(openssl_arch_dir)/crypto/aes/aes_core.c && \
		sed -i 's/static const u32 Te3\[256\].*/#define Te3 (Te3_real+1)\nconst u32 Te3_real[512] __attribute__((aligned(4096))) = { 0,/g' $(openssl_arch_dir)/crypto/aes/aes_core.c && \
		$(MAKE) openssl_setup; \
	fi

openssl_setup:
ifeq ($(ARCH),riscv64)
	cd $(openssl_arch_dir) && ./config linux64-riscv64 -static no-hw no-aria no-bf no-blake2 no-camellia no-cast no-chacha no-cmac no-des no-dh no-dsa no-ecdh no-ecdsa no-idea no-md4 no-mdc2 no-ocb no-poly1305 no-rc2 no-rc4 no-rmd160 no-scrypt no-seed no-siphash no-siv no-sm2 no-sm3 no-sm4 no-whirlpool no-asm no-ssl no-tls no-dtls no-ct no-ec no-ec2m no-fips no-gost no-ocsp no-rfc3779 no-sock no-srp no-tests no-ui no-apps -g3
else ifeq ($(ARCH),aarch64)
	cd $(openssl_arch_dir) && ./config linux-aarch64 -static no-hw no-aria no-bf no-blake2 no-camellia no-cast no-chacha no-cmac no-des no-dh no-dsa no-ecdh no-ecdsa no-idea no-md4 no-mdc2 no-ocb no-poly1305 no-rc2 no-rc4 no-rmd160 no-scrypt no-seed no-siphash no-siv no-sm2 no-sm3 no-sm4 no-whirlpool no-asm no-ssl no-tls no-dtls no-ct no-ec no-ec2m no-fips no-gost no-ocsp no-rfc3779 no-sock no-srp no-tests no-ui no-apps -g3
else ifeq ($(ARCH),loongarch64)
	cd $(openssl_arch_dir) && ./config linux64-loongarch64 -static no-hw no-aria no-bf no-blake2 no-camellia no-cast no-chacha no-cmac no-des no-dh no-dsa no-ecdh no-ecdsa no-idea no-md4 no-mdc2 no-ocb no-poly1305 no-rc2 no-rc4 no-rmd160 no-scrypt no-seed no-siphash no-siv no-sm2 no-sm3 no-sm4 no-whirlpool no-asm no-ssl no-tls no-dtls no-ct no-ec no-ec2m no-fips no-gost no-ocsp no-rfc3779 no-sock no-srp no-tests no-ui no-apps -g3
else
	$(error Unsupported ARCH: $(ARCH))
endif

$(openssl_arch_dir)/libcrypto.a: $(openssl_arch_dir) $(openssl_arch_dir)/crypto/aes/aes_core.c
	make -C $(openssl_arch_dir) -j $(nproc)

always:

# NOTE: we always want to build, as we cannot specify -B, as otherwise everything is rebuilt
# its fast, so no issue
aes_t_table: aes_t_table.c $(openssl_arch_dir)/libcrypto.a always
	$(CC) -static $(CFLAGS) -o $@ aes_t_table.c -I$(openssl_arch_dir)/include -lcrypto -L$(openssl_arch_dir) -Wl,-rpath=openssl

table:
	python table.py > /dev/null

aes_t_table_table.pdf: table table_standalone.tex
	@mkdir -p $(OUT_DIR)
	$(LATEX) $(LATEXFLAGS) -output-directory $(OUT_DIR) -jobname $(basename $@) table_standalone.tex > $(OUT_DIR)/$(basename $@).build.log 2>&1 || (echo "LaTeX build failed; see $(OUT_DIR)/$(basename $@).build.log"; false)
	@cp "$(OUT_DIR)/$@" "$@"
	@echo "Built $@"

t_table_cache_access_patterns.pdf: t_table_cache_access_patterns_standalone.tex
	@mkdir -p $(OUT_DIR)
	$(LATEX) $(LATEXFLAGS) -output-directory $(OUT_DIR) -jobname $(basename $@) t_table_cache_access_patterns_standalone.tex > $(OUT_DIR)/$(basename $@).build.log 2>&1 || (echo "LaTeX build failed; see $(OUT_DIR)/$(basename $@).build.log"; false)
	@cp "$(OUT_DIR)/$@" "$@"
	@echo "Built $@"
