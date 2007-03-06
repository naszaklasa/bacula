# Microsoft Developer Studio Generated NMAKE File, Based on wx-console.dsp
!IF "$(CFG)" == ""
CFG=wx-console - Win32 Release
!MESSAGE No configuration specified. Defaulting to wx-console - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "wx-console - Win32 Release" && "$(CFG)" != "wx-console - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "wx-console.mak" CFG="wx-console - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "wx-console - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "wx-console - Win32 Debug" (based on "Win32 (x86) Console Application")
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

!IF  "$(CFG)" == "wx-console - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\wx-console.exe"


CLEAN :
	-@erase "$(INTDIR)\address_conf.obj"
	-@erase "$(INTDIR)\alist.obj"
	-@erase "$(INTDIR)\alloc.obj"
	-@erase "$(INTDIR)\attr.obj"
	-@erase "$(INTDIR)\base64.obj"
	-@erase "$(INTDIR)\berrno.obj"
	-@erase "$(INTDIR)\bget_msg.obj"
	-@erase "$(INTDIR)\bnet.obj"
	-@erase "$(INTDIR)\bnet_pkt.obj"
	-@erase "$(INTDIR)\bnet_server.obj"
	-@erase "$(INTDIR)\bshm.obj"
	-@erase "$(INTDIR)\bsys.obj"
	-@erase "$(INTDIR)\btime.obj"
	-@erase "$(INTDIR)\cram-md5.obj"
	-@erase "$(INTDIR)\crc32.obj"
	-@erase "$(INTDIR)\daemon.obj"
	-@erase "$(INTDIR)\dlist.obj"
	-@erase "$(INTDIR)\edit.obj"
	-@erase "$(INTDIR)\fnmatch.obj"
	-@erase "$(INTDIR)\hmac.obj"
	-@erase "$(INTDIR)\htable.obj"
	-@erase "$(INTDIR)\idcache.obj"
	-@erase "$(INTDIR)\jcr.obj"
	-@erase "$(INTDIR)\lex.obj"
	-@erase "$(INTDIR)\md5.obj"
	-@erase "$(INTDIR)\mem_pool.obj"
	-@erase "$(INTDIR)\message.obj"
	-@erase "$(INTDIR)\parse_conf.obj"
	-@erase "$(INTDIR)\queue.obj"
	-@erase "$(INTDIR)\res.obj"
	-@erase "$(INTDIR)\rwlock.obj"
	-@erase "$(INTDIR)\scan.obj"
	-@erase "$(INTDIR)\semlock.obj"
	-@erase "$(INTDIR)\serial.obj"
	-@erase "$(INTDIR)\sha1.obj"
	-@erase "$(INTDIR)\signal.obj"
	-@erase "$(INTDIR)\smartall.obj"
	-@erase "$(INTDIR)\btimers.obj"
	-@erase "$(INTDIR)\tls.obj"
	-@erase "$(INTDIR)\tree.obj"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(INTDIR)\var.obj"
	-@erase "$(INTDIR)\watchdog.obj"
	-@erase "$(INTDIR)\winapi.obj"
	-@erase "$(INTDIR)\workq.obj"
	-@erase "$(INTDIR)\compat.obj"
	-@erase "$(INTDIR)\print.obj"
	-@erase "$(INTDIR)\authenticate.obj"
	-@erase "$(INTDIR)\console_conf.obj"
	-@erase "$(INTDIR)\console_thread.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\wxblistctrl.obj"
	-@erase "$(INTDIR)\wxbmainframe.obj"
	-@erase "$(INTDIR)\wxbrestorepanel.obj"
	-@erase "$(INTDIR)\wxbtableparser.obj"
	-@erase "$(INTDIR)\wxbtreectrl.obj"
	-@erase "$(INTDIR)\wxbutils.obj"
	-@erase "$(INTDIR)\wxbconfigpanel.obj"
	-@erase "$(INTDIR)\wxbconfigfileeditor.obj"
	-@erase "$(INTDIR)\wxbhistorytextctrl.obj"
        -@erase "$(OUTDIR)\wx-console.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "../compat" /I "../.." /I "../../../depkgs-win32/wx/include" /I "../../../../depkgs-win32/wx/include" /I "../../../../depkgs-win32/wx/lib/msw" /I "../../../../depkgs-win32/pthreads" /I "../../../../depkgs-win32/zlib" /I "." /D "UNICODE" /D "NDEBUG" /D "WIN32" /D "__WXMSW__" /D "_CONSOLE" /D "_MBCS" /D "HAVE_WIN32" /D "HAVE_WXCONSOLE" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\winres.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\wx-console.bsc" 
BSC32_SBRS= \
        
LINK32=link.exe
LINK32_FLAGS=wxmswu.lib rpcrt4.lib oleaut32.lib ole32.lib uuid.lib winspool.lib winmm.lib \
  comctl32.lib comdlg32.lib Shell32.lib AdvAPI32.lib User32.lib Gdi32.lib wsock32.lib \
  wldap32.lib pthreadVCE.lib zlib.lib /nodefaultlib:libcmt.lib \
  /nologo /subsystem:windows /machine:I386 /out:"$(OUTDIR)\wx-console.exe" /libpath:"../../../../depkgs-win32/wx/lib" /libpath:"../../../../depkgs-win32/pthreads" /libpath:"../../../../depkgs-win32/zlib" 
LINK32_OBJS= \
	"$(INTDIR)\address_conf.obj" \
	"$(INTDIR)\alist.obj" \
	"$(INTDIR)\alloc.obj" \
	"$(INTDIR)\attr.obj" \
	"$(INTDIR)\base64.obj" \
	"$(INTDIR)\berrno.obj" \
	"$(INTDIR)\bget_msg.obj" \
	"$(INTDIR)\bnet.obj" \
	"$(INTDIR)\bnet_pkt.obj" \
	"$(INTDIR)\bnet_server.obj" \
	"$(INTDIR)\bshm.obj" \
	"$(INTDIR)\bsys.obj" \
	"$(INTDIR)\btime.obj" \
	"$(INTDIR)\cram-md5.obj" \
	"$(INTDIR)\crc32.obj" \
	"$(INTDIR)\daemon.obj" \
	"$(INTDIR)\dlist.obj" \
	"$(INTDIR)\edit.obj" \
	"$(INTDIR)\fnmatch.obj" \
	"$(INTDIR)\hmac.obj" \
	"$(INTDIR)\htable.obj" \
	"$(INTDIR)\idcache.obj" \
	"$(INTDIR)\jcr.obj" \
	"$(INTDIR)\lex.obj" \
	"$(INTDIR)\md5.obj" \
	"$(INTDIR)\mem_pool.obj" \
	"$(INTDIR)\message.obj" \
	"$(INTDIR)\parse_conf.obj" \
	"$(INTDIR)\queue.obj" \
	"$(INTDIR)\res.obj" \
	"$(INTDIR)\rwlock.obj" \
	"$(INTDIR)\scan.obj" \
	"$(INTDIR)\semlock.obj" \
	"$(INTDIR)\serial.obj" \
	"$(INTDIR)\sha1.obj" \
	"$(INTDIR)\signal.obj" \
	"$(INTDIR)\smartall.obj" \
	"$(INTDIR)\btimers.obj" \
	"$(INTDIR)\tls.obj" \
	"$(INTDIR)\tree.obj" \
	"$(INTDIR)\util.obj" \
	"$(INTDIR)\var.obj" \
	"$(INTDIR)\watchdog.obj" \
	"$(INTDIR)\winapi.obj" \
	"$(INTDIR)\workq.obj" \
	"$(INTDIR)\compat.obj" \
	"$(INTDIR)\print.obj" \
	"$(INTDIR)\authenticate.obj" \
	"$(INTDIR)\console_conf.obj" \
	"$(INTDIR)\console_thread.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\wxblistctrl.obj" \
	"$(INTDIR)\wxbmainframe.obj" \
	"$(INTDIR)\wxbrestorepanel.obj" \
	"$(INTDIR)\wxbtableparser.obj" \
	"$(INTDIR)\wxbtreectrl.obj" \
	"$(INTDIR)\wxbutils.obj" \
	"$(INTDIR)\wxbconfigpanel.obj" \
	"$(INTDIR)\wxbconfigfileeditor.obj" \
	"$(INTDIR)\wxbhistorytextctrl.obj" \
        "$(INTDIR)\wx-console_private.res"

"$(OUTDIR)\wx-console.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\wx-console.exe" "$(OUTDIR)\wx-console.bsc"

CLEAN :
	-@erase "$(INTDIR)\address_conf.obj
	-@erase "$(INTDIR)\address_conf.sbr"
	-@erase "$(INTDIR)\alist.obj
	-@erase "$(INTDIR)\alist.sbr"
	-@erase "$(INTDIR)\alloc.obj
	-@erase "$(INTDIR)\alloc.sbr"
	-@erase "$(INTDIR)\attr.obj
	-@erase "$(INTDIR)\attr.sbr"
	-@erase "$(INTDIR)\base64.obj
	-@erase "$(INTDIR)\base64.sbr"
	-@erase "$(INTDIR)\berrno.obj
	-@erase "$(INTDIR)\berrno.sbr"
	-@erase "$(INTDIR)\bget_msg.obj
	-@erase "$(INTDIR)\bget_msg.sbr"
	-@erase "$(INTDIR)\bnet.obj
	-@erase "$(INTDIR)\bnet.sbr"
	-@erase "$(INTDIR)\bnet_pkt.obj
	-@erase "$(INTDIR)\bnet_pkt.sbr"
	-@erase "$(INTDIR)\bnet_server.obj
	-@erase "$(INTDIR)\bnet_server.sbr"
	-@erase "$(INTDIR)\bshm.obj
	-@erase "$(INTDIR)\bshm.sbr"
	-@erase "$(INTDIR)\bsys.obj
	-@erase "$(INTDIR)\bsys.sbr"
	-@erase "$(INTDIR)\btime.obj
	-@erase "$(INTDIR)\btime.sbr"
	-@erase "$(INTDIR)\cram-md5.obj
	-@erase "$(INTDIR)\cram-md5.sbr"
	-@erase "$(INTDIR)\crc32.obj
	-@erase "$(INTDIR)\crc32.sbr"
	-@erase "$(INTDIR)\daemon.obj
	-@erase "$(INTDIR)\daemon.sbr"
	-@erase "$(INTDIR)\dlist.obj
	-@erase "$(INTDIR)\dlist.sbr"
	-@erase "$(INTDIR)\edit.obj
	-@erase "$(INTDIR)\edit.sbr"
	-@erase "$(INTDIR)\fnmatch.obj
	-@erase "$(INTDIR)\fnmatch.sbr"
	-@erase "$(INTDIR)\hmac.obj
	-@erase "$(INTDIR)\hmac.sbr"
	-@erase "$(INTDIR)\htable.obj
	-@erase "$(INTDIR)\htable.sbr"
	-@erase "$(INTDIR)\idcache.obj
	-@erase "$(INTDIR)\idcache.sbr"
	-@erase "$(INTDIR)\jcr.obj
	-@erase "$(INTDIR)\jcr.sbr"
	-@erase "$(INTDIR)\lex.obj
	-@erase "$(INTDIR)\lex.sbr"
	-@erase "$(INTDIR)\md5.obj
	-@erase "$(INTDIR)\md5.sbr"
	-@erase "$(INTDIR)\mem_pool.obj
	-@erase "$(INTDIR)\mem_pool.sbr"
	-@erase "$(INTDIR)\message.obj
	-@erase "$(INTDIR)\message.sbr"
	-@erase "$(INTDIR)\parse_conf.obj
	-@erase "$(INTDIR)\parse_conf.sbr"
	-@erase "$(INTDIR)\queue.obj
	-@erase "$(INTDIR)\queue.sbr"
	-@erase "$(INTDIR)\res.obj
	-@erase "$(INTDIR)\res.sbr"
	-@erase "$(INTDIR)\rwlock.obj
	-@erase "$(INTDIR)\rwlock.sbr"
	-@erase "$(INTDIR)\scan.obj
	-@erase "$(INTDIR)\scan.sbr"
	-@erase "$(INTDIR)\semlock.obj
	-@erase "$(INTDIR)\semlock.sbr"
	-@erase "$(INTDIR)\serial.obj
	-@erase "$(INTDIR)\serial.sbr"
	-@erase "$(INTDIR)\sha1.obj
	-@erase "$(INTDIR)\sha1.sbr"
	-@erase "$(INTDIR)\signal.obj
	-@erase "$(INTDIR)\signal.sbr"
	-@erase "$(INTDIR)\smartall.obj
	-@erase "$(INTDIR)\smartall.sbr"
	-@erase "$(INTDIR)\btimers.obj
	-@erase "$(INTDIR)\btimers.sbr"
	-@erase "$(INTDIR)\tls.obj
	-@erase "$(INTDIR)\tls.sbr"
	-@erase "$(INTDIR)\tree.obj
	-@erase "$(INTDIR)\tree.sbr"
	-@erase "$(INTDIR)\util.obj
	-@erase "$(INTDIR)\util.sbr"
	-@erase "$(INTDIR)\var.obj
	-@erase "$(INTDIR)\var.sbr"
	-@erase "$(INTDIR)\watchdog.obj
	-@erase "$(INTDIR)\watchdog.sbr"
	-@erase "$(INTDIR)\winapi.obj
	-@erase "$(INTDIR)\winapi.sbr"
	-@erase "$(INTDIR)\workq.obj
	-@erase "$(INTDIR)\workq.sbr"
	-@erase "$(INTDIR)\compat.obj
	-@erase "$(INTDIR)\compat.sbr"
	-@erase "$(INTDIR)\print.obj
	-@erase "$(INTDIR)\print.sbr"
	-@erase "$(INTDIR)\authenticate.obj
	-@erase "$(INTDIR)\authenticate.sbr"
	-@erase "$(INTDIR)\console_conf.obj
	-@erase "$(INTDIR)\console_conf.sbr"
	-@erase "$(INTDIR)\console_thread.obj
	-@erase "$(INTDIR)\console_thread.sbr"
	-@erase "$(INTDIR)\main.obj
	-@erase "$(INTDIR)\main.sbr"
	-@erase "$(INTDIR)\wxblistctrl.obj
	-@erase "$(INTDIR)\wxblistctrl.sbr"
	-@erase "$(INTDIR)\wxbmainframe.obj
	-@erase "$(INTDIR)\wxbmainframe.sbr"
	-@erase "$(INTDIR)\wxbrestorepanel.obj
	-@erase "$(INTDIR)\wxbrestorepanel.sbr"
	-@erase "$(INTDIR)\wxbtableparser.obj
	-@erase "$(INTDIR)\wxbtableparser.sbr"
	-@erase "$(INTDIR)\wxbtreectrl.obj
	-@erase "$(INTDIR)\wxbtreectrl.sbr"
	-@erase "$(INTDIR)\wxbutils.obj
	-@erase "$(INTDIR)\wxbutils.sbr"
	-@erase "$(INTDIR)\wxbconfigpanel.obj
	-@erase "$(INTDIR)\wxbconfigpanel.sbr"
	-@erase "$(INTDIR)\wxbconfigfileeditor.obj
	-@erase "$(INTDIR)\wxbconfigfileeditor.sbr"
	-@erase "$(INTDIR)\wxbhistorytextctrl.obj
	-@erase "$(INTDIR)\wxbhistorytextctrl.sbr"
        -@erase "$(OUTDIR)\wx-console.exe"
        -@erase "$(OUTDIR)\wx-console.bsc"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"


CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../compat" /I "../.." /I "../../../depkgs-win32/wx/include" /I "../../../../depkgs-win32/wx/include" /I "../../../../depkgs-win32/wx/lib/mswd" /I "../../../../depkgs-win32/pthreads" /I "../../../../depkgs-win32/zlib" /I "." /D "UNICODE" /D "_DEBUG" /D "WIN32" /D "__WXMSW__" /D "_CONSOLE" /D "_MBCS" /D "HAVE_WIN32" /D "HAVE_WXCONSOLE" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\winres.res" /d "_DEBUG"
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\wx-console.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\address_conf.sbr" \
	"$(INTDIR)\alist.sbr" \
	"$(INTDIR)\alloc.sbr" \
	"$(INTDIR)\attr.sbr" \
	"$(INTDIR)\base64.sbr" \
	"$(INTDIR)\berrno.sbr" \
	"$(INTDIR)\bget_msg.sbr" \
	"$(INTDIR)\bnet.sbr" \
	"$(INTDIR)\bnet_pkt.sbr" \
	"$(INTDIR)\bnet_server.sbr" \
	"$(INTDIR)\bshm.sbr" \
	"$(INTDIR)\bsys.sbr" \
	"$(INTDIR)\btime.sbr" \
	"$(INTDIR)\cram-md5.sbr" \
	"$(INTDIR)\crc32.sbr" \
	"$(INTDIR)\daemon.sbr" \
	"$(INTDIR)\dlist.sbr" \
	"$(INTDIR)\edit.sbr" \
	"$(INTDIR)\fnmatch.sbr" \
	"$(INTDIR)\hmac.sbr" \
	"$(INTDIR)\htable.sbr" \
	"$(INTDIR)\idcache.sbr" \
	"$(INTDIR)\jcr.sbr" \
	"$(INTDIR)\lex.sbr" \
	"$(INTDIR)\md5.sbr" \
	"$(INTDIR)\mem_pool.sbr" \
	"$(INTDIR)\message.sbr" \
	"$(INTDIR)\parse_conf.sbr" \
	"$(INTDIR)\queue.sbr" \
	"$(INTDIR)\res.sbr" \
	"$(INTDIR)\rwlock.sbr" \
	"$(INTDIR)\scan.sbr" \
	"$(INTDIR)\semlock.sbr" \
	"$(INTDIR)\serial.sbr" \
	"$(INTDIR)\sha1.sbr" \
	"$(INTDIR)\signal.sbr" \
	"$(INTDIR)\smartall.sbr" \
	"$(INTDIR)\btimers.sbr" \
	"$(INTDIR)\tls.sbr" \
	"$(INTDIR)\tree.sbr" \
	"$(INTDIR)\util.sbr" \
	"$(INTDIR)\var.sbr" \
	"$(INTDIR)\watchdog.sbr" \
	"$(INTDIR)\winapi.sbr" \
	"$(INTDIR)\workq.sbr" \
	"$(INTDIR)\compat.sbr" \
	"$(INTDIR)\print.sbr" \
	"$(INTDIR)\authenticate.sbr" \
	"$(INTDIR)\console_conf.sbr" \
	"$(INTDIR)\console_thread.sbr" \
	"$(INTDIR)\main.sbr" \
	"$(INTDIR)\wxblistctrl.sbr" \
	"$(INTDIR)\wxbmainframe.sbr" \
	"$(INTDIR)\wxbrestorepanel.sbr" \
	"$(INTDIR)\wxbtableparser.sbr" \
	"$(INTDIR)\wxbtreectrl.sbr" \
	"$(INTDIR)\wxbutils.sbr" \
	"$(INTDIR)\wxbconfigpanel.sbr" \
	"$(INTDIR)\wxbconfigfileeditor.sbr" \
	"$(INTDIR)\wxbhistorytextctrl.sbr" \

"$(OUTDIR)\wx-console.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=wxmswu.lib rpcrt4.lib oleaut32.lib ole32.lib uuid.lib winspool.lib winmm.lib \
  comctl32.lib comdlg32.lib Shell32.lib AdvAPI32.lib User32.lib Gdi32.lib wsock32.lib \
  wldap32.lib pthreadVCE.lib zlib.lib /nodefaultlib:libcmtd.lib \
  /nologo /subsystem:windows /pdb:none /debug /machine:I386 /out:"$(OUTDIR)\wx-console.exe" /libpath:"../../../../depkgs-win32/wx/lib" /libpath:"../../../../depkgs-win32/pthreads" /libpath:"../../../../depkgs-win32/zlib" 
LINK32_OBJS= \
	"$(INTDIR)\address_conf.obj" \
	"$(INTDIR)\alist.obj" \
	"$(INTDIR)\alloc.obj" \
	"$(INTDIR)\attr.obj" \
	"$(INTDIR)\base64.obj" \
	"$(INTDIR)\berrno.obj" \
	"$(INTDIR)\bget_msg.obj" \
	"$(INTDIR)\bnet.obj" \
	"$(INTDIR)\bnet_pkt.obj" \
	"$(INTDIR)\bnet_server.obj" \
	"$(INTDIR)\bshm.obj" \
	"$(INTDIR)\bsys.obj" \
	"$(INTDIR)\btime.obj" \
	"$(INTDIR)\cram-md5.obj" \
	"$(INTDIR)\crc32.obj" \
	"$(INTDIR)\daemon.obj" \
	"$(INTDIR)\dlist.obj" \
	"$(INTDIR)\edit.obj" \
	"$(INTDIR)\fnmatch.obj" \
	"$(INTDIR)\hmac.obj" \
	"$(INTDIR)\htable.obj" \
	"$(INTDIR)\idcache.obj" \
	"$(INTDIR)\jcr.obj" \
	"$(INTDIR)\lex.obj" \
	"$(INTDIR)\md5.obj" \
	"$(INTDIR)\mem_pool.obj" \
	"$(INTDIR)\message.obj" \
	"$(INTDIR)\parse_conf.obj" \
	"$(INTDIR)\queue.obj" \
	"$(INTDIR)\res.obj" \
	"$(INTDIR)\rwlock.obj" \
	"$(INTDIR)\scan.obj" \
	"$(INTDIR)\semlock.obj" \
	"$(INTDIR)\serial.obj" \
	"$(INTDIR)\sha1.obj" \
	"$(INTDIR)\signal.obj" \
	"$(INTDIR)\smartall.obj" \
	"$(INTDIR)\btimers.obj" \
	"$(INTDIR)\tls.obj" \
	"$(INTDIR)\tree.obj" \
	"$(INTDIR)\util.obj" \
	"$(INTDIR)\var.obj" \
	"$(INTDIR)\watchdog.obj" \
	"$(INTDIR)\winapi.obj" \
	"$(INTDIR)\workq.obj" \
	"$(INTDIR)\compat.obj" \
	"$(INTDIR)\print.obj" \
	"$(INTDIR)\authenticate.obj" \
	"$(INTDIR)\console_conf.obj" \
	"$(INTDIR)\console_thread.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\wxblistctrl.obj" \
	"$(INTDIR)\wxbmainframe.obj" \
	"$(INTDIR)\wxbrestorepanel.obj" \
	"$(INTDIR)\wxbtableparser.obj" \
	"$(INTDIR)\wxbtreectrl.obj" \
	"$(INTDIR)\wxbutils.obj" \
	"$(INTDIR)\wxbconfigpanel.obj" \
	"$(INTDIR)\wxbconfigfileeditor.obj" \
	"$(INTDIR)\wxbhistorytextctrl.obj" \
        "$(INTDIR)\wx-console_private.res"

"$(OUTDIR)\wx-console.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("wx-console.dep")
!INCLUDE "wx-console.dep"
!ELSE 
!MESSAGE Warning: cannot find "wx-console.dep"
!ENDIF 
!ENDIF 

SOURCE=..\..\wx-console\wx-console_private.rc

"$(INTDIR)\wx-console_private.res" : $(SOURCE) "$(INTDIR)"
        $(RSC) /l 0x409 /fo"$(INTDIR)\wx-console_private.res" /d "NDEBUG" $(SOURCE)

FILENAME=address_conf
SOURCE=..\lib\address_conf.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=alist
SOURCE=..\lib\alist.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=alloc
SOURCE=..\lib\alloc.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=attr
SOURCE=..\lib\attr.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=base64
SOURCE=..\lib\base64.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=berrno
SOURCE=..\lib\berrno.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=bget_msg
SOURCE=..\lib\bget_msg.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=bnet
SOURCE=..\lib\bnet.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=bnet_pkt
SOURCE=..\lib\bnet_pkt.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=bnet_server
SOURCE=..\lib\bnet_server.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=bshm
SOURCE=..\lib\bshm.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=bsys
SOURCE=..\lib\bsys.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=btime
SOURCE=..\lib\btime.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=cram-md5
SOURCE=..\lib\cram-md5.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=crc32
SOURCE=..\lib\crc32.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=daemon
SOURCE=..\lib\daemon.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=dlist
SOURCE=..\lib\dlist.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=edit
SOURCE=..\lib\edit.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=fnmatch
SOURCE=..\lib\fnmatch.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=hmac
SOURCE=..\lib\hmac.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=htable
SOURCE=..\lib\htable.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=idcache
SOURCE=..\lib\idcache.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=jcr
SOURCE=..\lib\jcr.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=lex
SOURCE=..\lib\lex.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=md5
SOURCE=..\lib\md5.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=mem_pool
SOURCE=..\lib\mem_pool.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=message
SOURCE=..\lib\message.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=parse_conf
SOURCE=..\lib\parse_conf.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=queue
SOURCE=..\lib\queue.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=res
SOURCE=..\lib\res.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=rwlock
SOURCE=..\lib\rwlock.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=scan
SOURCE=..\lib\scan.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=semlock
SOURCE=..\lib\semlock.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=serial
SOURCE=..\lib\serial.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=sha1
SOURCE=..\lib\sha1.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=signal
SOURCE=..\lib\signal.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=smartall
SOURCE=..\lib\smartall.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=btimers
SOURCE=..\lib\btimers.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=tls
SOURCE=..\lib\tls.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=tree
SOURCE=..\lib\tree.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=util
SOURCE=..\lib\util.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=var
SOURCE=..\lib\var.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=watchdog
SOURCE=..\lib\watchdog.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=winapi
SOURCE=..\lib\winapi.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=workq
SOURCE=..\lib\workq.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=compat
SOURCE=..\compat\compat.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=print
SOURCE=..\compat\print.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=authenticate
SOURCE=.\authenticate.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=console_conf
SOURCE=.\console_conf.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=console_thread
SOURCE=..\..\wx-console\console_thread.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=main
SOURCE=..\..\wx-console\main.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=wxblistctrl
SOURCE=..\..\wx-console\wxblistctrl.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=wxbmainframe
SOURCE=..\..\wx-console\wxbmainframe.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=wxbrestorepanel
SOURCE=..\..\wx-console\wxbrestorepanel.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=wxbtableparser
SOURCE=..\..\wx-console\wxbtableparser.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=wxbtreectrl
SOURCE=..\..\wx-console\wxbtreectrl.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=wxbutils
SOURCE=..\..\wx-console\wxbutils.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=wxbconfigpanel
SOURCE=..\..\wx-console\wxbconfigpanel.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=wxbconfigfileeditor
SOURCE=..\..\wx-console\wxbconfigfileeditor.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


FILENAME=wxbhistorytextctrl
SOURCE=..\..\wx-console\wxbhistorytextctrl.cpp
!IF  "$(CFG)" == "wx-console - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "wx-console - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"	"$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


