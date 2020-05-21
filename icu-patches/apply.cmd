:: Copyright 2016 and later: Unicode, Inc. and others.
:: License & terms of use: http://www.unicode.org/copyright.html
@echo off

REM ----------------------------------------------------------------------
REM  This script is used to apply patches to ICU.
REM ----------------------------------------------------------------------

REM Don't add additional global environment variables, keep variables local to this script.
setlocal

REM Look for files under the "patches" folder that end in ".patch".
set FILES= 
for %%i in (patches\*.patch) do call :addfile "%%~i" 

echo.
echo Found the following Patch files (in the following order):
echo  %FILES%
echo.

REM Ask the user if they want to apply the patches.
choice /t 30 /c NY /d N /n /m "Do you want to proceed with applying the above patch files? [Y,N] "
echo.
if errorlevel 2 (
  echo Attempting to apply the patch files...
  echo.
REM  Notes on the 'git am' options:
REM   -3
REM         When the patch does not apply cleanly, fall back on 3-way merge.
REM   -i
REM         Run interactively.
REM   --keep-cr
REM         Prevent stripping CR at the end of lines. 
REM   --whitespace=nowarn 
REM         Turns off the trailing whitespace warning.
REM

git am -3 -i --keep-cr --whitespace=nowarn %FILES%

)

goto :eof 

:addfile 
  set FILES=%FILES% %1 
  goto :eof 
