CFLAGS += -O3 -g
LATEX ?= pdflatex
LATEXFLAGS ?= -interaction=nonstopmode -halt-on-error
OUT_DIR ?= out

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

all: b1_icache_oracle_performance

run: b1_icache_oracle_performance
	./b1_icache_oracle_performance

b1_icache_oracle_performance: b1_icache_oracle_performance.c
	$(CC) $(CFLAGS) -o b1_icache_oracle_performance b1_icache_oracle_performance.c

table:
	python table.py

table_tex:
	python table.py > /dev/null

b1_icache_oracle_performance_table.pdf: table_tex table_standalone.tex
	@mkdir -p $(OUT_DIR)
	$(LATEX) $(LATEXFLAGS) -output-directory $(OUT_DIR) -jobname $(basename $@) table_standalone.tex > $(OUT_DIR)/$(basename $@).build.log 2>&1 || (echo "LaTeX build failed; see $(OUT_DIR)/$(basename $@).build.log"; false)
	@cp "$(OUT_DIR)/$@" "$@"
	@echo "Built $@"

clean:
	rm -f b1_icache_oracle_performance b1_icache_oracle_performance_table.pdf table.tex
	rm -rf $(OUT_DIR)
