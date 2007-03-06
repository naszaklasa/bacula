rem 
rem  perform installation of bacula on Win32 systems
rem
c:
cd \
mkdir tmp
cd c:\bacula\bin
path=c:\bacula\bin
umount --remove-all-mounts
mount -f c:\ /
c:\bacula\bin\bacula-fd.exe /install -c c:\bacula\bin\bacula-fd.conf
