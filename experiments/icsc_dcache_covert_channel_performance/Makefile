CFLAGS += -O3 -g
LATEX ?= pdflatex
LATEXFLAGS ?= -interaction=nonstopmode -halt-on-error
OUT_DIR ?= out

ifeq ($(DEBUG),1)
CFLAGS += -DDEBUG
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

all: icsc_dcache_covert_channel_performance

run: icsc_dcache_covert_channel_performance
	./icsc_dcache_covert_channel_performance

icsc_dcache_covert_channel_performance: icsc_dcache_covert_channel_performance.c
	$(CC) $(CFLAGS) -o icsc_dcache_covert_channel_performance icsc_dcache_covert_channel_performance.c -lm

table:
	python table.py

table_vs_fr:
	python table.py --compare-fr

table_tex:
	python table.py > /dev/null

table_vs_fr_tex:
	python table.py --compare-fr > /dev/null

icsc_dcache_covert_channel_performance_table.pdf: table_tex table_standalone.tex
	@mkdir -p $(OUT_DIR)
	$(LATEX) $(LATEXFLAGS) -output-directory $(OUT_DIR) -jobname $(basename $@) table_standalone.tex > $(OUT_DIR)/$(basename $@).build.log 2>&1 || (echo "LaTeX build failed; see $(OUT_DIR)/$(basename $@).build.log"; false)
	@cp "$(OUT_DIR)/$@" "$@"
	@echo "Built $@"

icsc_dcache_covert_channel_performance_vs_flush_reload_table.pdf: table_vs_fr_tex table_standalone.tex
	@mkdir -p $(OUT_DIR)
	$(LATEX) $(LATEXFLAGS) -output-directory $(OUT_DIR) -jobname $(basename $@) table_standalone.tex > $(OUT_DIR)/$(basename $@).build.log 2>&1 || (echo "LaTeX build failed; see $(OUT_DIR)/$(basename $@).build.log"; false)
	@cp "$(OUT_DIR)/$@" "$@"
	@echo "Built $@"

clean:
	rm -f icsc_dcache_covert_channel_performance icsc_dcache_covert_channel_performance_table.pdf icsc_dcache_covert_channel_performance_vs_flush_reload_table.pdf table.tex
	rm -rf $(OUT_DIR)
