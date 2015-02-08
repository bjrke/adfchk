@echo off
set binary=adfchk.exe
set gcc=gcc.exe
set x=xdms\

set gccstrmain=gcc -O2 -pedantic -Wall
set gccstradfchk=-o %binary% adffile.c adfchkerr.c adfchkerrcmpr.c adfmisc.c adfparams.c adfmenu.c adfmain.c adfmyunzip.c LiteUnzip.h
set gccstrxdms1=adfmyundms.c %x%crc_csum.c %x%u_rle.c %x%u_init.c %x%u_quick.c %x%u_medium.c %x%u_deep.c %x%u_heavy.c
set gccstrxdms2=%x%pfile.c %x%tables.c %x%maketbl.c %x%getbits.c

cd src
%gccstrmain% %gccstradfchk% %gccstrxdms1% %gccstrxdms2%

cd ..
move src\%binary% . >nul

echo.
echo Built %binary% successfully. Check build.log for details.
echo.