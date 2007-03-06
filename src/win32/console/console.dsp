# Microsoft Developer Studio Project File - Name="console" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=console - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "console.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "console.mak" CFG="console - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "console - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "console - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "console - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /I "../compat" /I "../.." /I "../../../../depkgs-win32/pthreads" /I "." /D "_DEBUG" /D "HAVE_CONSOLE" /D "_WINMAIN_" /D "PTW32_BUILD" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_WIN32" /D "_AFXDLL" /FD /I /GZ /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 wsock32.lib pthreadVCE.lib /nologo /subsystem:console /pdb:none /machine:I386 /out:"Release/bconsole.exe" /libpath:"../../../../depkgs-win32/pthreads"

!ELSEIF  "$(CFG)" == "console - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../compat" /I "../.." /I "../../../../depkgs-win32/pthreads" /I "." /D "_DEBUG" /D "HAVE_CONSOLE" /D "_WINMAIN_" /D "PTW32_BUILD" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_WIN32" /D "_AFXDLL" /FD /I /GZ /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wsock32.lib pthreadVCE.lib /nologo /subsystem:console /pdb:none /debug /machine:I386 /out:"Debug/bconsole.exe" /libpath:"../../../../depkgs-win32/pthreads"

!ENDIF 

# Begin Target

# Name "console - Win32 Release"
# Name "console - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\lib\alist.cpp
# End Source File
# Begin Source File

SOURCE=.\authenticate.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\base64.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\bnet.cpp
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

SOURCE=.\console.cpp
# End Source File
# Begin Source File

SOURCE=.\console_conf.cpp
# End Source File
# Begin Source File

SOURCE="..\lib\cram-md5.cpp"
# End Source File
# Begin Source File

SOURCE=..\lib\crc32.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\dlist.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\edit.cpp
# End Source File
# Begin Source File

SOURCE=..\compat\getopt.c
# End Source File
# Begin Source File

SOURCE=..\lib\hmac.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\idcache.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\jcr.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\lex.cpp
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

SOURCE=..\lib\rwlock.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\scan.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\serial.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\sha1.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\smartall.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=..\lib\util.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\watchdog.cpp
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

SOURCE=..\compat\dirent.h
# End Source File
# Begin Source File

SOURCE=..\compat\getopt.h
# End Source File
# Begin Source File

SOURCE=..\compat\grp.h
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

SOURCE=..\compat\winconfig.h
# End Source File
# Begin Source File

SOURCE=..\compat\winhost.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
