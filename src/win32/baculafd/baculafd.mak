# Microsoft Developer Studio Generated NMAKE File, Based on baculafd.dsp
!IF "$(CFG)" == ""
CFG=baculafd - Win32 Debug
!MESSAGE No configuration specified. Defaulting to baculafd - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "baculafd - Win32 Release" && "$(CFG)" != "baculafd - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
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
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "baculafd - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\bacula-fd.exe"


CLEAN :
        -@erase "$(INTDIR)\address_conf.obj"
        -@erase "$(INTDIR)\alist.obj"
        -@erase "$(INTDIR)\alloc.obj"
        -@erase "$(INTDIR)\attr.obj"
        -@erase "$(INTDIR)\attribs.obj"
        -@erase "$(INTDIR)\authenticate.obj"
        -@erase "$(INTDIR)\backup.obj"
        -@erase "$(INTDIR)\base64.obj"
        -@erase "$(INTDIR)\berrno.obj"
        -@erase "$(INTDIR)\bfile.obj"
        -@erase "$(INTDIR)\bget_msg.obj"
        -@erase "$(INTDIR)\bnet.obj"
        -@erase "$(INTDIR)\bnet_server.obj"
        -@erase "$(INTDIR)\bshm.obj"
        -@erase "$(INTDIR)\bsys.obj"
        -@erase "$(INTDIR)\btime.obj"
        -@erase "$(INTDIR)\btimers.obj"
        -@erase "$(INTDIR)\chksum.obj"
        -@erase "$(INTDIR)\compat.obj"
        -@erase "$(INTDIR)\cram-md5.obj"
        -@erase "$(INTDIR)\crc32.obj"
        -@erase "$(INTDIR)\create_file.obj"
        -@erase "$(INTDIR)\daemon.obj"
        -@erase "$(INTDIR)\dlist.obj"
        -@erase "$(INTDIR)\edit.obj"
        -@erase "$(INTDIR)\enable_priv.obj"
        -@erase "$(INTDIR)\estimate.obj"
        -@erase "$(INTDIR)\filed.obj"
        -@erase "$(INTDIR)\filed_conf.obj"
        -@erase "$(INTDIR)\find.obj"
        -@erase "$(INTDIR)\find_one.obj"
        -@erase "$(INTDIR)\fnmatch.obj"
        -@erase "$(INTDIR)\fstype.obj"
        -@erase "$(INTDIR)\getopt.obj"
        -@erase "$(INTDIR)\heartbeat.obj"
        -@erase "$(INTDIR)\hmac.obj"
        -@erase "$(INTDIR)\htable.obj"
        -@erase "$(INTDIR)\idcache.obj"
        -@erase "$(INTDIR)\jcr.obj"
        -@erase "$(INTDIR)\job.obj"
        -@erase "$(INTDIR)\lex.obj"
        -@erase "$(INTDIR)\makepath.obj"
        -@erase "$(INTDIR)\match.obj"
        -@erase "$(INTDIR)\md5.obj"
        -@erase "$(INTDIR)\mem_pool.obj"
        -@erase "$(INTDIR)\message.obj"
        -@erase "$(INTDIR)\parse_conf.obj"
        -@erase "$(INTDIR)\print.obj"
        -@erase "$(INTDIR)\pythonlib.obj"
        -@erase "$(INTDIR)\queue.obj"
        -@erase "$(INTDIR)\bregex.obj"
        -@erase "$(INTDIR)\restore.obj"
        -@erase "$(INTDIR)\res.obj"
        -@erase "$(INTDIR)\rwlock.obj"
        -@erase "$(INTDIR)\save-cwd.obj"
        -@erase "$(INTDIR)\scan.obj"
        -@erase "$(INTDIR)\semlock.obj"
        -@erase "$(INTDIR)\serial.obj"
        -@erase "$(INTDIR)\sha1.obj"
        -@erase "$(INTDIR)\signal.obj"
        -@erase "$(INTDIR)\smartall.obj"
        -@erase "$(INTDIR)\status.obj"
        -@erase "$(INTDIR)\StdAfx.obj"
        -@erase "$(INTDIR)\tls.obj"
        -@erase "$(INTDIR)\tree.obj"
        -@erase "$(INTDIR)\util.obj"
        -@erase "$(INTDIR)\var.obj"
        -@erase "$(INTDIR)\vc60.idb"
        -@erase "$(INTDIR)\verify.obj"
        -@erase "$(INTDIR)\verify_vol.obj"
        -@erase "$(INTDIR)\vss.obj"
        -@erase "$(INTDIR)\vss_xp.obj"
        -@erase "$(INTDIR)\vss_w2k3.obj"
        -@erase "$(INTDIR)\watchdog.obj"
        -@erase "$(INTDIR)\winabout.obj"
        -@erase "$(INTDIR)\winapi.obj"
        -@erase "$(INTDIR)\winevents.obj"
        -@erase "$(INTDIR)\winmain.obj"
        -@erase "$(INTDIR)\winres.res"
        -@erase "$(INTDIR)\winservice.obj"
        -@erase "$(INTDIR)\winstat.obj"
        -@erase "$(INTDIR)\wintray.obj"
        -@erase "$(INTDIR)\workq.obj"
        -@erase "$(OUTDIR)\bacula-fd.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "../compat" /I "../.." /I "../../../../depkgs-win32/pthreads" /I "../../../../depkgs-win32/zlib" /I "." /D "_WINDOWS" /D "_WIN32_WINNT=0x500" /D "WIN32_VSS" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_WIN32" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\winres.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\baculafd.bsc" 
BSC32_SBRS= \
        
LINK32=link.exe
LINK32_FLAGS=ole32.lib oleaut32.lib user32.lib advapi32.lib gdi32.lib wsock32.lib shell32.lib pthreadVCE.lib zlib.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"MSVCRT.lib" /out:"$(OUTDIR)\bacula-fd.exe" /libpath:"../../../../depkgs-win32/pthreads" /libpath:"../../../../depkgs-win32/zlib" 
LINK32_OBJS= \
        "$(INTDIR)\address_conf.obj" \
        "$(INTDIR)\alist.obj" \
        "$(INTDIR)\alloc.obj" \
        "$(INTDIR)\attr.obj" \
        "$(INTDIR)\attribs.obj" \
        "$(INTDIR)\authenticate.obj" \
        "$(INTDIR)\backup.obj" \
        "$(INTDIR)\base64.obj" \
        "$(INTDIR)\berrno.obj" \
        "$(INTDIR)\bfile.obj" \
        "$(INTDIR)\bget_msg.obj" \
        "$(INTDIR)\bnet.obj" \
        "$(INTDIR)\bnet_server.obj" \
        "$(INTDIR)\bshm.obj" \
        "$(INTDIR)\bsys.obj" \
        "$(INTDIR)\btime.obj" \
        "$(INTDIR)\btimers.obj" \
        "$(INTDIR)\chksum.obj" \
        "$(INTDIR)\compat.obj" \
        "$(INTDIR)\cram-md5.obj" \
        "$(INTDIR)\crc32.obj" \
        "$(INTDIR)\create_file.obj" \
        "$(INTDIR)\daemon.obj" \
        "$(INTDIR)\dlist.obj" \
        "$(INTDIR)\edit.obj" \
        "$(INTDIR)\enable_priv.obj" \
        "$(INTDIR)\estimate.obj" \
        "$(INTDIR)\filed.obj" \
        "$(INTDIR)\filed_conf.obj" \
        "$(INTDIR)\find.obj" \
        "$(INTDIR)\find_one.obj" \
        "$(INTDIR)\fnmatch.obj" \
        "$(INTDIR)\fstype.obj" \
        "$(INTDIR)\getopt.obj" \
        "$(INTDIR)\heartbeat.obj" \
        "$(INTDIR)\hmac.obj" \
        "$(INTDIR)\htable.obj" \
        "$(INTDIR)\idcache.obj" \
        "$(INTDIR)\jcr.obj" \
        "$(INTDIR)\job.obj" \
        "$(INTDIR)\lex.obj" \
        "$(INTDIR)\makepath.obj" \
        "$(INTDIR)\match.obj" \
        "$(INTDIR)\md5.obj" \
        "$(INTDIR)\mem_pool.obj" \
        "$(INTDIR)\message.obj" \
        "$(INTDIR)\parse_conf.obj" \
        "$(INTDIR)\print.obj" \
        "$(INTDIR)\pythonlib.obj" \
        "$(INTDIR)\queue.obj" \
        "$(INTDIR)\bregex.obj" \
        "$(INTDIR)\restore.obj" \
        "$(INTDIR)\res.obj" \
        "$(INTDIR)\rwlock.obj" \
        "$(INTDIR)\save-cwd.obj" \
        "$(INTDIR)\scan.obj" \
        "$(INTDIR)\semlock.obj" \
        "$(INTDIR)\serial.obj" \
        "$(INTDIR)\sha1.obj" \
        "$(INTDIR)\signal.obj" \
        "$(INTDIR)\smartall.obj" \
        "$(INTDIR)\status.obj" \
        "$(INTDIR)\StdAfx.obj" \
        "$(INTDIR)\tls.obj" \
        "$(INTDIR)\tree.obj" \
        "$(INTDIR)\util.obj" \
        "$(INTDIR)\var.obj" \
        "$(INTDIR)\verify.obj" \
        "$(INTDIR)\verify_vol.obj" \
        "$(INTDIR)\vss.obj" \
        "$(INTDIR)\vss_xp.obj" \
        "$(INTDIR)\vss_w2k3.obj" \
        "$(INTDIR)\watchdog.obj" \
        "$(INTDIR)\winabout.obj" \
        "$(INTDIR)\winapi.obj" \
        "$(INTDIR)\winevents.obj" \
        "$(INTDIR)\winmain.obj" \
        "$(INTDIR)\winservice.obj" \
        "$(INTDIR)\winstat.obj" \
        "$(INTDIR)\wintray.obj" \
        "$(INTDIR)\workq.obj" \
        "$(INTDIR)\winres.res"

"$(OUTDIR)\bacula-fd.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\bacula-fd.exe" "$(OUTDIR)\baculafd.bsc"


CLEAN :
        -@erase "$(INTDIR)\address_conf.obj"
        -@erase "$(INTDIR)\address_conf.sbr"
        -@erase "$(INTDIR)\alist.obj"
        -@erase "$(INTDIR)\alist.sbr"
        -@erase "$(INTDIR)\alloc.obj"
        -@erase "$(INTDIR)\alloc.sbr"
        -@erase "$(INTDIR)\attr.obj"
        -@erase "$(INTDIR)\attr.sbr"
        -@erase "$(INTDIR)\attribs.obj"
        -@erase "$(INTDIR)\attribs.sbr"
        -@erase "$(INTDIR)\authenticate.obj"
        -@erase "$(INTDIR)\authenticate.sbr"
        -@erase "$(INTDIR)\backup.obj"
        -@erase "$(INTDIR)\backup.sbr"
        -@erase "$(INTDIR)\base64.obj"
        -@erase "$(INTDIR)\base64.sbr"
        -@erase "$(INTDIR)\berrno.obj"
        -@erase "$(INTDIR)\berrno.sbr"
        -@erase "$(INTDIR)\bfile.obj"
        -@erase "$(INTDIR)\bfile.sbr"
        -@erase "$(INTDIR)\bget_msg.obj"
        -@erase "$(INTDIR)\bget_msg.sbr"
        -@erase "$(INTDIR)\bnet.obj"
        -@erase "$(INTDIR)\bnet.sbr"
        -@erase "$(INTDIR)\bnet_server.obj"
        -@erase "$(INTDIR)\bnet_server.sbr"
        -@erase "$(INTDIR)\bshm.obj"
        -@erase "$(INTDIR)\bshm.sbr"
        -@erase "$(INTDIR)\bsys.obj"
        -@erase "$(INTDIR)\bsys.sbr"
        -@erase "$(INTDIR)\btime.obj"
        -@erase "$(INTDIR)\btime.sbr"
        -@erase "$(INTDIR)\btimers.obj"
        -@erase "$(INTDIR)\btimers.sbr"
        -@erase "$(INTDIR)\chksum.obj"
        -@erase "$(INTDIR)\chksum.sbr"
        -@erase "$(INTDIR)\compat.obj"
        -@erase "$(INTDIR)\compat.sbr"
        -@erase "$(INTDIR)\cram-md5.obj"
        -@erase "$(INTDIR)\cram-md5.sbr"
        -@erase "$(INTDIR)\crc32.obj"
        -@erase "$(INTDIR)\crc32.sbr"
        -@erase "$(INTDIR)\create_file.obj"
        -@erase "$(INTDIR)\create_file.sbr"
        -@erase "$(INTDIR)\daemon.obj"
        -@erase "$(INTDIR)\daemon.sbr"
        -@erase "$(INTDIR)\dlist.obj"
        -@erase "$(INTDIR)\dlist.sbr"
        -@erase "$(INTDIR)\edit.obj"
        -@erase "$(INTDIR)\edit.sbr"
        -@erase "$(INTDIR)\enable_priv.obj"
        -@erase "$(INTDIR)\enable_priv.sbr"
        -@erase "$(INTDIR)\estimate.obj"
        -@erase "$(INTDIR)\estimate.sbr"
        -@erase "$(INTDIR)\filed.obj"
        -@erase "$(INTDIR)\filed.sbr"
        -@erase "$(INTDIR)\filed_conf.obj"
        -@erase "$(INTDIR)\filed_conf.sbr"
        -@erase "$(INTDIR)\find.obj"
        -@erase "$(INTDIR)\find.sbr"
        -@erase "$(INTDIR)\find_one.obj"
        -@erase "$(INTDIR)\find_one.sbr"
        -@erase "$(INTDIR)\fnmatch.obj"
        -@erase "$(INTDIR)\fnmatch.sbr"
        -@erase "$(INTDIR)\fstype.obj"
        -@erase "$(INTDIR)\fstype.sbr"
        -@erase "$(INTDIR)\getopt.obj"
        -@erase "$(INTDIR)\getopt.sbr"
        -@erase "$(INTDIR)\heartbeat.obj"
        -@erase "$(INTDIR)\heartbeat.sbr"
        -@erase "$(INTDIR)\hmac.obj"
        -@erase "$(INTDIR)\hmac.sbr"
        -@erase "$(INTDIR)\htable.obj"
        -@erase "$(INTDIR)\htable.sbr"
        -@erase "$(INTDIR)\idcache.obj"
        -@erase "$(INTDIR)\idcache.sbr"
        -@erase "$(INTDIR)\jcr.obj"
        -@erase "$(INTDIR)\jcr.sbr"
        -@erase "$(INTDIR)\job.obj"
        -@erase "$(INTDIR)\job.sbr"
        -@erase "$(INTDIR)\lex.obj"
        -@erase "$(INTDIR)\lex.sbr"
        -@erase "$(INTDIR)\makepath.obj"
        -@erase "$(INTDIR)\makepath.sbr"
        -@erase "$(INTDIR)\match.obj"
        -@erase "$(INTDIR)\match.sbr"
        -@erase "$(INTDIR)\md5.obj"
        -@erase "$(INTDIR)\md5.sbr"
        -@erase "$(INTDIR)\mem_pool.obj"
        -@erase "$(INTDIR)\mem_pool.sbr"
        -@erase "$(INTDIR)\message.obj"
        -@erase "$(INTDIR)\message.sbr"
        -@erase "$(INTDIR)\parse_conf.obj"
        -@erase "$(INTDIR)\parse_conf.sbr"
        -@erase "$(INTDIR)\print.obj"
        -@erase "$(INTDIR)\print.sbr"
        -@erase "$(INTDIR)\pythonlib.obj"
        -@erase "$(INTDIR)\pythonlib.sbr"
        -@erase "$(INTDIR)\queue.obj"
        -@erase "$(INTDIR)\queue.sbr"
        -@erase "$(INTDIR)\bregex.obj"
        -@erase "$(INTDIR)\bregex.sbr"
        -@erase "$(INTDIR)\restore.obj"
        -@erase "$(INTDIR)\restore.sbr"
        -@erase "$(INTDIR)\res.obj"
        -@erase "$(INTDIR)\res.sbr"
        -@erase "$(INTDIR)\rwlock.obj"
        -@erase "$(INTDIR)\rwlock.sbr"
        -@erase "$(INTDIR)\save-cwd.obj"
        -@erase "$(INTDIR)\save-cwd.sbr"
        -@erase "$(INTDIR)\scan.obj"
        -@erase "$(INTDIR)\scan.sbr"
        -@erase "$(INTDIR)\semlock.obj"
        -@erase "$(INTDIR)\semlock.sbr"
        -@erase "$(INTDIR)\serial.obj"
        -@erase "$(INTDIR)\serial.sbr"
        -@erase "$(INTDIR)\sha1.obj"
        -@erase "$(INTDIR)\sha1.sbr"
        -@erase "$(INTDIR)\signal.obj"
        -@erase "$(INTDIR)\signal.sbr"
        -@erase "$(INTDIR)\smartall.obj"
        -@erase "$(INTDIR)\smartall.sbr"
        -@erase "$(INTDIR)\status.obj"
        -@erase "$(INTDIR)\status.sbr"
        -@erase "$(INTDIR)\StdAfx.obj"
        -@erase "$(INTDIR)\StdAfx.sbr"
        -@erase "$(INTDIR)\tls.obj"
        -@erase "$(INTDIR)\tls.sbr"
        -@erase "$(INTDIR)\tree.obj"
        -@erase "$(INTDIR)\tree.sbr"
        -@erase "$(INTDIR)\util.obj"
        -@erase "$(INTDIR)\util.sbr"
        -@erase "$(INTDIR)\var.obj"
        -@erase "$(INTDIR)\var.sbr"
        -@erase "$(INTDIR)\vc60.idb"
        -@erase "$(INTDIR)\vc60.pdb"
        -@erase "$(INTDIR)\verify.obj"
        -@erase "$(INTDIR)\verify.sbr"
        -@erase "$(INTDIR)\verify_vol.obj"
        -@erase "$(INTDIR)\verify_vol.sbr"
        -@erase "$(INTDIR)\vss.obj"
        -@erase "$(INTDIR)\vss.sbr"
        -@erase "$(INTDIR)\vss_xp.obj"
        -@erase "$(INTDIR)\vss_xp.sbr"
        -@erase "$(INTDIR)\vss_w2k3.obj"
        -@erase "$(INTDIR)\vss_w2k3.sbr"
        -@erase "$(INTDIR)\watchdog.obj"
        -@erase "$(INTDIR)\watchdog.sbr"
        -@erase "$(INTDIR)\winabout.obj"
        -@erase "$(INTDIR)\winabout.sbr"
        -@erase "$(INTDIR)\winapi.obj"
        -@erase "$(INTDIR)\winapi.sbr"
        -@erase "$(INTDIR)\winevents.obj"
        -@erase "$(INTDIR)\winevents.sbr"
        -@erase "$(INTDIR)\winmain.obj"
        -@erase "$(INTDIR)\winmain.sbr"
        -@erase "$(INTDIR)\winres.res"
        -@erase "$(INTDIR)\winservice.obj"
        -@erase "$(INTDIR)\winservice.sbr"
        -@erase "$(INTDIR)\winstat.obj"
        -@erase "$(INTDIR)\winstat.sbr"
        -@erase "$(INTDIR)\wintray.obj"
        -@erase "$(INTDIR)\wintray.sbr"
        -@erase "$(INTDIR)\workq.obj"
        -@erase "$(INTDIR)\workq.sbr"
        -@erase "$(OUTDIR)\bacula-fd.exe"
        -@erase "$(OUTDIR)\baculafd.bsc"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../compat" /I "../.." /I "../../../../depkgs-win32/pthreads" /I "../../../../depkgs-win32/zlib" /I "." /D "_WINDOWS" /D "_WIN32_WINNT=0x500" /D "WIN32_VSS" /D "_DEBUG" /D "_WINMAIN_" /D "PTW32_BUILD" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_WIN32" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\winres.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\baculafd.bsc" 
BSC32_SBRS= \
        "$(INTDIR)\address_conf.sbr" \
        "$(INTDIR)\alist.sbr" \
        "$(INTDIR)\alloc.sbr" \
        "$(INTDIR)\attr.sbr" \
        "$(INTDIR)\attribs.sbr" \
        "$(INTDIR)\authenticate.sbr" \
        "$(INTDIR)\backup.sbr" \
        "$(INTDIR)\base64.sbr" \
        "$(INTDIR)\berrno.sbr" \
        "$(INTDIR)\bfile.sbr" \
        "$(INTDIR)\bget_msg.sbr" \
        "$(INTDIR)\bnet.sbr" \
        "$(INTDIR)\bnet_server.sbr" \
        "$(INTDIR)\bshm.sbr" \
        "$(INTDIR)\bsys.sbr" \
        "$(INTDIR)\btime.sbr" \
        "$(INTDIR)\btimers.sbr" \
        "$(INTDIR)\chksum.sbr" \
        "$(INTDIR)\compat.sbr" \
        "$(INTDIR)\cram-md5.sbr" \
        "$(INTDIR)\crc32.sbr" \
        "$(INTDIR)\create_file.sbr" \
        "$(INTDIR)\daemon.sbr" \
        "$(INTDIR)\dlist.sbr" \
        "$(INTDIR)\edit.sbr" \
        "$(INTDIR)\enable_priv.sbr" \
        "$(INTDIR)\estimate.sbr" \
        "$(INTDIR)\filed.sbr" \
        "$(INTDIR)\filed_conf.sbr" \
        "$(INTDIR)\find.sbr" \
        "$(INTDIR)\find_one.sbr" \
        "$(INTDIR)\fnmatch.sbr" \
        "$(INTDIR)\fstype.sbr" \
        "$(INTDIR)\getopt.sbr" \
        "$(INTDIR)\heartbeat.sbr" \
        "$(INTDIR)\hmac.sbr" \
        "$(INTDIR)\htable.sbr" \
        "$(INTDIR)\idcache.sbr" \
        "$(INTDIR)\jcr.sbr" \
        "$(INTDIR)\job.sbr" \
        "$(INTDIR)\lex.sbr" \
        "$(INTDIR)\makepath.sbr" \
        "$(INTDIR)\match.sbr" \
        "$(INTDIR)\md5.sbr" \
        "$(INTDIR)\mem_pool.sbr" \
        "$(INTDIR)\message.sbr" \
        "$(INTDIR)\parse_conf.sbr" \
        "$(INTDIR)\print.sbr" \
        "$(INTDIR)\pythonlib.sbr" \
        "$(INTDIR)\queue.sbr" \
        "$(INTDIR)\bregex.sbr" \
        "$(INTDIR)\restore.sbr" \
        "$(INTDIR)\res.sbr" \
        "$(INTDIR)\rwlock.sbr" \
        "$(INTDIR)\save-cwd.sbr" \
        "$(INTDIR)\scan.sbr" \
        "$(INTDIR)\semlock.sbr" \
        "$(INTDIR)\serial.sbr" \
        "$(INTDIR)\sha1.sbr" \
        "$(INTDIR)\signal.sbr" \
        "$(INTDIR)\smartall.sbr" \
        "$(INTDIR)\status.sbr" \
        "$(INTDIR)\StdAfx.sbr" \
        "$(INTDIR)\tls.sbr" \
        "$(INTDIR)\tree.sbr" \
        "$(INTDIR)\util.sbr" \
        "$(INTDIR)\var.sbr" \
        "$(INTDIR)\verify.sbr" \
        "$(INTDIR)\verify_vol.sbr" \
        "$(INTDIR)\vss.sbr" \
        "$(INTDIR)\vss_xp.sbr" \
        "$(INTDIR)\vss_w2k3.sbr" \
        "$(INTDIR)\watchdog.sbr" \
        "$(INTDIR)\winabout.sbr" \
        "$(INTDIR)\winapi.sbr" \
        "$(INTDIR)\winevents.sbr" \
        "$(INTDIR)\winmain.sbr" \
        "$(INTDIR)\winservice.sbr" \
        "$(INTDIR)\winstat.sbr" \
        "$(INTDIR)\wintray.sbr" \
        "$(INTDIR)\workq.sbr"

"$(OUTDIR)\baculafd.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=ole32.lib oleaut32.lib user32.lib advapi32.lib gdi32.lib shell32.lib wsock32.lib pthreadVCE.lib zlib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"MSVCRT.lib" /out:"$(OUTDIR)\bacula-fd.exe" /libpath:"../../../../depkgs-win32/pthreads" /libpath:"../../../../depkgs-win32/zlib" 
LINK32_OBJS= \
        "$(INTDIR)\address_conf.obj" \
        "$(INTDIR)\alist.obj" \
        "$(INTDIR)\alloc.obj" \
        "$(INTDIR)\attr.obj" \
        "$(INTDIR)\attribs.obj" \
        "$(INTDIR)\authenticate.obj" \
        "$(INTDIR)\backup.obj" \
        "$(INTDIR)\base64.obj" \
        "$(INTDIR)\berrno.obj" \
        "$(INTDIR)\bfile.obj" \
        "$(INTDIR)\bget_msg.obj" \
        "$(INTDIR)\bnet.obj" \
        "$(INTDIR)\bnet_server.obj" \
        "$(INTDIR)\bshm.obj" \
        "$(INTDIR)\bsys.obj" \
        "$(INTDIR)\btime.obj" \
        "$(INTDIR)\btimers.obj" \
        "$(INTDIR)\chksum.obj" \
        "$(INTDIR)\compat.obj" \
        "$(INTDIR)\cram-md5.obj" \
        "$(INTDIR)\crc32.obj" \
        "$(INTDIR)\create_file.obj" \
        "$(INTDIR)\daemon.obj" \
        "$(INTDIR)\dlist.obj" \
        "$(INTDIR)\edit.obj" \
        "$(INTDIR)\enable_priv.obj" \
        "$(INTDIR)\estimate.obj" \
        "$(INTDIR)\filed.obj" \
        "$(INTDIR)\filed_conf.obj" \
        "$(INTDIR)\find.obj" \
        "$(INTDIR)\find_one.obj" \
        "$(INTDIR)\fnmatch.obj" \
        "$(INTDIR)\fstype.obj" \
        "$(INTDIR)\getopt.obj" \
        "$(INTDIR)\heartbeat.obj" \
        "$(INTDIR)\hmac.obj" \
        "$(INTDIR)\htable.obj" \
        "$(INTDIR)\idcache.obj" \
        "$(INTDIR)\jcr.obj" \
        "$(INTDIR)\job.obj" \
        "$(INTDIR)\lex.obj" \
        "$(INTDIR)\makepath.obj" \
        "$(INTDIR)\match.obj" \
        "$(INTDIR)\md5.obj" \
        "$(INTDIR)\mem_pool.obj" \
        "$(INTDIR)\message.obj" \
        "$(INTDIR)\parse_conf.obj" \
        "$(INTDIR)\print.obj" \
        "$(INTDIR)\pythonlib.obj" \
        "$(INTDIR)\queue.obj" \
        "$(INTDIR)\bregex.obj" \
        "$(INTDIR)\restore.obj" \
        "$(INTDIR)\res.obj" \
        "$(INTDIR)\rwlock.obj" \
        "$(INTDIR)\save-cwd.obj" \
        "$(INTDIR)\scan.obj" \
        "$(INTDIR)\semlock.obj" \
        "$(INTDIR)\serial.obj" \
        "$(INTDIR)\sha1.obj" \
        "$(INTDIR)\signal.obj" \
        "$(INTDIR)\smartall.obj" \
        "$(INTDIR)\status.obj" \
        "$(INTDIR)\StdAfx.obj" \
        "$(INTDIR)\tls.obj" \
        "$(INTDIR)\tree.obj" \
        "$(INTDIR)\util.obj" \
        "$(INTDIR)\var.obj" \
        "$(INTDIR)\verify.obj" \
        "$(INTDIR)\verify_vol.obj" \
        "$(INTDIR)\vss.obj" \
        "$(INTDIR)\vss_xp.obj" \
        "$(INTDIR)\vss_w2k3.obj" \
        "$(INTDIR)\watchdog.obj" \
        "$(INTDIR)\winabout.obj" \
        "$(INTDIR)\winapi.obj" \
        "$(INTDIR)\winevents.obj" \
        "$(INTDIR)\winmain.obj" \
        "$(INTDIR)\winservice.obj" \
        "$(INTDIR)\winstat.obj" \
        "$(INTDIR)\wintray.obj" \
        "$(INTDIR)\workq.obj" \
        "$(INTDIR)\winres.res"

"$(OUTDIR)\bacula-fd.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("baculafd.dep")
!INCLUDE "baculafd.dep"
!ELSE 
!MESSAGE Warning: cannot find "baculafd.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "baculafd - Win32 Release" || "$(CFG)" == "baculafd - Win32 Debug"

SOURCE=..\lib\address_conf.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\address_conf.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\address_conf.obj"   "$(INTDIR)\address_conf.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\alist.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\alist.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\alist.obj"   "$(INTDIR)\alist.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


SOURCE=..\lib\alloc.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\alloc.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\alloc.obj"   "$(INTDIR)\alloc.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\attr.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\attr.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\attr.obj"    "$(INTDIR)\attr.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\findlib\attribs.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\attribs.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\attribs.obj" "$(INTDIR)\attribs.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\filed\authenticate.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\authenticate.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\authenticate.obj"    "$(INTDIR)\authenticate.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\filed\backup.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\backup.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\backup.obj"  "$(INTDIR)\backup.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\base64.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\base64.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\base64.obj"  "$(INTDIR)\base64.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\berrno.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\berrno.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\berrno.obj"  "$(INTDIR)\berrno.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


SOURCE=..\findlib\bfile.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\bfile.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\bfile.obj"   "$(INTDIR)\bfile.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\bget_msg.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\bget_msg.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\bget_msg.obj"        "$(INTDIR)\bget_msg.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\bnet.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\bnet.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\bnet.obj"    "$(INTDIR)\bnet.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\bnet_server.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\bnet_server.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\bnet_server.obj"     "$(INTDIR)\bnet_server.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\bshm.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\bshm.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\bshm.obj"    "$(INTDIR)\bshm.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\bsys.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\bsys.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\bsys.obj"    "$(INTDIR)\bsys.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\btime.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\btime.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\btime.obj"   "$(INTDIR)\btime.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\btimers.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\btimers.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\btimers.obj" "$(INTDIR)\btimers.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


SOURCE=..\filed\chksum.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\chksum.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\chksum.obj" "$(INTDIR)\chksum.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


SOURCE=..\compat\compat.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\compat.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\compat.obj"  "$(INTDIR)\compat.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE="..\lib\cram-md5.cpp"

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\cram-md5.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\cram-md5.obj"        "$(INTDIR)\cram-md5.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\crc32.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\crc32.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\crc32.obj"   "$(INTDIR)\crc32.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\findlib\create_file.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\create_file.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\create_file.obj"     "$(INTDIR)\create_file.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\daemon.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\daemon.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\daemon.obj"  "$(INTDIR)\daemon.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\dlist.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\dlist.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\dlist.obj"   "$(INTDIR)\dlist.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\edit.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\edit.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\edit.obj"    "$(INTDIR)\edit.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\findlib\enable_priv.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\enable_priv.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\enable_priv.obj"     "$(INTDIR)\enable_priv.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\filed\estimate.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\estimate.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\estimate.obj"        "$(INTDIR)\estimate.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\filed\filed.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\filed.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\filed.obj"   "$(INTDIR)\filed.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\filed\filed_conf.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\filed_conf.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\filed_conf.obj"      "$(INTDIR)\filed_conf.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\findlib\find.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\find.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\find.obj"    "$(INTDIR)\find.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\findlib\find_one.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\find_one.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\find_one.obj"        "$(INTDIR)\find_one.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\fnmatch.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\fnmatch.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\fnmatch.obj" "$(INTDIR)\fnmatch.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\findlib\fstype.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\fstype.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\fstype.obj" "$(INTDIR)\fstype.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


SOURCE=..\compat\getopt.c

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\getopt.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\getopt.obj"  "$(INTDIR)\getopt.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\filed\heartbeat.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\heartbeat.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\heartbeat.obj"       "$(INTDIR)\heartbeat.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\hmac.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\hmac.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\hmac.obj"    "$(INTDIR)\hmac.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\htable.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\htable.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\htable.obj"  "$(INTDIR)\htable.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\idcache.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\idcache.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\idcache.obj" "$(INTDIR)\idcache.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\jcr.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\jcr.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\jcr.obj"     "$(INTDIR)\jcr.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\filed\job.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\job.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\job.obj"     "$(INTDIR)\job.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\lex.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\lex.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\lex.obj"     "$(INTDIR)\lex.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\findlib\makepath.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\makepath.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\makepath.obj"        "$(INTDIR)\makepath.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\findlib\match.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\match.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\match.obj"   "$(INTDIR)\match.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\md5.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\md5.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\md5.obj"     "$(INTDIR)\md5.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\mem_pool.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\mem_pool.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\mem_pool.obj"        "$(INTDIR)\mem_pool.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\message.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\message.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\message.obj" "$(INTDIR)\message.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\parse_conf.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\parse_conf.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\parse_conf.obj"      "$(INTDIR)\parse_conf.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\compat\print.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\print.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\print.obj"   "$(INTDIR)\print.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\pythonlib.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\pythonlib.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\pythonlib.obj"   "$(INTDIR)\pythonlib.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


SOURCE=..\lib\queue.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\queue.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\queue.obj"   "$(INTDIR)\queue.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\bregex.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\bregex.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\bregex.obj"   "$(INTDIR)\bregex.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


SOURCE=..\filed\restore.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\restore.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\restore.obj" "$(INTDIR)\restore.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\res.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\res.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\res.obj" "$(INTDIR)\res.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


SOURCE=..\lib\rwlock.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\rwlock.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\rwlock.obj"  "$(INTDIR)\rwlock.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE="..\findlib\save-cwd.cpp"

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\save-cwd.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\save-cwd.obj"        "$(INTDIR)\save-cwd.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\scan.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\scan.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\scan.obj"    "$(INTDIR)\scan.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\semlock.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\semlock.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\semlock.obj" "$(INTDIR)\semlock.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\serial.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\serial.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\serial.obj"  "$(INTDIR)\serial.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\sha1.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\sha1.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\sha1.obj"    "$(INTDIR)\sha1.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\signal.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\signal.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\signal.obj"  "$(INTDIR)\signal.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\smartall.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\smartall.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\smartall.obj"        "$(INTDIR)\smartall.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\filed\status.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\status.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\status.obj"  "$(INTDIR)\status.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\StdAfx.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\StdAfx.obj"  "$(INTDIR)\StdAfx.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=..\lib\tls.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\tls.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\tls.obj"     "$(INTDIR)\tls.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


SOURCE=..\lib\tree.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\tree.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\tree.obj"    "$(INTDIR)\tree.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\util.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\util.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\util.obj"    "$(INTDIR)\util.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


SOURCE=..\lib\var.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\var.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\var.obj"     "$(INTDIR)\var.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 



SOURCE=..\filed\verify.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\verify.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\verify.obj"  "$(INTDIR)\verify.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\filed\verify_vol.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\verify_vol.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\verify_vol.obj"      "$(INTDIR)\verify_vol.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\compat\vss.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\vss.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\vss.obj"      "$(INTDIR)\vss.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\compat\vss_xp.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\vss_xp.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\vss_xp.obj"      "$(INTDIR)\vss_xp.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\compat\vss_w2k3.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\vss_w2k3.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\vss_w2k3.obj"      "$(INTDIR)\vss_w2k3.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 




SOURCE=..\lib\watchdog.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\watchdog.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\watchdog.obj"        "$(INTDIR)\watchdog.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\filed\win32\winabout.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\winabout.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\winabout.obj"        "$(INTDIR)\winabout.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\winapi.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\winapi.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\winapi.obj"  "$(INTDIR)\winapi.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\filed\win32\winevents.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\winevents.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\winevents.obj"       "$(INTDIR)\winevents.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\filed\win32\winmain.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\winmain.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\winmain.obj" "$(INTDIR)\winmain.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\filed\win32\winres.rc

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\winres.res" : $(SOURCE) "$(INTDIR)"
        $(RSC) /l 0x409 /fo"$(INTDIR)\winres.res" /i "..\..\filed\win32" /d "NDEBUG" $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\winres.res" : $(SOURCE) "$(INTDIR)"
        $(RSC) /l 0x409 /fo"$(INTDIR)\winres.res" /i "..\..\filed\win32" /d "_DEBUG" $(SOURCE)


!ENDIF 

SOURCE=..\..\filed\win32\winservice.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\winservice.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\winservice.obj"      "$(INTDIR)\winservice.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\filed\win32\winstat.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\winstat.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\winstat.obj" "$(INTDIR)\winstat.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\filed\win32\wintray.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\wintray.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\wintray.obj" "$(INTDIR)\wintray.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib\workq.cpp

!IF  "$(CFG)" == "baculafd - Win32 Release"


"$(INTDIR)\workq.obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "baculafd - Win32 Debug"


"$(INTDIR)\workq.obj"   "$(INTDIR)\workq.sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


!ENDIF 
