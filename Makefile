
CFLAGS := -g -O0

wm: wm.o
	gcc -o $@ $^ -lcairo
