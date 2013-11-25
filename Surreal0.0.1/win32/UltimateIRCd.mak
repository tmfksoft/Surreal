# Microsoft Developer Studio Generated NMAKE File, Based on UltimateIRCd.dsp
!IF "$(CFG)" == ""
CFG=UltimateIRCd - Win32 Debug
!MESSAGE No configuration specified. Defaulting to UltimateIRCd - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "UltimateIRCd - Win32 Release" && "$(CFG)" != "UltimateIRCd - Win32 Debug" && "$(CFG)" != "UltimateIRCd - Win32 Debug SSL" && "$(CFG)" != "UltimateIRCd - Win32 Release SSL"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "UltimateIRCd.mak" CFG="UltimateIRCd - Win32 Debug SSL"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "UltimateIRCd - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "UltimateIRCd - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "UltimateIRCd - Win32 Debug SSL" (based on "Win32 (x86) Console Application")
!MESSAGE "UltimateIRCd - Win32 Release SSL" (based on "Win32 (x86) Console Application")
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

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\UltimateIRCd.exe"


CLEAN :
	-@erase "$(INTDIR)\adler32.obj"
	-@erase "$(INTDIR)\blalloc.obj"
	-@erase "$(INTDIR)\bsd.obj"
	-@erase "$(INTDIR)\channel.obj"
	-@erase "$(INTDIR)\class.obj"
	-@erase "$(INTDIR)\clientlist.obj"
	-@erase "$(INTDIR)\compress.obj"
	-@erase "$(INTDIR)\crc32.obj"
	-@erase "$(INTDIR)\dbuf.obj"
	-@erase "$(INTDIR)\deflate.obj"
	-@erase "$(INTDIR)\dich_conf.obj"
	-@erase "$(INTDIR)\drone.obj"
	-@erase "$(INTDIR)\dynconf.obj"
	-@erase "$(INTDIR)\fdlist.obj"
	-@erase "$(INTDIR)\fds.obj"
	-@erase "$(INTDIR)\gzio.obj"
	-@erase "$(INTDIR)\hash.obj"
	-@erase "$(INTDIR)\inet_addr.obj"
	-@erase "$(INTDIR)\inet_ntop.obj"
	-@erase "$(INTDIR)\inet_pton.obj"
	-@erase "$(INTDIR)\infback.obj"
	-@erase "$(INTDIR)\inffast.obj"
	-@erase "$(INTDIR)\inflate.obj"
	-@erase "$(INTDIR)\inftrees.obj"
	-@erase "$(INTDIR)\ircd.obj"
	-@erase "$(INTDIR)\ircsprintf.obj"
	-@erase "$(INTDIR)\list.obj"
	-@erase "$(INTDIR)\m_nick.obj"
	-@erase "$(INTDIR)\m_who.obj"
	-@erase "$(INTDIR)\match.obj"
	-@erase "$(INTDIR)\packet.obj"
	-@erase "$(INTDIR)\parse.obj"
	-@erase "$(INTDIR)\rc4.obj"
	-@erase "$(INTDIR)\res.obj"
	-@erase "$(INTDIR)\res_comp.obj"
	-@erase "$(INTDIR)\res_init.obj"
	-@erase "$(INTDIR)\res_mkquery.obj"
	-@erase "$(INTDIR)\res_win32.obj"
	-@erase "$(INTDIR)\s_auth.obj"
	-@erase "$(INTDIR)\s_bsd.obj"
	-@erase "$(INTDIR)\s_conf.obj"
	-@erase "$(INTDIR)\s_debug.obj"
	-@erase "$(INTDIR)\s_err.obj"
	-@erase "$(INTDIR)\s_help.obj"
	-@erase "$(INTDIR)\s_hidehost.obj"
	-@erase "$(INTDIR)\s_misc.obj"
	-@erase "$(INTDIR)\s_numeric.obj"
	-@erase "$(INTDIR)\s_serv.obj"
	-@erase "$(INTDIR)\s_services.obj"
	-@erase "$(INTDIR)\s_ultimate.obj"
	-@erase "$(INTDIR)\s_user.obj"
	-@erase "$(INTDIR)\scache.obj"
	-@erase "$(INTDIR)\send.obj"
	-@erase "$(INTDIR)\socketengine_devpoll.obj"
	-@erase "$(INTDIR)\socketengine_epoll.obj"
	-@erase "$(INTDIR)\socketengine_kqueue.obj"
	-@erase "$(INTDIR)\socketengine_poll.obj"
	-@erase "$(INTDIR)\socketengine_select.obj"
	-@erase "$(INTDIR)\socketengine_winsock.obj"
	-@erase "$(INTDIR)\ssl.obj"
	-@erase "$(INTDIR)\support.obj"
	-@erase "$(INTDIR)\throttle.obj"
	-@erase "$(INTDIR)\trees.obj"
	-@erase "$(INTDIR)\uncompr.obj"
	-@erase "$(INTDIR)\userban.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\whowas.obj"
	-@erase "$(INTDIR)\zlink.obj"
	-@erase "$(INTDIR)\zutil.obj"
	-@erase "$(OUTDIR)\UltimateIRCd.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "..\include" /I "..\zlib" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\UltimateIRCd.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\UltimateIRCd.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\UltimateIRCd.pdb" /machine:I386 /out:"$(OUTDIR)\UltimateIRCd.exe" 
LINK32_OBJS= \
	"$(INTDIR)\blalloc.obj" \
	"$(INTDIR)\bsd.obj" \
	"$(INTDIR)\channel.obj" \
	"$(INTDIR)\class.obj" \
	"$(INTDIR)\clientlist.obj" \
	"$(INTDIR)\dbuf.obj" \
	"$(INTDIR)\dich_conf.obj" \
	"$(INTDIR)\drone.obj" \
	"$(INTDIR)\dynconf.obj" \
	"$(INTDIR)\fdlist.obj" \
	"$(INTDIR)\fds.obj" \
	"$(INTDIR)\hash.obj" \
	"$(INTDIR)\inet_addr.obj" \
	"$(INTDIR)\inet_ntop.obj" \
	"$(INTDIR)\inet_pton.obj" \
	"$(INTDIR)\ircd.obj" \
	"$(INTDIR)\ircsprintf.obj" \
	"$(INTDIR)\list.obj" \
	"$(INTDIR)\m_nick.obj" \
	"$(INTDIR)\m_who.obj" \
	"$(INTDIR)\match.obj" \
	"$(INTDIR)\packet.obj" \
	"$(INTDIR)\parse.obj" \
	"$(INTDIR)\rc4.obj" \
	"$(INTDIR)\res.obj" \
	"$(INTDIR)\res_comp.obj" \
	"$(INTDIR)\res_init.obj" \
	"$(INTDIR)\res_mkquery.obj" \
	"$(INTDIR)\res_win32.obj" \
	"$(INTDIR)\s_auth.obj" \
	"$(INTDIR)\s_bsd.obj" \
	"$(INTDIR)\s_conf.obj" \
	"$(INTDIR)\s_debug.obj" \
	"$(INTDIR)\s_err.obj" \
	"$(INTDIR)\s_help.obj" \
	"$(INTDIR)\s_hidehost.obj" \
	"$(INTDIR)\s_misc.obj" \
	"$(INTDIR)\s_numeric.obj" \
	"$(INTDIR)\s_serv.obj" \
	"$(INTDIR)\s_services.obj" \
	"$(INTDIR)\s_ultimate.obj" \
	"$(INTDIR)\s_user.obj" \
	"$(INTDIR)\scache.obj" \
	"$(INTDIR)\send.obj" \
	"$(INTDIR)\socketengine_devpoll.obj" \
	"$(INTDIR)\socketengine_epoll.obj" \
	"$(INTDIR)\socketengine_kqueue.obj" \
	"$(INTDIR)\socketengine_poll.obj" \
	"$(INTDIR)\socketengine_select.obj" \
	"$(INTDIR)\socketengine_winsock.obj" \
	"$(INTDIR)\ssl.obj" \
	"$(INTDIR)\support.obj" \
	"$(INTDIR)\throttle.obj" \
	"$(INTDIR)\userban.obj" \
	"$(INTDIR)\version.obj" \
	"$(INTDIR)\whowas.obj" \
	"$(INTDIR)\zlink.obj" \
	"$(INTDIR)\adler32.obj" \
	"$(INTDIR)\compress.obj" \
	"$(INTDIR)\crc32.obj" \
	"$(INTDIR)\deflate.obj" \
	"$(INTDIR)\gzio.obj" \
	"$(INTDIR)\infback.obj" \
	"$(INTDIR)\inffast.obj" \
	"$(INTDIR)\inflate.obj" \
	"$(INTDIR)\inftrees.obj" \
	"$(INTDIR)\trees.obj" \
	"$(INTDIR)\uncompr.obj" \
	"$(INTDIR)\zutil.obj"

"$(OUTDIR)\UltimateIRCd.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\UltimateIRCd.exe" "$(OUTDIR)\UltimateIRCd.bsc"


CLEAN :
	-@erase "$(INTDIR)\adler32.obj"
	-@erase "$(INTDIR)\adler32.sbr"
	-@erase "$(INTDIR)\blalloc.obj"
	-@erase "$(INTDIR)\blalloc.sbr"
	-@erase "$(INTDIR)\bsd.obj"
	-@erase "$(INTDIR)\bsd.sbr"
	-@erase "$(INTDIR)\channel.obj"
	-@erase "$(INTDIR)\channel.sbr"
	-@erase "$(INTDIR)\class.obj"
	-@erase "$(INTDIR)\class.sbr"
	-@erase "$(INTDIR)\clientlist.obj"
	-@erase "$(INTDIR)\clientlist.sbr"
	-@erase "$(INTDIR)\compress.obj"
	-@erase "$(INTDIR)\compress.sbr"
	-@erase "$(INTDIR)\crc32.obj"
	-@erase "$(INTDIR)\crc32.sbr"
	-@erase "$(INTDIR)\dbuf.obj"
	-@erase "$(INTDIR)\dbuf.sbr"
	-@erase "$(INTDIR)\deflate.obj"
	-@erase "$(INTDIR)\deflate.sbr"
	-@erase "$(INTDIR)\dich_conf.obj"
	-@erase "$(INTDIR)\dich_conf.sbr"
	-@erase "$(INTDIR)\drone.obj"
	-@erase "$(INTDIR)\drone.sbr"
	-@erase "$(INTDIR)\dynconf.obj"
	-@erase "$(INTDIR)\dynconf.sbr"
	-@erase "$(INTDIR)\fdlist.obj"
	-@erase "$(INTDIR)\fdlist.sbr"
	-@erase "$(INTDIR)\fds.obj"
	-@erase "$(INTDIR)\fds.sbr"
	-@erase "$(INTDIR)\gzio.obj"
	-@erase "$(INTDIR)\gzio.sbr"
	-@erase "$(INTDIR)\hash.obj"
	-@erase "$(INTDIR)\hash.sbr"
	-@erase "$(INTDIR)\inet_addr.obj"
	-@erase "$(INTDIR)\inet_addr.sbr"
	-@erase "$(INTDIR)\inet_ntop.obj"
	-@erase "$(INTDIR)\inet_ntop.sbr"
	-@erase "$(INTDIR)\inet_pton.obj"
	-@erase "$(INTDIR)\inet_pton.sbr"
	-@erase "$(INTDIR)\infback.obj"
	-@erase "$(INTDIR)\infback.sbr"
	-@erase "$(INTDIR)\inffast.obj"
	-@erase "$(INTDIR)\inffast.sbr"
	-@erase "$(INTDIR)\inflate.obj"
	-@erase "$(INTDIR)\inflate.sbr"
	-@erase "$(INTDIR)\inftrees.obj"
	-@erase "$(INTDIR)\inftrees.sbr"
	-@erase "$(INTDIR)\ircd.obj"
	-@erase "$(INTDIR)\ircd.sbr"
	-@erase "$(INTDIR)\ircsprintf.obj"
	-@erase "$(INTDIR)\ircsprintf.sbr"
	-@erase "$(INTDIR)\list.obj"
	-@erase "$(INTDIR)\list.sbr"
	-@erase "$(INTDIR)\m_nick.obj"
	-@erase "$(INTDIR)\m_nick.sbr"
	-@erase "$(INTDIR)\m_who.obj"
	-@erase "$(INTDIR)\m_who.sbr"
	-@erase "$(INTDIR)\match.obj"
	-@erase "$(INTDIR)\match.sbr"
	-@erase "$(INTDIR)\packet.obj"
	-@erase "$(INTDIR)\packet.sbr"
	-@erase "$(INTDIR)\parse.obj"
	-@erase "$(INTDIR)\parse.sbr"
	-@erase "$(INTDIR)\rc4.obj"
	-@erase "$(INTDIR)\rc4.sbr"
	-@erase "$(INTDIR)\res.obj"
	-@erase "$(INTDIR)\res.sbr"
	-@erase "$(INTDIR)\res_comp.obj"
	-@erase "$(INTDIR)\res_comp.sbr"
	-@erase "$(INTDIR)\res_init.obj"
	-@erase "$(INTDIR)\res_init.sbr"
	-@erase "$(INTDIR)\res_mkquery.obj"
	-@erase "$(INTDIR)\res_mkquery.sbr"
	-@erase "$(INTDIR)\res_win32.obj"
	-@erase "$(INTDIR)\res_win32.sbr"
	-@erase "$(INTDIR)\s_auth.obj"
	-@erase "$(INTDIR)\s_auth.sbr"
	-@erase "$(INTDIR)\s_bsd.obj"
	-@erase "$(INTDIR)\s_bsd.sbr"
	-@erase "$(INTDIR)\s_conf.obj"
	-@erase "$(INTDIR)\s_conf.sbr"
	-@erase "$(INTDIR)\s_debug.obj"
	-@erase "$(INTDIR)\s_debug.sbr"
	-@erase "$(INTDIR)\s_err.obj"
	-@erase "$(INTDIR)\s_err.sbr"
	-@erase "$(INTDIR)\s_help.obj"
	-@erase "$(INTDIR)\s_help.sbr"
	-@erase "$(INTDIR)\s_hidehost.obj"
	-@erase "$(INTDIR)\s_hidehost.sbr"
	-@erase "$(INTDIR)\s_misc.obj"
	-@erase "$(INTDIR)\s_misc.sbr"
	-@erase "$(INTDIR)\s_numeric.obj"
	-@erase "$(INTDIR)\s_numeric.sbr"
	-@erase "$(INTDIR)\s_serv.obj"
	-@erase "$(INTDIR)\s_serv.sbr"
	-@erase "$(INTDIR)\s_services.obj"
	-@erase "$(INTDIR)\s_services.sbr"
	-@erase "$(INTDIR)\s_ultimate.obj"
	-@erase "$(INTDIR)\s_ultimate.sbr"
	-@erase "$(INTDIR)\s_user.obj"
	-@erase "$(INTDIR)\s_user.sbr"
	-@erase "$(INTDIR)\scache.obj"
	-@erase "$(INTDIR)\scache.sbr"
	-@erase "$(INTDIR)\send.obj"
	-@erase "$(INTDIR)\send.sbr"
	-@erase "$(INTDIR)\socketengine_devpoll.obj"
	-@erase "$(INTDIR)\socketengine_devpoll.sbr"
	-@erase "$(INTDIR)\socketengine_epoll.obj"
	-@erase "$(INTDIR)\socketengine_epoll.sbr"
	-@erase "$(INTDIR)\socketengine_kqueue.obj"
	-@erase "$(INTDIR)\socketengine_kqueue.sbr"
	-@erase "$(INTDIR)\socketengine_poll.obj"
	-@erase "$(INTDIR)\socketengine_poll.sbr"
	-@erase "$(INTDIR)\socketengine_select.obj"
	-@erase "$(INTDIR)\socketengine_select.sbr"
	-@erase "$(INTDIR)\socketengine_winsock.obj"
	-@erase "$(INTDIR)\socketengine_winsock.sbr"
	-@erase "$(INTDIR)\ssl.obj"
	-@erase "$(INTDIR)\ssl.sbr"
	-@erase "$(INTDIR)\support.obj"
	-@erase "$(INTDIR)\support.sbr"
	-@erase "$(INTDIR)\throttle.obj"
	-@erase "$(INTDIR)\throttle.sbr"
	-@erase "$(INTDIR)\trees.obj"
	-@erase "$(INTDIR)\trees.sbr"
	-@erase "$(INTDIR)\uncompr.obj"
	-@erase "$(INTDIR)\uncompr.sbr"
	-@erase "$(INTDIR)\userban.obj"
	-@erase "$(INTDIR)\userban.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\version.sbr"
	-@erase "$(INTDIR)\whowas.obj"
	-@erase "$(INTDIR)\whowas.sbr"
	-@erase "$(INTDIR)\zlink.obj"
	-@erase "$(INTDIR)\zlink.sbr"
	-@erase "$(INTDIR)\zutil.obj"
	-@erase "$(INTDIR)\zutil.sbr"
	-@erase "$(OUTDIR)\UltimateIRCd.bsc"
	-@erase "$(OUTDIR)\UltimateIRCd.exe"
	-@erase "$(OUTDIR)\UltimateIRCd.ilk"
	-@erase "$(OUTDIR)\UltimateIRCd.map"
	-@erase "$(OUTDIR)\UltimateIRCd.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../include" /I "../zlib" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "DEBUGMODE" /FAcs /Fa"$(INTDIR)\\" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\UltimateIRCd.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\UltimateIRCd.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\blalloc.sbr" \
	"$(INTDIR)\bsd.sbr" \
	"$(INTDIR)\channel.sbr" \
	"$(INTDIR)\class.sbr" \
	"$(INTDIR)\clientlist.sbr" \
	"$(INTDIR)\dbuf.sbr" \
	"$(INTDIR)\dich_conf.sbr" \
	"$(INTDIR)\drone.sbr" \
	"$(INTDIR)\dynconf.sbr" \
	"$(INTDIR)\fdlist.sbr" \
	"$(INTDIR)\fds.sbr" \
	"$(INTDIR)\hash.sbr" \
	"$(INTDIR)\inet_addr.sbr" \
	"$(INTDIR)\inet_ntop.sbr" \
	"$(INTDIR)\inet_pton.sbr" \
	"$(INTDIR)\ircd.sbr" \
	"$(INTDIR)\ircsprintf.sbr" \
	"$(INTDIR)\list.sbr" \
	"$(INTDIR)\m_nick.sbr" \
	"$(INTDIR)\m_who.sbr" \
	"$(INTDIR)\match.sbr" \
	"$(INTDIR)\packet.sbr" \
	"$(INTDIR)\parse.sbr" \
	"$(INTDIR)\rc4.sbr" \
	"$(INTDIR)\res.sbr" \
	"$(INTDIR)\res_comp.sbr" \
	"$(INTDIR)\res_init.sbr" \
	"$(INTDIR)\res_mkquery.sbr" \
	"$(INTDIR)\res_win32.sbr" \
	"$(INTDIR)\s_auth.sbr" \
	"$(INTDIR)\s_bsd.sbr" \
	"$(INTDIR)\s_conf.sbr" \
	"$(INTDIR)\s_debug.sbr" \
	"$(INTDIR)\s_err.sbr" \
	"$(INTDIR)\s_help.sbr" \
	"$(INTDIR)\s_hidehost.sbr" \
	"$(INTDIR)\s_misc.sbr" \
	"$(INTDIR)\s_numeric.sbr" \
	"$(INTDIR)\s_serv.sbr" \
	"$(INTDIR)\s_services.sbr" \
	"$(INTDIR)\s_ultimate.sbr" \
	"$(INTDIR)\s_user.sbr" \
	"$(INTDIR)\scache.sbr" \
	"$(INTDIR)\send.sbr" \
	"$(INTDIR)\socketengine_devpoll.sbr" \
	"$(INTDIR)\socketengine_epoll.sbr" \
	"$(INTDIR)\socketengine_kqueue.sbr" \
	"$(INTDIR)\socketengine_poll.sbr" \
	"$(INTDIR)\socketengine_select.sbr" \
	"$(INTDIR)\socketengine_winsock.sbr" \
	"$(INTDIR)\ssl.sbr" \
	"$(INTDIR)\support.sbr" \
	"$(INTDIR)\throttle.sbr" \
	"$(INTDIR)\userban.sbr" \
	"$(INTDIR)\version.sbr" \
	"$(INTDIR)\whowas.sbr" \
	"$(INTDIR)\zlink.sbr" \
	"$(INTDIR)\adler32.sbr" \
	"$(INTDIR)\compress.sbr" \
	"$(INTDIR)\crc32.sbr" \
	"$(INTDIR)\deflate.sbr" \
	"$(INTDIR)\gzio.sbr" \
	"$(INTDIR)\infback.sbr" \
	"$(INTDIR)\inffast.sbr" \
	"$(INTDIR)\inflate.sbr" \
	"$(INTDIR)\inftrees.sbr" \
	"$(INTDIR)\trees.sbr" \
	"$(INTDIR)\uncompr.sbr" \
	"$(INTDIR)\zutil.sbr"

"$(OUTDIR)\UltimateIRCd.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\UltimateIRCd.pdb" /map:"$(INTDIR)\UltimateIRCd.map" /debug /machine:I386 /out:"$(OUTDIR)\UltimateIRCd.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\blalloc.obj" \
	"$(INTDIR)\bsd.obj" \
	"$(INTDIR)\channel.obj" \
	"$(INTDIR)\class.obj" \
	"$(INTDIR)\clientlist.obj" \
	"$(INTDIR)\dbuf.obj" \
	"$(INTDIR)\dich_conf.obj" \
	"$(INTDIR)\drone.obj" \
	"$(INTDIR)\dynconf.obj" \
	"$(INTDIR)\fdlist.obj" \
	"$(INTDIR)\fds.obj" \
	"$(INTDIR)\hash.obj" \
	"$(INTDIR)\inet_addr.obj" \
	"$(INTDIR)\inet_ntop.obj" \
	"$(INTDIR)\inet_pton.obj" \
	"$(INTDIR)\ircd.obj" \
	"$(INTDIR)\ircsprintf.obj" \
	"$(INTDIR)\list.obj" \
	"$(INTDIR)\m_nick.obj" \
	"$(INTDIR)\m_who.obj" \
	"$(INTDIR)\match.obj" \
	"$(INTDIR)\packet.obj" \
	"$(INTDIR)\parse.obj" \
	"$(INTDIR)\rc4.obj" \
	"$(INTDIR)\res.obj" \
	"$(INTDIR)\res_comp.obj" \
	"$(INTDIR)\res_init.obj" \
	"$(INTDIR)\res_mkquery.obj" \
	"$(INTDIR)\res_win32.obj" \
	"$(INTDIR)\s_auth.obj" \
	"$(INTDIR)\s_bsd.obj" \
	"$(INTDIR)\s_conf.obj" \
	"$(INTDIR)\s_debug.obj" \
	"$(INTDIR)\s_err.obj" \
	"$(INTDIR)\s_help.obj" \
	"$(INTDIR)\s_hidehost.obj" \
	"$(INTDIR)\s_misc.obj" \
	"$(INTDIR)\s_numeric.obj" \
	"$(INTDIR)\s_serv.obj" \
	"$(INTDIR)\s_services.obj" \
	"$(INTDIR)\s_ultimate.obj" \
	"$(INTDIR)\s_user.obj" \
	"$(INTDIR)\scache.obj" \
	"$(INTDIR)\send.obj" \
	"$(INTDIR)\socketengine_devpoll.obj" \
	"$(INTDIR)\socketengine_epoll.obj" \
	"$(INTDIR)\socketengine_kqueue.obj" \
	"$(INTDIR)\socketengine_poll.obj" \
	"$(INTDIR)\socketengine_select.obj" \
	"$(INTDIR)\socketengine_winsock.obj" \
	"$(INTDIR)\ssl.obj" \
	"$(INTDIR)\support.obj" \
	"$(INTDIR)\throttle.obj" \
	"$(INTDIR)\userban.obj" \
	"$(INTDIR)\version.obj" \
	"$(INTDIR)\whowas.obj" \
	"$(INTDIR)\zlink.obj" \
	"$(INTDIR)\adler32.obj" \
	"$(INTDIR)\compress.obj" \
	"$(INTDIR)\crc32.obj" \
	"$(INTDIR)\deflate.obj" \
	"$(INTDIR)\gzio.obj" \
	"$(INTDIR)\infback.obj" \
	"$(INTDIR)\inffast.obj" \
	"$(INTDIR)\inflate.obj" \
	"$(INTDIR)\inftrees.obj" \
	"$(INTDIR)\trees.obj" \
	"$(INTDIR)\uncompr.obj" \
	"$(INTDIR)\zutil.obj"

"$(OUTDIR)\UltimateIRCd.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"

OUTDIR=.\Debug_SSL
INTDIR=.\Debug_SSL
# Begin Custom Macros
OutDir=.\Debug_SSL
# End Custom Macros

ALL : "$(OUTDIR)\UltimateIRCd.exe" "$(OUTDIR)\UltimateIRCd.bsc"


CLEAN :
	-@erase "$(INTDIR)\adler32.obj"
	-@erase "$(INTDIR)\adler32.sbr"
	-@erase "$(INTDIR)\blalloc.obj"
	-@erase "$(INTDIR)\blalloc.sbr"
	-@erase "$(INTDIR)\bsd.obj"
	-@erase "$(INTDIR)\bsd.sbr"
	-@erase "$(INTDIR)\channel.obj"
	-@erase "$(INTDIR)\channel.sbr"
	-@erase "$(INTDIR)\class.obj"
	-@erase "$(INTDIR)\class.sbr"
	-@erase "$(INTDIR)\clientlist.obj"
	-@erase "$(INTDIR)\clientlist.sbr"
	-@erase "$(INTDIR)\compress.obj"
	-@erase "$(INTDIR)\compress.sbr"
	-@erase "$(INTDIR)\crc32.obj"
	-@erase "$(INTDIR)\crc32.sbr"
	-@erase "$(INTDIR)\dbuf.obj"
	-@erase "$(INTDIR)\dbuf.sbr"
	-@erase "$(INTDIR)\deflate.obj"
	-@erase "$(INTDIR)\deflate.sbr"
	-@erase "$(INTDIR)\dich_conf.obj"
	-@erase "$(INTDIR)\dich_conf.sbr"
	-@erase "$(INTDIR)\drone.obj"
	-@erase "$(INTDIR)\drone.sbr"
	-@erase "$(INTDIR)\dynconf.obj"
	-@erase "$(INTDIR)\dynconf.sbr"
	-@erase "$(INTDIR)\fdlist.obj"
	-@erase "$(INTDIR)\fdlist.sbr"
	-@erase "$(INTDIR)\fds.obj"
	-@erase "$(INTDIR)\fds.sbr"
	-@erase "$(INTDIR)\gzio.obj"
	-@erase "$(INTDIR)\gzio.sbr"
	-@erase "$(INTDIR)\hash.obj"
	-@erase "$(INTDIR)\hash.sbr"
	-@erase "$(INTDIR)\inet_addr.obj"
	-@erase "$(INTDIR)\inet_addr.sbr"
	-@erase "$(INTDIR)\inet_ntop.obj"
	-@erase "$(INTDIR)\inet_ntop.sbr"
	-@erase "$(INTDIR)\inet_pton.obj"
	-@erase "$(INTDIR)\inet_pton.sbr"
	-@erase "$(INTDIR)\infback.obj"
	-@erase "$(INTDIR)\infback.sbr"
	-@erase "$(INTDIR)\inffast.obj"
	-@erase "$(INTDIR)\inffast.sbr"
	-@erase "$(INTDIR)\inflate.obj"
	-@erase "$(INTDIR)\inflate.sbr"
	-@erase "$(INTDIR)\inftrees.obj"
	-@erase "$(INTDIR)\inftrees.sbr"
	-@erase "$(INTDIR)\ircd.obj"
	-@erase "$(INTDIR)\ircd.sbr"
	-@erase "$(INTDIR)\ircsprintf.obj"
	-@erase "$(INTDIR)\ircsprintf.sbr"
	-@erase "$(INTDIR)\list.obj"
	-@erase "$(INTDIR)\list.sbr"
	-@erase "$(INTDIR)\m_nick.obj"
	-@erase "$(INTDIR)\m_nick.sbr"
	-@erase "$(INTDIR)\m_who.obj"
	-@erase "$(INTDIR)\m_who.sbr"
	-@erase "$(INTDIR)\match.obj"
	-@erase "$(INTDIR)\match.sbr"
	-@erase "$(INTDIR)\packet.obj"
	-@erase "$(INTDIR)\packet.sbr"
	-@erase "$(INTDIR)\parse.obj"
	-@erase "$(INTDIR)\parse.sbr"
	-@erase "$(INTDIR)\rc4.obj"
	-@erase "$(INTDIR)\rc4.sbr"
	-@erase "$(INTDIR)\res.obj"
	-@erase "$(INTDIR)\res.sbr"
	-@erase "$(INTDIR)\res_comp.obj"
	-@erase "$(INTDIR)\res_comp.sbr"
	-@erase "$(INTDIR)\res_init.obj"
	-@erase "$(INTDIR)\res_init.sbr"
	-@erase "$(INTDIR)\res_mkquery.obj"
	-@erase "$(INTDIR)\res_mkquery.sbr"
	-@erase "$(INTDIR)\res_win32.obj"
	-@erase "$(INTDIR)\res_win32.sbr"
	-@erase "$(INTDIR)\s_auth.obj"
	-@erase "$(INTDIR)\s_auth.sbr"
	-@erase "$(INTDIR)\s_bsd.obj"
	-@erase "$(INTDIR)\s_bsd.sbr"
	-@erase "$(INTDIR)\s_conf.obj"
	-@erase "$(INTDIR)\s_conf.sbr"
	-@erase "$(INTDIR)\s_debug.obj"
	-@erase "$(INTDIR)\s_debug.sbr"
	-@erase "$(INTDIR)\s_err.obj"
	-@erase "$(INTDIR)\s_err.sbr"
	-@erase "$(INTDIR)\s_help.obj"
	-@erase "$(INTDIR)\s_help.sbr"
	-@erase "$(INTDIR)\s_hidehost.obj"
	-@erase "$(INTDIR)\s_hidehost.sbr"
	-@erase "$(INTDIR)\s_misc.obj"
	-@erase "$(INTDIR)\s_misc.sbr"
	-@erase "$(INTDIR)\s_numeric.obj"
	-@erase "$(INTDIR)\s_numeric.sbr"
	-@erase "$(INTDIR)\s_serv.obj"
	-@erase "$(INTDIR)\s_serv.sbr"
	-@erase "$(INTDIR)\s_services.obj"
	-@erase "$(INTDIR)\s_services.sbr"
	-@erase "$(INTDIR)\s_ultimate.obj"
	-@erase "$(INTDIR)\s_ultimate.sbr"
	-@erase "$(INTDIR)\s_user.obj"
	-@erase "$(INTDIR)\s_user.sbr"
	-@erase "$(INTDIR)\scache.obj"
	-@erase "$(INTDIR)\scache.sbr"
	-@erase "$(INTDIR)\send.obj"
	-@erase "$(INTDIR)\send.sbr"
	-@erase "$(INTDIR)\socketengine_devpoll.obj"
	-@erase "$(INTDIR)\socketengine_devpoll.sbr"
	-@erase "$(INTDIR)\socketengine_epoll.obj"
	-@erase "$(INTDIR)\socketengine_epoll.sbr"
	-@erase "$(INTDIR)\socketengine_kqueue.obj"
	-@erase "$(INTDIR)\socketengine_kqueue.sbr"
	-@erase "$(INTDIR)\socketengine_poll.obj"
	-@erase "$(INTDIR)\socketengine_poll.sbr"
	-@erase "$(INTDIR)\socketengine_select.obj"
	-@erase "$(INTDIR)\socketengine_select.sbr"
	-@erase "$(INTDIR)\socketengine_winsock.obj"
	-@erase "$(INTDIR)\socketengine_winsock.sbr"
	-@erase "$(INTDIR)\ssl.obj"
	-@erase "$(INTDIR)\ssl.sbr"
	-@erase "$(INTDIR)\support.obj"
	-@erase "$(INTDIR)\support.sbr"
	-@erase "$(INTDIR)\throttle.obj"
	-@erase "$(INTDIR)\throttle.sbr"
	-@erase "$(INTDIR)\trees.obj"
	-@erase "$(INTDIR)\trees.sbr"
	-@erase "$(INTDIR)\uncompr.obj"
	-@erase "$(INTDIR)\uncompr.sbr"
	-@erase "$(INTDIR)\userban.obj"
	-@erase "$(INTDIR)\userban.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\version.sbr"
	-@erase "$(INTDIR)\whowas.obj"
	-@erase "$(INTDIR)\whowas.sbr"
	-@erase "$(INTDIR)\zlink.obj"
	-@erase "$(INTDIR)\zlink.sbr"
	-@erase "$(INTDIR)\zutil.obj"
	-@erase "$(INTDIR)\zutil.sbr"
	-@erase "$(OUTDIR)\UltimateIRCd.bsc"
	-@erase "$(OUTDIR)\UltimateIRCd.exe"
	-@erase "$(OUTDIR)\UltimateIRCd.ilk"
	-@erase "$(OUTDIR)\UltimateIRCd.map"
	-@erase "$(OUTDIR)\UltimateIRCd.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../include" /I "../zlib" /I "c:\openssl\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "DEBUGMODE" /D "USE_SSL" /FAcs /Fa"$(INTDIR)\\" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\UltimateIRCd.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\UltimateIRCd.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\blalloc.sbr" \
	"$(INTDIR)\bsd.sbr" \
	"$(INTDIR)\channel.sbr" \
	"$(INTDIR)\class.sbr" \
	"$(INTDIR)\clientlist.sbr" \
	"$(INTDIR)\dbuf.sbr" \
	"$(INTDIR)\dich_conf.sbr" \
	"$(INTDIR)\drone.sbr" \
	"$(INTDIR)\dynconf.sbr" \
	"$(INTDIR)\fdlist.sbr" \
	"$(INTDIR)\fds.sbr" \
	"$(INTDIR)\hash.sbr" \
	"$(INTDIR)\inet_addr.sbr" \
	"$(INTDIR)\inet_ntop.sbr" \
	"$(INTDIR)\inet_pton.sbr" \
	"$(INTDIR)\ircd.sbr" \
	"$(INTDIR)\ircsprintf.sbr" \
	"$(INTDIR)\list.sbr" \
	"$(INTDIR)\m_nick.sbr" \
	"$(INTDIR)\m_who.sbr" \
	"$(INTDIR)\match.sbr" \
	"$(INTDIR)\packet.sbr" \
	"$(INTDIR)\parse.sbr" \
	"$(INTDIR)\rc4.sbr" \
	"$(INTDIR)\res.sbr" \
	"$(INTDIR)\res_comp.sbr" \
	"$(INTDIR)\res_init.sbr" \
	"$(INTDIR)\res_mkquery.sbr" \
	"$(INTDIR)\res_win32.sbr" \
	"$(INTDIR)\s_auth.sbr" \
	"$(INTDIR)\s_bsd.sbr" \
	"$(INTDIR)\s_conf.sbr" \
	"$(INTDIR)\s_debug.sbr" \
	"$(INTDIR)\s_err.sbr" \
	"$(INTDIR)\s_help.sbr" \
	"$(INTDIR)\s_hidehost.sbr" \
	"$(INTDIR)\s_misc.sbr" \
	"$(INTDIR)\s_numeric.sbr" \
	"$(INTDIR)\s_serv.sbr" \
	"$(INTDIR)\s_services.sbr" \
	"$(INTDIR)\s_ultimate.sbr" \
	"$(INTDIR)\s_user.sbr" \
	"$(INTDIR)\scache.sbr" \
	"$(INTDIR)\send.sbr" \
	"$(INTDIR)\socketengine_devpoll.sbr" \
	"$(INTDIR)\socketengine_epoll.sbr" \
	"$(INTDIR)\socketengine_kqueue.sbr" \
	"$(INTDIR)\socketengine_poll.sbr" \
	"$(INTDIR)\socketengine_select.sbr" \
	"$(INTDIR)\socketengine_winsock.sbr" \
	"$(INTDIR)\ssl.sbr" \
	"$(INTDIR)\support.sbr" \
	"$(INTDIR)\throttle.sbr" \
	"$(INTDIR)\userban.sbr" \
	"$(INTDIR)\version.sbr" \
	"$(INTDIR)\whowas.sbr" \
	"$(INTDIR)\zlink.sbr" \
	"$(INTDIR)\adler32.sbr" \
	"$(INTDIR)\compress.sbr" \
	"$(INTDIR)\crc32.sbr" \
	"$(INTDIR)\deflate.sbr" \
	"$(INTDIR)\gzio.sbr" \
	"$(INTDIR)\infback.sbr" \
	"$(INTDIR)\inffast.sbr" \
	"$(INTDIR)\inflate.sbr" \
	"$(INTDIR)\inftrees.sbr" \
	"$(INTDIR)\trees.sbr" \
	"$(INTDIR)\uncompr.sbr" \
	"$(INTDIR)\zutil.sbr"

"$(OUTDIR)\UltimateIRCd.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\UltimateIRCd.pdb" /map:"$(INTDIR)\UltimateIRCd.map" /debug /machine:I386 /out:"$(OUTDIR)\UltimateIRCd.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\blalloc.obj" \
	"$(INTDIR)\bsd.obj" \
	"$(INTDIR)\channel.obj" \
	"$(INTDIR)\class.obj" \
	"$(INTDIR)\clientlist.obj" \
	"$(INTDIR)\dbuf.obj" \
	"$(INTDIR)\dich_conf.obj" \
	"$(INTDIR)\drone.obj" \
	"$(INTDIR)\dynconf.obj" \
	"$(INTDIR)\fdlist.obj" \
	"$(INTDIR)\fds.obj" \
	"$(INTDIR)\hash.obj" \
	"$(INTDIR)\inet_addr.obj" \
	"$(INTDIR)\inet_ntop.obj" \
	"$(INTDIR)\inet_pton.obj" \
	"$(INTDIR)\ircd.obj" \
	"$(INTDIR)\ircsprintf.obj" \
	"$(INTDIR)\list.obj" \
	"$(INTDIR)\m_nick.obj" \
	"$(INTDIR)\m_who.obj" \
	"$(INTDIR)\match.obj" \
	"$(INTDIR)\packet.obj" \
	"$(INTDIR)\parse.obj" \
	"$(INTDIR)\rc4.obj" \
	"$(INTDIR)\res.obj" \
	"$(INTDIR)\res_comp.obj" \
	"$(INTDIR)\res_init.obj" \
	"$(INTDIR)\res_mkquery.obj" \
	"$(INTDIR)\res_win32.obj" \
	"$(INTDIR)\s_auth.obj" \
	"$(INTDIR)\s_bsd.obj" \
	"$(INTDIR)\s_conf.obj" \
	"$(INTDIR)\s_debug.obj" \
	"$(INTDIR)\s_err.obj" \
	"$(INTDIR)\s_help.obj" \
	"$(INTDIR)\s_hidehost.obj" \
	"$(INTDIR)\s_misc.obj" \
	"$(INTDIR)\s_numeric.obj" \
	"$(INTDIR)\s_serv.obj" \
	"$(INTDIR)\s_services.obj" \
	"$(INTDIR)\s_ultimate.obj" \
	"$(INTDIR)\s_user.obj" \
	"$(INTDIR)\scache.obj" \
	"$(INTDIR)\send.obj" \
	"$(INTDIR)\socketengine_devpoll.obj" \
	"$(INTDIR)\socketengine_epoll.obj" \
	"$(INTDIR)\socketengine_kqueue.obj" \
	"$(INTDIR)\socketengine_poll.obj" \
	"$(INTDIR)\socketengine_select.obj" \
	"$(INTDIR)\socketengine_winsock.obj" \
	"$(INTDIR)\ssl.obj" \
	"$(INTDIR)\support.obj" \
	"$(INTDIR)\throttle.obj" \
	"$(INTDIR)\userban.obj" \
	"$(INTDIR)\version.obj" \
	"$(INTDIR)\whowas.obj" \
	"$(INTDIR)\zlink.obj" \
	"$(INTDIR)\adler32.obj" \
	"$(INTDIR)\compress.obj" \
	"$(INTDIR)\crc32.obj" \
	"$(INTDIR)\deflate.obj" \
	"$(INTDIR)\gzio.obj" \
	"$(INTDIR)\infback.obj" \
	"$(INTDIR)\inffast.obj" \
	"$(INTDIR)\inflate.obj" \
	"$(INTDIR)\inftrees.obj" \
	"$(INTDIR)\trees.obj" \
	"$(INTDIR)\uncompr.obj" \
	"$(INTDIR)\zutil.obj"

"$(OUTDIR)\UltimateIRCd.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"

OUTDIR=.\Release_SSL
INTDIR=.\Release_SSL
# Begin Custom Macros
OutDir=.\Release_SSL
# End Custom Macros

ALL : "$(OUTDIR)\UltimateIRCd.exe"


CLEAN :
	-@erase "$(INTDIR)\adler32.obj"
	-@erase "$(INTDIR)\blalloc.obj"
	-@erase "$(INTDIR)\bsd.obj"
	-@erase "$(INTDIR)\channel.obj"
	-@erase "$(INTDIR)\class.obj"
	-@erase "$(INTDIR)\clientlist.obj"
	-@erase "$(INTDIR)\compress.obj"
	-@erase "$(INTDIR)\crc32.obj"
	-@erase "$(INTDIR)\dbuf.obj"
	-@erase "$(INTDIR)\deflate.obj"
	-@erase "$(INTDIR)\dich_conf.obj"
	-@erase "$(INTDIR)\drone.obj"
	-@erase "$(INTDIR)\dynconf.obj"
	-@erase "$(INTDIR)\fdlist.obj"
	-@erase "$(INTDIR)\fds.obj"
	-@erase "$(INTDIR)\gzio.obj"
	-@erase "$(INTDIR)\hash.obj"
	-@erase "$(INTDIR)\inet_addr.obj"
	-@erase "$(INTDIR)\inet_ntop.obj"
	-@erase "$(INTDIR)\inet_pton.obj"
	-@erase "$(INTDIR)\infback.obj"
	-@erase "$(INTDIR)\inffast.obj"
	-@erase "$(INTDIR)\inflate.obj"
	-@erase "$(INTDIR)\inftrees.obj"
	-@erase "$(INTDIR)\ircd.obj"
	-@erase "$(INTDIR)\ircsprintf.obj"
	-@erase "$(INTDIR)\list.obj"
	-@erase "$(INTDIR)\m_nick.obj"
	-@erase "$(INTDIR)\m_who.obj"
	-@erase "$(INTDIR)\match.obj"
	-@erase "$(INTDIR)\packet.obj"
	-@erase "$(INTDIR)\parse.obj"
	-@erase "$(INTDIR)\rc4.obj"
	-@erase "$(INTDIR)\res.obj"
	-@erase "$(INTDIR)\res_comp.obj"
	-@erase "$(INTDIR)\res_init.obj"
	-@erase "$(INTDIR)\res_mkquery.obj"
	-@erase "$(INTDIR)\res_win32.obj"
	-@erase "$(INTDIR)\s_auth.obj"
	-@erase "$(INTDIR)\s_bsd.obj"
	-@erase "$(INTDIR)\s_conf.obj"
	-@erase "$(INTDIR)\s_debug.obj"
	-@erase "$(INTDIR)\s_err.obj"
	-@erase "$(INTDIR)\s_help.obj"
	-@erase "$(INTDIR)\s_hidehost.obj"
	-@erase "$(INTDIR)\s_misc.obj"
	-@erase "$(INTDIR)\s_numeric.obj"
	-@erase "$(INTDIR)\s_serv.obj"
	-@erase "$(INTDIR)\s_services.obj"
	-@erase "$(INTDIR)\s_ultimate.obj"
	-@erase "$(INTDIR)\s_user.obj"
	-@erase "$(INTDIR)\scache.obj"
	-@erase "$(INTDIR)\send.obj"
	-@erase "$(INTDIR)\socketengine_devpoll.obj"
	-@erase "$(INTDIR)\socketengine_epoll.obj"
	-@erase "$(INTDIR)\socketengine_kqueue.obj"
	-@erase "$(INTDIR)\socketengine_poll.obj"
	-@erase "$(INTDIR)\socketengine_select.obj"
	-@erase "$(INTDIR)\socketengine_winsock.obj"
	-@erase "$(INTDIR)\ssl.obj"
	-@erase "$(INTDIR)\support.obj"
	-@erase "$(INTDIR)\throttle.obj"
	-@erase "$(INTDIR)\trees.obj"
	-@erase "$(INTDIR)\uncompr.obj"
	-@erase "$(INTDIR)\userban.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\whowas.obj"
	-@erase "$(INTDIR)\zlink.obj"
	-@erase "$(INTDIR)\zutil.obj"
	-@erase "$(OUTDIR)\UltimateIRCd.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "..\include" /I "..\zlib" /I "c:\openssl\include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "USE_SSL" /Fp"$(INTDIR)\UltimateIRCd.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\UltimateIRCd.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\UltimateIRCd.pdb" /machine:I386 /out:"$(OUTDIR)\UltimateIRCd.exe" 
LINK32_OBJS= \
	"$(INTDIR)\blalloc.obj" \
	"$(INTDIR)\bsd.obj" \
	"$(INTDIR)\channel.obj" \
	"$(INTDIR)\class.obj" \
	"$(INTDIR)\clientlist.obj" \
	"$(INTDIR)\dbuf.obj" \
	"$(INTDIR)\dich_conf.obj" \
	"$(INTDIR)\drone.obj" \
	"$(INTDIR)\dynconf.obj" \
	"$(INTDIR)\fdlist.obj" \
	"$(INTDIR)\fds.obj" \
	"$(INTDIR)\hash.obj" \
	"$(INTDIR)\inet_addr.obj" \
	"$(INTDIR)\inet_ntop.obj" \
	"$(INTDIR)\inet_pton.obj" \
	"$(INTDIR)\ircd.obj" \
	"$(INTDIR)\ircsprintf.obj" \
	"$(INTDIR)\list.obj" \
	"$(INTDIR)\m_nick.obj" \
	"$(INTDIR)\m_who.obj" \
	"$(INTDIR)\match.obj" \
	"$(INTDIR)\packet.obj" \
	"$(INTDIR)\parse.obj" \
	"$(INTDIR)\rc4.obj" \
	"$(INTDIR)\res.obj" \
	"$(INTDIR)\res_comp.obj" \
	"$(INTDIR)\res_init.obj" \
	"$(INTDIR)\res_mkquery.obj" \
	"$(INTDIR)\res_win32.obj" \
	"$(INTDIR)\s_auth.obj" \
	"$(INTDIR)\s_bsd.obj" \
	"$(INTDIR)\s_conf.obj" \
	"$(INTDIR)\s_debug.obj" \
	"$(INTDIR)\s_err.obj" \
	"$(INTDIR)\s_help.obj" \
	"$(INTDIR)\s_hidehost.obj" \
	"$(INTDIR)\s_misc.obj" \
	"$(INTDIR)\s_numeric.obj" \
	"$(INTDIR)\s_serv.obj" \
	"$(INTDIR)\s_services.obj" \
	"$(INTDIR)\s_ultimate.obj" \
	"$(INTDIR)\s_user.obj" \
	"$(INTDIR)\scache.obj" \
	"$(INTDIR)\send.obj" \
	"$(INTDIR)\socketengine_devpoll.obj" \
	"$(INTDIR)\socketengine_epoll.obj" \
	"$(INTDIR)\socketengine_kqueue.obj" \
	"$(INTDIR)\socketengine_poll.obj" \
	"$(INTDIR)\socketengine_select.obj" \
	"$(INTDIR)\socketengine_winsock.obj" \
	"$(INTDIR)\ssl.obj" \
	"$(INTDIR)\support.obj" \
	"$(INTDIR)\throttle.obj" \
	"$(INTDIR)\userban.obj" \
	"$(INTDIR)\version.obj" \
	"$(INTDIR)\whowas.obj" \
	"$(INTDIR)\zlink.obj" \
	"$(INTDIR)\adler32.obj" \
	"$(INTDIR)\compress.obj" \
	"$(INTDIR)\crc32.obj" \
	"$(INTDIR)\deflate.obj" \
	"$(INTDIR)\gzio.obj" \
	"$(INTDIR)\infback.obj" \
	"$(INTDIR)\inffast.obj" \
	"$(INTDIR)\inflate.obj" \
	"$(INTDIR)\inftrees.obj" \
	"$(INTDIR)\trees.obj" \
	"$(INTDIR)\uncompr.obj" \
	"$(INTDIR)\zutil.obj"

"$(OUTDIR)\UltimateIRCd.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("UltimateIRCd.dep")
!INCLUDE "UltimateIRCd.dep"
!ELSE 
!MESSAGE Warning: cannot find "UltimateIRCd.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "UltimateIRCd - Win32 Release" || "$(CFG)" == "UltimateIRCd - Win32 Debug" || "$(CFG)" == "UltimateIRCd - Win32 Debug SSL" || "$(CFG)" == "UltimateIRCd - Win32 Release SSL"
SOURCE=..\src\blalloc.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\blalloc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\blalloc.obj"	"$(INTDIR)\blalloc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\blalloc.obj"	"$(INTDIR)\blalloc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\blalloc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\bsd.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\bsd.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\bsd.obj"	"$(INTDIR)\bsd.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\bsd.obj"	"$(INTDIR)\bsd.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\bsd.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\channel.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\channel.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\channel.obj"	"$(INTDIR)\channel.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\channel.obj"	"$(INTDIR)\channel.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\channel.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\class.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\class.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\class.obj"	"$(INTDIR)\class.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\class.obj"	"$(INTDIR)\class.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\class.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\clientlist.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\clientlist.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\clientlist.obj"	"$(INTDIR)\clientlist.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\clientlist.obj"	"$(INTDIR)\clientlist.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\clientlist.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\dbuf.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\dbuf.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\dbuf.obj"	"$(INTDIR)\dbuf.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\dbuf.obj"	"$(INTDIR)\dbuf.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\dbuf.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\dich_conf.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\dich_conf.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\dich_conf.obj"	"$(INTDIR)\dich_conf.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\dich_conf.obj"	"$(INTDIR)\dich_conf.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\dich_conf.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\drone.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\drone.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\drone.obj"	"$(INTDIR)\drone.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\drone.obj"	"$(INTDIR)\drone.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\drone.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\dynconf.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\dynconf.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\dynconf.obj"	"$(INTDIR)\dynconf.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\dynconf.obj"	"$(INTDIR)\dynconf.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\dynconf.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\fdlist.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\fdlist.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\fdlist.obj"	"$(INTDIR)\fdlist.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\fdlist.obj"	"$(INTDIR)\fdlist.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\fdlist.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\fds.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\fds.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\fds.obj"	"$(INTDIR)\fds.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\fds.obj"	"$(INTDIR)\fds.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\fds.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\hash.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\hash.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\hash.obj"	"$(INTDIR)\hash.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\hash.obj"	"$(INTDIR)\hash.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\hash.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\inet_addr.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\inet_addr.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\inet_addr.obj"	"$(INTDIR)\inet_addr.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\inet_addr.obj"	"$(INTDIR)\inet_addr.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\inet_addr.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\inet_ntop.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\inet_ntop.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\inet_ntop.obj"	"$(INTDIR)\inet_ntop.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\inet_ntop.obj"	"$(INTDIR)\inet_ntop.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\inet_ntop.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\inet_pton.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\inet_pton.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\inet_pton.obj"	"$(INTDIR)\inet_pton.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\inet_pton.obj"	"$(INTDIR)\inet_pton.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\inet_pton.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\ircd.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\ircd.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\ircd.obj"	"$(INTDIR)\ircd.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\ircd.obj"	"$(INTDIR)\ircd.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\ircd.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\ircsprintf.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\ircsprintf.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\ircsprintf.obj"	"$(INTDIR)\ircsprintf.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\ircsprintf.obj"	"$(INTDIR)\ircsprintf.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\ircsprintf.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\list.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\list.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\list.obj"	"$(INTDIR)\list.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\list.obj"	"$(INTDIR)\list.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\list.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\m_nick.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\m_nick.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\m_nick.obj"	"$(INTDIR)\m_nick.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\m_nick.obj"	"$(INTDIR)\m_nick.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\m_nick.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\m_who.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\m_who.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\m_who.obj"	"$(INTDIR)\m_who.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\m_who.obj"	"$(INTDIR)\m_who.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\m_who.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\match.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\match.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\match.obj"	"$(INTDIR)\match.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\match.obj"	"$(INTDIR)\match.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\match.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\packet.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\packet.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\packet.obj"	"$(INTDIR)\packet.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\packet.obj"	"$(INTDIR)\packet.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\packet.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\parse.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\parse.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\parse.obj"	"$(INTDIR)\parse.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\parse.obj"	"$(INTDIR)\parse.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\parse.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\rc4.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\rc4.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\rc4.obj"	"$(INTDIR)\rc4.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\rc4.obj"	"$(INTDIR)\rc4.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\rc4.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\res.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\res.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\res.obj"	"$(INTDIR)\res.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\res.obj"	"$(INTDIR)\res.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\res.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\res_comp.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\res_comp.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\res_comp.obj"	"$(INTDIR)\res_comp.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\res_comp.obj"	"$(INTDIR)\res_comp.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\res_comp.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\res_init.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\res_init.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\res_init.obj"	"$(INTDIR)\res_init.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\res_init.obj"	"$(INTDIR)\res_init.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\res_init.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\res_mkquery.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\res_mkquery.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\res_mkquery.obj"	"$(INTDIR)\res_mkquery.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\res_mkquery.obj"	"$(INTDIR)\res_mkquery.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\res_mkquery.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\res_win32.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\res_win32.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\res_win32.obj"	"$(INTDIR)\res_win32.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\res_win32.obj"	"$(INTDIR)\res_win32.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\res_win32.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\s_auth.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\s_auth.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\s_auth.obj"	"$(INTDIR)\s_auth.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\s_auth.obj"	"$(INTDIR)\s_auth.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\s_auth.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\s_bsd.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\s_bsd.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\s_bsd.obj"	"$(INTDIR)\s_bsd.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\s_bsd.obj"	"$(INTDIR)\s_bsd.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\s_bsd.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\s_conf.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\s_conf.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\s_conf.obj"	"$(INTDIR)\s_conf.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\s_conf.obj"	"$(INTDIR)\s_conf.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\s_conf.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\s_debug.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\s_debug.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\s_debug.obj"	"$(INTDIR)\s_debug.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\s_debug.obj"	"$(INTDIR)\s_debug.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\s_debug.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\s_err.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\s_err.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\s_err.obj"	"$(INTDIR)\s_err.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\s_err.obj"	"$(INTDIR)\s_err.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\s_err.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\s_help.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\s_help.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\s_help.obj"	"$(INTDIR)\s_help.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\s_help.obj"	"$(INTDIR)\s_help.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\s_help.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\s_hidehost.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\s_hidehost.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\s_hidehost.obj"	"$(INTDIR)\s_hidehost.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\s_hidehost.obj"	"$(INTDIR)\s_hidehost.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\s_hidehost.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\s_misc.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\s_misc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\s_misc.obj"	"$(INTDIR)\s_misc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\s_misc.obj"	"$(INTDIR)\s_misc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\s_misc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\s_numeric.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\s_numeric.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\s_numeric.obj"	"$(INTDIR)\s_numeric.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\s_numeric.obj"	"$(INTDIR)\s_numeric.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\s_numeric.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\s_serv.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\s_serv.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\s_serv.obj"	"$(INTDIR)\s_serv.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\s_serv.obj"	"$(INTDIR)\s_serv.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\s_serv.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\s_services.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\s_services.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\s_services.obj"	"$(INTDIR)\s_services.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\s_services.obj"	"$(INTDIR)\s_services.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\s_services.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\s_ultimate.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\s_ultimate.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\s_ultimate.obj"	"$(INTDIR)\s_ultimate.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\s_ultimate.obj"	"$(INTDIR)\s_ultimate.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\s_ultimate.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\s_user.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\s_user.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\s_user.obj"	"$(INTDIR)\s_user.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\s_user.obj"	"$(INTDIR)\s_user.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\s_user.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\scache.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\scache.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\scache.obj"	"$(INTDIR)\scache.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\scache.obj"	"$(INTDIR)\scache.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\scache.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\send.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\send.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\send.obj"	"$(INTDIR)\send.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\send.obj"	"$(INTDIR)\send.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\send.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\socketengine_devpoll.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\socketengine_devpoll.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\socketengine_devpoll.obj"	"$(INTDIR)\socketengine_devpoll.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\socketengine_devpoll.obj"	"$(INTDIR)\socketengine_devpoll.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\socketengine_devpoll.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\socketengine_epoll.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\socketengine_epoll.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\socketengine_epoll.obj"	"$(INTDIR)\socketengine_epoll.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\socketengine_epoll.obj"	"$(INTDIR)\socketengine_epoll.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\socketengine_epoll.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\socketengine_kqueue.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\socketengine_kqueue.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\socketengine_kqueue.obj"	"$(INTDIR)\socketengine_kqueue.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\socketengine_kqueue.obj"	"$(INTDIR)\socketengine_kqueue.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\socketengine_kqueue.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\socketengine_poll.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\socketengine_poll.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\socketengine_poll.obj"	"$(INTDIR)\socketengine_poll.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\socketengine_poll.obj"	"$(INTDIR)\socketengine_poll.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\socketengine_poll.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\socketengine_select.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\socketengine_select.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\socketengine_select.obj"	"$(INTDIR)\socketengine_select.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\socketengine_select.obj"	"$(INTDIR)\socketengine_select.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\socketengine_select.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\socketengine_winsock.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\socketengine_winsock.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\socketengine_winsock.obj"	"$(INTDIR)\socketengine_winsock.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\socketengine_winsock.obj"	"$(INTDIR)\socketengine_winsock.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\socketengine_winsock.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\ssl.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\ssl.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\ssl.obj"	"$(INTDIR)\ssl.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\ssl.obj"	"$(INTDIR)\ssl.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\ssl.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\support.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\support.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\support.obj"	"$(INTDIR)\support.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\support.obj"	"$(INTDIR)\support.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\support.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\throttle.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\throttle.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\throttle.obj"	"$(INTDIR)\throttle.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\throttle.obj"	"$(INTDIR)\throttle.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\throttle.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\userban.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\userban.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\userban.obj"	"$(INTDIR)\userban.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\userban.obj"	"$(INTDIR)\userban.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\userban.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\version.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\version.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\version.obj"	"$(INTDIR)\version.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\version.obj"	"$(INTDIR)\version.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\version.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\whowas.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\whowas.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\whowas.obj"	"$(INTDIR)\whowas.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\whowas.obj"	"$(INTDIR)\whowas.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\whowas.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\src\zlink.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\zlink.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\zlink.obj"	"$(INTDIR)\zlink.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\zlink.obj"	"$(INTDIR)\zlink.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\zlink.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\CreateVersion.vbs

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"

!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"

InputPath=.\CreateVersion.vbs

"..\src\version.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	CreateVersion.vbs
<< 
	

!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"

InputPath=.\CreateVersion.vbs

"..\src\version.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	CreateVersion.vbs
<< 
	

!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"

!ENDIF 

SOURCE=..\zlib\adler32.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\adler32.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\adler32.obj"	"$(INTDIR)\adler32.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\adler32.obj"	"$(INTDIR)\adler32.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\adler32.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\zlib\compress.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\compress.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\compress.obj"	"$(INTDIR)\compress.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\compress.obj"	"$(INTDIR)\compress.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\compress.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\zlib\crc32.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\crc32.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\crc32.obj"	"$(INTDIR)\crc32.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\crc32.obj"	"$(INTDIR)\crc32.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\crc32.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\zlib\deflate.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\deflate.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\deflate.obj"	"$(INTDIR)\deflate.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\deflate.obj"	"$(INTDIR)\deflate.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\deflate.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\zlib\gzio.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\gzio.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\gzio.obj"	"$(INTDIR)\gzio.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\gzio.obj"	"$(INTDIR)\gzio.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\gzio.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\zlib\infback.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\infback.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\infback.obj"	"$(INTDIR)\infback.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\infback.obj"	"$(INTDIR)\infback.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\infback.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\zlib\inffast.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\inffast.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\inffast.obj"	"$(INTDIR)\inffast.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\inffast.obj"	"$(INTDIR)\inffast.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\inffast.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\zlib\inflate.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\inflate.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\inflate.obj"	"$(INTDIR)\inflate.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\inflate.obj"	"$(INTDIR)\inflate.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\inflate.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\zlib\inftrees.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\inftrees.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\inftrees.obj"	"$(INTDIR)\inftrees.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\inftrees.obj"	"$(INTDIR)\inftrees.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\inftrees.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\zlib\trees.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\trees.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\trees.obj"	"$(INTDIR)\trees.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\trees.obj"	"$(INTDIR)\trees.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\trees.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\zlib\uncompr.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\uncompr.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\uncompr.obj"	"$(INTDIR)\uncompr.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\uncompr.obj"	"$(INTDIR)\uncompr.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\uncompr.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\zlib\zutil.c

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"


"$(INTDIR)\zutil.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"


"$(INTDIR)\zutil.obj"	"$(INTDIR)\zutil.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"


"$(INTDIR)\zutil.obj"	"$(INTDIR)\zutil.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"


"$(INTDIR)\zutil.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


!ENDIF 

