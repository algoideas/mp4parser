C=gcc
CFLAGS=-I$(IDIR)

LIBS= -lm 

all:
	$(CC) -c -g -std=c99 libmp4.c -o libmp4.o
	$(CC) -c -g -std=c99 stream.c -o stream.o
	$(CC) -c -g -std=c99 mp4parser.c -o mp4parser.o
	$(CC) stream.o libmp4.o mp4parser.o -o mp4parser $(LIBS)
	rm -f *.o

clean:
	rm -f *.o
	rm -f mp4parser
