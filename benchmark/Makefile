MYHTML_DIR=../../myhtml
HTML2HTML_DIR=../../html2html_lib

CPPFLAGS="-I$(MYHTML_DIR)/include" "-I$(HTML2HTML_DIR)/include"
LDFLAGS="-L$(HTML2HTML_DIR)/target/release" "-L$(MYHTML_DIR)/lib" -lhtml2html_lib -lmyhtml -lm -lpthread -ldl

CFLAGS=-O2

SOURCES = main.c perf.c proc_stat.c work.c
OBJS = $(SOURCES:.c=.o)

benchmark: $(OBJS)
	gcc $(CFLAGS) -o benchmark $(OBJS) $(CPPFLAGS) $(LDFLAGS)

run: benchmark
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):"$(MYHTML_DIR)/lib" ./benchmark

.PHONY: run
