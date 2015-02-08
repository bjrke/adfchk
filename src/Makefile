CC=gcc
CFLAGS=-O2 -m32
SUBDIRS=LiteUnzip

all:
	@for i in $(SUBDIRS); do \
	echo "make all in $$i..."; \
	(cd $$i; $(MAKE)); done
#	$(CC) $(CFLAGS) -o adfchk adffile.c adfchkerr.c adfchkerrcmpr.c adfmisc.c adfparams.c adfmenu.c adfmain.c adfmyunzip.c adfmyundms.c $(wildcard xmds/*.c) -ldl
	$(CC) $(CFLAGS) -o adfchk adfchkerr.c  adfchkerrcmpr.c  adffile.c  adfmain.c  adfmenu.c  adfmisc.c  adfmyundms.c  adfmyunzip.c  adfparams.c xdms/crc_csum.c  xdms/getbits.c  xdms/maketbl.c  xdms/pfile.c  xdms/tables.c  xdms/u_deep.c  xdms/u_heavy.c  xdms/u_init.c  xdms/u_medium.c  xdms/u_quick.c  xdms/u_rle.c -ldl

.PHONY:	clean
clean:	
	rm -f *.o *.gch adfchk
	@for i in $(SUBDIRS); do \
	echo "make all in $$i..."; \
	(cd $$i; $(MAKE) clean); done
