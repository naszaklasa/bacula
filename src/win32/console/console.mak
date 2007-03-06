# Microsoft Developer Studio Generated NMAKE File, Based on console.dsp
!IF "$(CFG)" == ""
CFG=console - Win32 Debug
!MESSAGE No configuration specified. Defaulting to console - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "console - Win32 Release" && "$(CFG)" != "console - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
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
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "console - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\bconsole.exe" "$(OUTDIR)\console.pch"


CLEAN :
        -@erase "$(INTDIR)\address_conf.obj"
        -@erase "$(INTDIR)\alist.obj"
        -@erase "$(INTDIR)\authenticate.obj"
        -@erase "$(INTDIR)\base64.obj"
        -@erase "$(INTDIR)\berrno.obj"
        -@erase "$(INTDIR)\bnet.obj"
        -@erase "$(INTDIR)\bsys.obj"
        -@erase "$(INTDIR)\btime.obj"
        -@erase "$(INTDIR)\compat.obj"
        -@erase "$(INTDIR)\console.obj"
        -@erase "$(INTDIR)\console.pch"
        -@erase "$(INTDIR)\console_conf.obj"
        -@erase "$(INTDIR)\cram-md5.obj"
        -@erase "$(INTDIR)\crc32.obj"
        -@erase "$(INTDIR)\dlist.obj"
        -@erase "$(INTDIR)\edit.obj"
        -@erase "$(INTDIR)\getopt.obj"
        -@erase "$(INTDIR)\hmac.obj"
        -@erase "$(INTDIR)\idcache.obj"
        -@erase "$(INTDIR)\jcr.obj"
        -@erase "$(INTDIR)\lex.obj"
        -@erase "$(INTDIR)\md5.obj"
        -@erase "$(INTDIR)\mem_pool.obj"
        -@erase "$(INTDIR)\message.obj"
        -@erase "$(INTDIR)\parse_conf.obj"
        -@erase "$(INTDIR)\print.obj"
        -@erase "$(INTDIR)\queue.obj"
        -@erase "$(INTDIR)\rwlock.obj"
        -@erase "$(INTDIR)\res.obj"
        -@erase "$(INTDIR)\scan.obj"
        -@erase "$(INTDIR)\serial.obj"
        -@erase "$(INTDIR)\sha1.obj"
        -@erase "$(INTDIR)\smartall.obj"
        -@erase "$(INTDIR)\tls.obj"
        -@erase "$(INTDIR)\StdAfx.obj"
        -@erase "$(INTDIR)\btimers.obj"
        -@erase "$(INTDIR)\util.obj"
        -@erase "$(INTDIR)\vc60.idb"
        -@erase "$(INTDIR)\watchdog.obj"
        -@erase "$(INTDIR)\winapi.obj"
        -@erase "$(OUTDIR)\bconsole.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /I "../compat" /I "../.." /I "../../../../depkgs-win32/pthreads" /I "." /D "_DEBUG" /D "HAVE_CONSOLE" /D "_WINMAIN_" /D "PTW32_BUILD" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_WIN32" /D "_AFXDLL" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /I /GZ /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\console.bsc" 
BSC32_SBRS= \
        
LINK32=link.exe
LINK32_FLAGS=wsock32.lib pthreadVCE.lib /nologo /subsystem:console /pdb:none /machine:I386 /out:"$(OUTDIR)\bconsole.exe" /libpath:"../../../../depkgs-win32/pthreads" 
LINK32_OBJS= \
        "$(INTDIR)\address_conf.obj" \
        "$(INTDIR)\alist.obj" \
        "$(INTDIR)\authenticate.obj" \
        "$(INTDIR)\base64.obj" \
        "$(INTDIR)\berrno.obj" \
        "$(INTDIR)\bnet.obj" \
        "$(INTDIR)\bsys.obj" \
        "$(INTDIR)\btime.obj" \
        "$(INTDIR)\compat.obj" \
        "$(INTDIR)\console.obj" \
        "$(INTDIR)\console_conf.obj" \
        "$(INTDIR)\cram-md5.obj" \
        "$(INTDIR)\crc32.obj" \
        "$(INTDIR)\dlist.obj" \
        "$(INTDIR)\edit.obj" \
        "$(INTDIR)\getopt.obj" \
        "$(INTDIR)\hmac.obj" \
        "$(INTDIR)\idcache.obj" \
        "$(INTDIR)\jcr.obj" \
        "$(INTDIR)\lex.obj" \
        "$(INTDIR)\md5.obj" \
        "$(INTDIR)\mem_pool.obj" \
        "$(INTDIR)\message.obj" \
        "$(INTDIR)\parse_conf.obj" \
        "$(INTDIR)\print.obj" \
        "$(INTDIR)\queue.obj" \
        "$(INTDIR)\rwlock.obj" \
        "$(INTDIR)\res.obj" \
        "$(INTDIR)\scan.obj" \
        "$(INTDIR)\serial.obj" \
        "$(INTDIR)\sha1.obj" \
        "$(INTDIR)\smartall.obj" \
        "$(INTDIR)\tls.obj" \
        "$(INTDIR)\StdAfx.obj" \
        "$(INTDIR)\btimers.obj" \
        "$(INTDIR)\util.obj" \
        "$(INTDIR)\watchdog.obj" \
        "$(INTDIR)\winapi.obj"

"$(OUTDIR)\bconsole.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "console - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\bconsole.exe" "$(OUTDIR)\console.pch"


CLEAN :
        -@erase "$(INTDIR)\address_conf.obj"
        -@erase "$(INTDIR)\alist.obj"
        -@erase "$(INTDIR)\authenticate.obj"
        -@erase "$(INTDIR)\base64.obj"
        -@erase "$(INTDIR)\berrno.obj"
        -@erase "$(INTDIR)\bnet.obj"
        -@erase "$(INTDIR)\bsys.obj"
        -@erase "$(INTDIR)\btime.obj"
        -@erase "$(INTDIR)\compat.obj"
        -@erase "$(INTDIR)\console.obj"
        -@erase "$(INTDIR)\console.pch"
        -@erase "$(INTDIR)\console_conf.obj"
        -@erase "$(INTDIR)\cram-md5.obj"
        -@erase "$(INTDIR)\crc32.obj"
        -@erase "$(INTDIR)\dlist.obj"
        -@erase "$(INTDIR)\edit.obj"
        -@erase "$(INTDIR)\getopt.obj"
        -@erase "$(INTDIR)\hmac.obj"
        -@erase "$(INTDIR)\idcache.obj"
        -@erase "$(INTDIR)\jcr.obj"
        -@erase "$(INTDIR)\lex.obj"
        -@erase "$(INTDIR)\md5.obj"
        -@erase "$(INTDIR)\mem_pool.obj"
        -@erase "$(INTDIR)\message.obj"
        -@erase "$(INTDIR)\parse_conf.obj"
        -@erase "$(INTDIR)\print.obj"
        -@erase "$(INTDIR)\queue.obj"
        -@erase "$(INTDIR)\rwlock.obj"
        -@erase "$(INTDIR)\res.obj"
        -@erase "$(INTDIR)\scan.obj"
        -@erase "$(INTDIR)\serial.obj"
        -@erase "$(INTDIR)\sha1.obj"
        -@erase "$(INTDIR)\smartall.obj"
        -@erase "$(INTDIR)\tls.obj"
        -@erase "$(INTDIR)\StdAfx.obj"
        -@erase "$(INTDIR)\btimers.obj"
        -@erase "$(INTDIR)\util.obj"
        -@erase "$(INTDIR)\vc60.idb"
        -@erase "$(INTDIR)\vc60.pdb"
        -@erase "$(INTDIR)\watchdog.obj"
        -@erase "$(INTDIR)\winapi.obj"
        -@erase "$(OUTDIR)\bconsole.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../compat" /I "../.." /I "../../../../depkgs-win32/pthreads" /I "." /D "_DEBUG" /D "HAVE_CONSOLE" /D "_WINMAIN_" /D "PTW32_BUILD" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_WIN32" /D "_AFXDLL" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /I /GZ /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\console.bsc" 
BSC32_SBRS= \
        
LINK32=link.exe
LINK32_FLAGS=wsock32.lib pthreadVCE.lib /nologo /subsystem:console /pdb:none /debug /machine:I386 /out:"$(OUTDIR)\bconsole.exe" /libpath:"../../../../depkgs-win32/pthreads" 
LINK32_OBJS= \
        "$(INTDIR)\address_conf.obj" \
        "$(INTDIR)\alist.obj" \
        "$(INTDIR)\authenticate.obj" \
        "$(INTDIR)\base64.obj" \
        "$(INTDIR)\berrno.obj" \
        "$(INTDIR)\bnet.obj" \
        "$(INTDIR)\bsys.obj" \
        "$(INTDIR)\btime.obj" \
        "$(INTDIR)\compat.obj" \
        "$(INTDIR)\console.obj" \
        "$(INTDIR)\console_conf.obj" \
        "$(INTDIR)\cram-md5.obj" \
        "$(INTDIR)\crc32.obj" \
        "$(INTDIR)\dlist.obj" \
        "$(INTDIR)\edit.obj" \
        "$(INTDIR)\getopt.obj" \
        "$(INTDIR)\hmac.obj" \
        "$(INTDIR)\idcache.obj" \
        "$(INTDIR)\jcr.obj" \
        "$(INTDIR)\lex.obj" \
        "$(INTDIR)\md5.obj" \
        "$(INTDIR)\mem_pool.obj" \
        "$(INTDIR)\message.obj" \
        "$(INTDIR)\parse_conf.obj" \
        "$(INTDIR)\print.obj" \
        "$(INTDIR)\queue.obj" \
        "$(INTDIR)\rwlock.obj" \
        "$(INTDIR)\res.obj" \
        "$(INTDIR)\scan.obj" \
        "$(INTDIR)\serial.obj" \
        "$(INTDIR)\sha1.obj" \
        "$(INTDIR)\smartall.obj" \
        "$(INTDIR)\tls.obj" \
        "$(INTDIR)\StdAfx.obj" \
        "$(INTDIR)\btimers.obj" \
        "$(INTDIR)\util.obj" \
        "$(INTDIR)\watchdog.obj" \
        "$(INTDIR)\winapi.obj"

"$(OUTDIR)\bconsole.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("console.dep")
!INCLUDE "console.dep"
!ELSE 
!MESSAGE Warning: cannot find "console.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "console - Win32 Release" || "$(CFG)" == "console - Win32 Debug"

SOURCE=..\lib\address_conf.cpp

"$(INTDIR)\address_conf.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\lib\alist.cpp

"$(INTDIR)\alist.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)



SOURCE=.\authenticate.cpp

"$(INTDIR)\authenticate.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=..\lib\base64.cpp

"$(INTDIR)\base64.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\lib\berrno.cpp

"$(INTDIR)\berrno.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)



SOURCE=..\lib\bnet.cpp

"$(INTDIR)\bnet.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\lib\bsys.cpp

"$(INTDIR)\bsys.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\lib\btime.cpp

"$(INTDIR)\btime.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\compat\compat.cpp

"$(INTDIR)\compat.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\console.cpp

"$(INTDIR)\console.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\console_conf.cpp

"$(INTDIR)\console_conf.obj" : $(SOURCE) "$(INTDIR)"


SOURCE="..\lib\cram-md5.cpp"

"$(INTDIR)\cram-md5.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\lib\crc32.cpp

"$(INTDIR)\crc32.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\lib\dlist.cpp

"$(INTDIR)\dlist.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\lib\edit.cpp

"$(INTDIR)\edit.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\compat\getopt.c

"$(INTDIR)\getopt.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\lib\hmac.cpp

"$(INTDIR)\hmac.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\lib\idcache.cpp

"$(INTDIR)\idcache.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\lib\jcr.cpp

"$(INTDIR)\jcr.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\lib\lex.cpp

"$(INTDIR)\lex.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\lib\md5.cpp

"$(INTDIR)\md5.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\lib\mem_pool.cpp

"$(INTDIR)\mem_pool.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\lib\message.cpp

"$(INTDIR)\message.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\lib\parse_conf.cpp

"$(INTDIR)\parse_conf.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\compat\print.cpp

"$(INTDIR)\print.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\lib\queue.cpp

"$(INTDIR)\queue.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\lib\rwlock.cpp

"$(INTDIR)\rwlock.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\lib\res.cpp

"$(INTDIR)\res.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\lib\scan.cpp

"$(INTDIR)\scan.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\lib\serial.cpp

"$(INTDIR)\serial.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\lib\sha1.cpp

"$(INTDIR)\sha1.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\lib\smartall.cpp

"$(INTDIR)\smartall.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=..\lib\tls.cpp

"$(INTDIR)\tls.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "console - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /I "../compat" /I "../.." /I "../../../../depkgs-win32/pthreads" /I "." /D "_DEBUG" /D "HAVE_CONSOLE" /D "_WINMAIN_" /D "PTW32_BUILD" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_WIN32" /D "_AFXDLL" /Fp"$(INTDIR)\console.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /I /GZ /c 

"$(INTDIR)\StdAfx.obj"  "$(INTDIR)\console.pch" : $(SOURCE) "$(INTDIR)"
        $(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "console - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../compat" /I "../.." /I "../../../../depkgs-win32/pthreads" /I "." /D "_DEBUG" /D "HAVE_CONSOLE" /D "_WINMAIN_" /D "PTW32_BUILD" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_WIN32" /D "_AFXDLL" /Fp"$(INTDIR)\console.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /I /GZ /c 

"$(INTDIR)\StdAfx.obj"  "$(INTDIR)\console.pch" : $(SOURCE) "$(INTDIR)"
        $(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=..\lib\btimers.cpp

"$(INTDIR)\btimers.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\lib\util.cpp

"$(INTDIR)\util.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\lib\watchdog.cpp

"$(INTDIR)\watchdog.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\lib\winapi.cpp

"$(INTDIR)\winapi.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)




!ENDIF 
