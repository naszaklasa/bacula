REM build release if cygwin make not available.

cd zlib
nmake /f win32\Makefile.msc
cd ..
cd pthreads
nmake VCE
cd ..
cd baculafd
nmake CFG="baculafd - Win32 Release" /f baculafd.mak
cd ..
makensis winbacula.nsi
