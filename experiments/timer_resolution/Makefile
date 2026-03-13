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

all: timer_resolution

run: timer_resolution
	./timer_resolution

timer_resolution: timer_resolution.c
	$(CC) $(CFLAGS) -o timer_resolution timer_resolution.c

table:
	python table.py > table.tex

timer_resolution_table.pdf: table table_standalone.tex
	@mkdir -p $(OUT_DIR)
	$(LATEX) $(LATEXFLAGS) -output-directory $(OUT_DIR) -jobname $(basename $@) table_standalone.tex > $(OUT_DIR)/$(basename $@).build.log 2>&1 || (echo "LaTeX build failed; see $(OUT_DIR)/$(basename $@).build.log"; false)
	@cp "$(OUT_DIR)/$@" "$@"
	@echo "Built $@"

clean:
	rm -f timer_resolution timer_resolution_table.pdf table.tex
	rm -rf $(OUT_DIR)
