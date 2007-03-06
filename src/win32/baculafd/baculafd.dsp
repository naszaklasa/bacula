# Microsoft Developer Studio Project File - Name="baculafd" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=baculafd - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "baculafd.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "baculafd.mak" CFG="baculafd - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "baculafd - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "baculafd - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "baculafd - Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../compat" /I "../.." /I "../../../../depkgs-win32/pthreads" /I "../../../../depkgs-win32/zlib" /I "." /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_WIN32" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 wsock32.lib pthreadVCE.lib zlib.lib /nologo /subsystem:windows /pdb:none /machine:I386 /nodefaultlib:"MSVCRT.lib" /out:"Release/bacula-fd.exe" /libpath:"../../../../depkgs-win32/pthreads" /libpath:"../../../../depkgs-win32/zlib"

!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../compat" /I "../.." /I "../../../../depkgs-win32/pthreads" /I "../../../../depkgs-win32/zlib" /I "." /D "_DEBUG" /D "_WINMAIN_" /D "PTW32_BUILD" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_WIN32" /FR /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 user32.lib advapi32.lib gdi32.lib wsock32.lib shell32.lib pthreadVCE.lib zlib.lib kernel32.lib comdlg32.lib /nologo /subsystem:windows /pdb:none /debug /machine:I386 /nodefaultlib:"MSVCRT.lib" /out:"Debug/bacula-fd.exe" /libpath:"../../../../depkgs-win32/pthreads" /libpath:"../../../../depkgs-win32/zlib"

!ENDIF 

# Begin Target

# Name "baculafd - Win32 Release"
# Name "baculafd - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\lib\alist.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\alloc.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\attr.cpp
# End Source File
# Begin Source File

SOURCE=..\findlib\attribs.cpp
# End Source File
# Begin Source File

SOURCE=..\filed\authenticate.cpp
# End Source File
# Begin Source File

SOURCE=..\filed\backup.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\base64.cpp
# End Source File
# Begin Source File

SOURCE=..\findlib\bfile.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\bget_msg.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\bnet.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\bnet_server.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\bshm.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\bsys.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\btime.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\btimers.cpp
# End Source File
# Begin Source File

SOURCE=..\compat\compat.cpp
# End Source File
# Begin Source File

SOURCE="..\lib\cram-md5.cpp"
# End Source File
# Begin Source File

SOURCE=..\lib\crc32.cpp
# End Source File
# Begin Source File

SOURCE=..\findlib\create_file.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\daemon.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\dlist.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\edit.cpp
# End Source File
# Begin Source File

SOURCE=..\findlib\enable_priv.cpp
# End Source File
# Begin Source File

SOURCE=..\filed\estimate.cpp
# End Source File
# Begin Source File

SOURCE=..\filed\filed.cpp
# End Source File
# Begin Source File

SOURCE=..\filed\filed_conf.cpp
# End Source File
# Begin Source File

SOURCE=..\findlib\find.cpp
# End Source File
# Begin Source File

SOURCE=..\findlib\find_one.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\fnmatch.cpp
# End Source File
# Begin Source File

SOURCE=..\compat\getopt.c
# End Source File
# Begin Source File

SOURCE=..\filed\heartbeat.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\hmac.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\htable.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\idcache.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\jcr.cpp
# End Source File
# Begin Source File

SOURCE=..\filed\job.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\lex.cpp
# End Source File
# Begin Source File

SOURCE=..\findlib\makepath.cpp
# End Source File
# Begin Source File

SOURCE=..\findlib\match.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\md5.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\mem_pool.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\message.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\parse_conf.cpp
# End Source File
# Begin Source File

SOURCE=..\compat\print.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\queue.cpp
# End Source File
# Begin Source File

SOURCE=..\filed\restore.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\rwlock.cpp
# End Source File
# Begin Source File

SOURCE="..\findlib\save-cwd.cpp"
# End Source File
# Begin Source File

SOURCE=..\lib\scan.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\semlock.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\serial.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\sha1.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\signal.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\smartall.cpp
# End Source File
# Begin Source File

SOURCE=..\filed\status.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\tree.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\util.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\var.cpp
# End Source File
# Begin Source File

SOURCE=..\filed\verify.cpp
# End Source File
# Begin Source File

SOURCE=..\filed\verify_vol.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\watchdog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\filed\win32\winabout.cpp
# End Source File
# Begin Source File

SOURCE=..\findlib\winapi.cpp
# End Source File
# Begin Source File

SOURCE=..\..\filed\win32\winevents.cpp
# End Source File
# Begin Source File

SOURCE=..\..\filed\win32\winmain.cpp
# End Source File
# Begin Source File

SOURCE=..\..\filed\win32\winres.rc
# End Source File
# Begin Source File

SOURCE=..\..\filed\win32\winservice.cpp
# End Source File
# Begin Source File

SOURCE=..\..\filed\win32\winstat.cpp
# End Source File
# Begin Source File

SOURCE=..\..\filed\win32\wintray.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\workq.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\compat\alloca.h
# End Source File
# Begin Source File

SOURCE=..\compat\compat.h
# End Source File
# Begin Source File

SOURCE=..\compat\config.h
# End Source File
# Begin Source File

SOURCE=..\compat\dirent.h
# End Source File
# Begin Source File

SOURCE=..\compat\getopt.h
# End Source File
# Begin Source File

SOURCE=..\compat\grp.h
# End Source File
# Begin Source File

SOURCE=..\compat\host.h
# End Source File
# Begin Source File

SOURCE=..\compat\mswinver.h
# End Source File
# Begin Source File

SOURCE=..\compat\netdb.h
# End Source File
# Begin Source File

SOURCE=..\compat\pwd.h
# End Source File
# Begin Source File

SOURCE=..\compat\sched.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=..\compat\stdint.h
# End Source File
# Begin Source File

SOURCE=..\compat\strings.h
# End Source File
# Begin Source File

SOURCE=..\compat\syslog.h
# End Source File
# Begin Source File

SOURCE=..\compat\unistd.h
# End Source File
# Begin Source File

SOURCE=..\..\filed\win32\winabout.h
# End Source File
# Begin Source File

SOURCE=..\..\filed\win32\winbacula.h
# End Source File
# Begin Source File

SOURCE=..\..\filed\win32\winevents.h
# End Source File
# Begin Source File

SOURCE=..\..\filed\win32\winres.h
# End Source File
# Begin Source File

SOURCE=..\..\filed\win32\winservice.h
# End Source File
# Begin Source File

SOURCE=..\..\filed\win32\winstat.h
# End Source File
# Begin Source File

SOURCE=..\..\filed\win32\wintray.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\..\src\filed\win32\bacula.bmp
# End Source File
# Begin Source File

SOURCE=..\..\src\filed\win32\bacula.ico
# End Source File
# Begin Source File

SOURCE=..\..\src\filed\win32\error.ico
# End Source File
# Begin Source File

SOURCE=..\..\src\filed\win32\idle.ico
# End Source File
# Begin Source File

SOURCE=..\..\src\filed\win32\running.ico
# End Source File
# Begin Source File

SOURCE=..\..\src\filed\win32\saving.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
