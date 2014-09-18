all: watermark

watermark: main.o
	gcc -o $@ $^ -lcairo

clean:
	-rm watermark
	-rm main.o

.PHONY: all clean
