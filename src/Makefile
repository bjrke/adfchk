CC=gcc
CFLAGS=-O2
SUBDIRS=LiteUnzip xdms

all:
	@for i in $(SUBDIRS); do \
	echo "make all in $$i..."; \
	(cd $$i; $(MAKE)); done
	$(CC) $(CFLAGS) -o adfchk adffile.c adfchkerr.c adfchkerrcmpr.c adfmisc.c adfparams.c adfmenu.c adfmain.c adfmyunzip.c LiteUnzip.h -ldl

.PHONY:	clean
clean:	
	rm -f *.o *.gch adfchk
	@for i in $(SUBDIRS); do \
	echo "make all in $$i..."; \
	(cd $$i; $(MAKE) clean); done
