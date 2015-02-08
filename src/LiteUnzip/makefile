CC=gcc
CFLAGS=-Wall -O -m32
LIBS=
OBJECTS=LiteUnzip.o
LDFLAGS=-g -m32
#LDFLAGS=-g

libliteunzip.so: $(OBJECTS)
	$(CC) $(LDFLAGS) -shared -Wl,-soname,libliteunzip.so -o $@ $(OBJECTS) $(LIBS)

LiteUnzip.o: LiteUnzip.c ../LiteUnzip.h
	$(CC) $(CFLAGS) -fPIC -c LiteUnzip.c

.PHONY: clean
clean:
	rm -f $(OBJECTS) libliteunzip.so
