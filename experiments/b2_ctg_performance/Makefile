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
ifeq ($(ITLB_PRIME),1)
CFLAGS += -DITLB_PRIME
endif

all: b2_ctg_performance

run: b2_ctg_performance
	./b2_ctg_performance

b2_ctg_performance: b2_ctg_performance.c
	$(CC) $(CFLAGS) -o b2_ctg_performance b2_ctg_performance.c

table:
	python table.py

table_tex:
	python table.py > /dev/null

b2_ctg_performance_table.pdf: table_tex table_standalone.tex
	@mkdir -p $(OUT_DIR)
	$(LATEX) $(LATEXFLAGS) -output-directory $(OUT_DIR) -jobname $(basename $@) table_standalone.tex > $(OUT_DIR)/$(basename $@).build.log 2>&1 || (echo "LaTeX build failed; see $(OUT_DIR)/$(basename $@).build.log"; false)
	@cp "$(OUT_DIR)/$@" "$@"
	@echo "Built $@"

clean:
	rm -f b2_ctg_performance b2_ctg_performance_table.pdf table.tex
	rm -rf $(OUT_DIR)
