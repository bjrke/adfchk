@echo off 

set origDir=%cd%

REM ------------------------------------------------------------------
REM   USE THIS LINE IF YOU ARE USING ADFCHK FROM THE CURRENT DIR
REM   (COMMENT IT OUT [::] IF YOU HAVE ADFCHK IN A GLOBALLY KNOWN PATH)
REM ------------------------------------------------------------------
set fname="%origDir%\adfchk.exe"

REM -------------------------------------------------------------------
REM   ONLY (!) USE THIS LINE IF YOU ARE USING ADFCHK FROM A KNOWN PATH
REM -------------------------------------------------------------------
:: set fname=adfchk.exe

REM ====================================================
REM   DO NOT CHANGE ANYTHING BELOW THIS LINE!!
REM ====================================================

setlocal
set logDir=%origDir%\logs
set curDir="%cd%"
set totalCnt=0

if not exist "%logDir%" md "%logDir%"

if not exist %curDir% echo %~1 does not exist&goto :EOF
if not "%~1"=="" set curDir=%~1
set mask=*.ADF

call :PROCESS %curDir%

echo.
echo TOTAL NUMBER OF FILES CHECKED: %totalCnt%
echo.
goto :EOF

:PROCESS

pushd "%~1"
for /f "tokens=*" %%a in ('dir /b /ad 2^>NUL') do call :PROCESS "%%a"
set cnt=0
for /f "tokens=*" %%a in ('dir /b /a-d %mask% 2^>NUL') do (
 set /a cnt+=1
REM call subtwo with FULL pathname (because we need this for the logfile)
 call :subtwo "%%~fa"
)
popd
rem if /i %cnt% EQU 0 goto :EOF

echo Directory %~f1 files (%mask%): %cnt%
set /a totalCnt+=cnt
rem RETURN
goto :EOF

:subtwo
set /a fcnt=%fcnt%+1
set logfname=adfchk%fcnt%.log
REM use leading zeroes to ease sorting
if %fcnt% lss 10 (
 set logfname=adfchk0%fcnt%.log
)
%fname% -f %1 -l "%logdir%\%logfname%" -b
goto :EOF
