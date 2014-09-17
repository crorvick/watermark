
CFLAGS := -g -O0

watermark: main.o
	gcc -o $@ $^ -lcairo
