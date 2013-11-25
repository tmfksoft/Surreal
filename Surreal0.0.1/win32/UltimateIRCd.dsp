# Microsoft Developer Studio Project File - Name="UltimateIRCd" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=UltimateIRCd - Win32 Debug SSL
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "UltimateIRCd.mak".
!MESSAGE 
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

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\include" /I "..\zlib" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x413 /d "NDEBUG"
# ADD RSC /l 0x413 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../include" /I "../zlib" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "DEBUGMODE" /FAcs /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x413 /d "_DEBUG"
# ADD RSC /l 0x413 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:console /map /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /verbose /pdb:none /force

!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "UltimateIRCd___Win32_Debug_SSL"
# PROP BASE Intermediate_Dir "UltimateIRCd___Win32_Debug_SSL"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_SSL"
# PROP Intermediate_Dir "Debug_SSL"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../include" /I "../zlib" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "DEBUGMODE" /FAcs /FR /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../include" /I "../zlib" /I "c:\openssl\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "DEBUGMODE" /D "USE_SSL" /FAcs /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x413 /d "_DEBUG"
# ADD RSC /l 0x413 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:console /map /debug /machine:I386 /pdbtype:sept
# SUBTRACT BASE LINK32 /verbose /pdb:none /force
# ADD LINK32 /nologo /subsystem:console /map /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /verbose /pdb:none /force

!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "UltimateIRCd___Win32_Release_SSL"
# PROP BASE Intermediate_Dir "UltimateIRCd___Win32_Release_SSL"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_SSL"
# PROP Intermediate_Dir "Release_SSL"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "..\include" /I "..\zlib" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\include" /I "..\zlib" /I "c:\openssl\include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "USE_SSL" /YX /FD /c
# ADD BASE RSC /l 0x413 /d "NDEBUG"
# ADD RSC /l 0x413 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:console /machine:I386
# ADD LINK32 /nologo /subsystem:console /machine:I386

!ENDIF 

# Begin Target

# Name "UltimateIRCd - Win32 Release"
# Name "UltimateIRCd - Win32 Debug"
# Name "UltimateIRCd - Win32 Debug SSL"
# Name "UltimateIRCd - Win32 Release SSL"
# Begin Group "IRCd"

# PROP Default_Filter ""
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\blalloc.c
# End Source File
# Begin Source File

SOURCE=..\src\bsd.c
# End Source File
# Begin Source File

SOURCE=..\src\channel.c
# End Source File
# Begin Source File

SOURCE=..\src\class.c
# End Source File
# Begin Source File

SOURCE=..\src\clientlist.c
# End Source File
# Begin Source File

SOURCE=..\src\dbuf.c
# End Source File
# Begin Source File

SOURCE=..\src\dich_conf.c
# End Source File
# Begin Source File

SOURCE=..\src\drone.c
# End Source File
# Begin Source File

SOURCE=..\src\dynconf.c
# End Source File
# Begin Source File

SOURCE=..\src\fdlist.c
# End Source File
# Begin Source File

SOURCE=..\src\fds.c
# End Source File
# Begin Source File

SOURCE=..\src\hash.c
# End Source File
# Begin Source File

SOURCE=..\src\inet_addr.c
# End Source File
# Begin Source File

SOURCE=..\src\inet_ntop.c
# End Source File
# Begin Source File

SOURCE=..\src\inet_pton.c
# End Source File
# Begin Source File

SOURCE=..\src\ircd.c
# End Source File
# Begin Source File

SOURCE=..\src\ircsprintf.c
# End Source File
# Begin Source File

SOURCE=..\src\list.c
# End Source File
# Begin Source File

SOURCE=..\src\m_nick.c
# End Source File
# Begin Source File

SOURCE=..\src\m_who.c
# End Source File
# Begin Source File

SOURCE=..\src\match.c
# End Source File
# Begin Source File

SOURCE=..\src\packet.c
# End Source File
# Begin Source File

SOURCE=..\src\parse.c
# End Source File
# Begin Source File

SOURCE=..\src\rc4.c
# End Source File
# Begin Source File

SOURCE=..\src\res.c
# End Source File
# Begin Source File

SOURCE=..\src\res_comp.c
# End Source File
# Begin Source File

SOURCE=..\src\res_init.c
# End Source File
# Begin Source File

SOURCE=..\src\res_mkquery.c
# End Source File
# Begin Source File

SOURCE=..\src\res_win32.c
# End Source File
# Begin Source File

SOURCE=..\src\s_auth.c
# End Source File
# Begin Source File

SOURCE=..\src\s_bsd.c
# End Source File
# Begin Source File

SOURCE=..\src\s_conf.c
# End Source File
# Begin Source File

SOURCE=..\src\s_debug.c
# End Source File
# Begin Source File

SOURCE=..\src\s_err.c
# End Source File
# Begin Source File

SOURCE=..\src\s_help.c
# End Source File
# Begin Source File

SOURCE=..\src\s_hidehost.c
# End Source File
# Begin Source File

SOURCE=..\src\s_misc.c
# End Source File
# Begin Source File

SOURCE=..\src\s_numeric.c
# End Source File
# Begin Source File

SOURCE=..\src\s_serv.c
# End Source File
# Begin Source File

SOURCE=..\src\s_services.c
# End Source File
# Begin Source File

SOURCE=..\src\s_ultimate.c
# End Source File
# Begin Source File

SOURCE=..\src\s_user.c
# End Source File
# Begin Source File

SOURCE=..\src\scache.c
# End Source File
# Begin Source File

SOURCE=..\src\send.c
# End Source File
# Begin Source File

SOURCE=..\src\socketengine_devpoll.c
# End Source File
# Begin Source File

SOURCE=..\src\socketengine_epoll.c
# End Source File
# Begin Source File

SOURCE=..\src\socketengine_kqueue.c
# End Source File
# Begin Source File

SOURCE=..\src\socketengine_poll.c
# End Source File
# Begin Source File

SOURCE=..\src\socketengine_select.c
# End Source File
# Begin Source File

SOURCE=..\src\socketengine_winsock.c
# End Source File
# Begin Source File

SOURCE=..\src\ssl.c
# End Source File
# Begin Source File

SOURCE=..\src\support.c
# End Source File
# Begin Source File

SOURCE=..\src\throttle.c
# End Source File
# Begin Source File

SOURCE=..\src\userban.c
# End Source File
# Begin Source File

SOURCE=..\src\version.c
# End Source File
# Begin Source File

SOURCE=..\src\whowas.c
# End Source File
# Begin Source File

SOURCE=..\src\zlink.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\include\blalloc.h
# End Source File
# Begin Source File

SOURCE=..\include\cdefs.h
# End Source File
# Begin Source File

SOURCE=..\include\channel.h
# End Source File
# Begin Source File

SOURCE=..\include\class.h
# End Source File
# Begin Source File

SOURCE=..\include\common.h
# End Source File
# Begin Source File

SOURCE=..\include\config.h
# End Source File
# Begin Source File

SOURCE=..\include\dbuf.h
# End Source File
# Begin Source File

SOURCE=..\include\defs.h
# End Source File
# Begin Source File

SOURCE=..\include\dh.h
# End Source File
# Begin Source File

SOURCE=..\include\dich_conf.h
# End Source File
# Begin Source File

SOURCE=..\include\dynconf.h
# End Source File
# Begin Source File

SOURCE=..\include\fdlist.h
# End Source File
# Begin Source File

SOURCE=..\include\fds.h
# End Source File
# Begin Source File

SOURCE=..\include\find.h
# End Source File
# Begin Source File

SOURCE=..\include\h.h
# End Source File
# Begin Source File

SOURCE=..\include\hash.h
# End Source File
# Begin Source File

SOURCE=..\include\inet.h
# End Source File
# Begin Source File

SOURCE=..\include\ircsprintf.h
# End Source File
# Begin Source File

SOURCE=..\include\msg.h
# End Source File
# Begin Source File

SOURCE=..\include\nameser.h
# End Source File
# Begin Source File

SOURCE=..\include\numeric.h
# End Source File
# Begin Source File

SOURCE=..\include\queue.h
# End Source File
# Begin Source File

SOURCE=..\include\res.h
# End Source File
# Begin Source File

SOURCE=..\include\res_win32.h
# End Source File
# Begin Source File

SOURCE=..\include\resolv.h
# End Source File
# Begin Source File

SOURCE=..\include\send.h
# End Source File
# Begin Source File

SOURCE=..\include\setup_win32.h
# End Source File
# Begin Source File

SOURCE=..\include\sock.h
# End Source File
# Begin Source File

SOURCE=..\include\ssl.h
# End Source File
# Begin Source File

SOURCE=..\include\struct.h
# End Source File
# Begin Source File

SOURCE=..\include\supported.h
# End Source File
# Begin Source File

SOURCE=..\include\sys.h
# End Source File
# Begin Source File

SOURCE=..\include\throttle.h
# End Source File
# Begin Source File

SOURCE=..\include\userban.h
# End Source File
# Begin Source File

SOURCE=..\include\va_copy.h
# End Source File
# Begin Source File

SOURCE=..\include\version.h
# End Source File
# Begin Source File

SOURCE=..\include\whowas.h
# End Source File
# Begin Source File

SOURCE=..\include\zlink.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\CreateVersion.vbs

!IF  "$(CFG)" == "UltimateIRCd - Win32 Release"

!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=.\CreateVersion.vbs

"..\src\version.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	CreateVersion.vbs

# End Custom Build

!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Debug SSL"

# PROP BASE Ignore_Default_Tool 1
# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=.\CreateVersion.vbs

"..\src\version.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	CreateVersion.vbs

# End Custom Build

!ELSEIF  "$(CFG)" == "UltimateIRCd - Win32 Release SSL"

!ENDIF 

# End Source File
# End Group
# Begin Group "zlib"

# PROP Default_Filter ""
# Begin Group "Source Files "

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\zlib\adler32.c
# End Source File
# Begin Source File

SOURCE=..\zlib\compress.c
# End Source File
# Begin Source File

SOURCE=..\zlib\crc32.c
# End Source File
# Begin Source File

SOURCE=..\zlib\deflate.c
# End Source File
# Begin Source File

SOURCE=..\zlib\gzio.c
# End Source File
# Begin Source File

SOURCE=..\zlib\infback.c
# End Source File
# Begin Source File

SOURCE=..\zlib\inffast.c
# End Source File
# Begin Source File

SOURCE=..\zlib\inflate.c
# End Source File
# Begin Source File

SOURCE=..\zlib\inftrees.c
# End Source File
# Begin Source File

SOURCE=..\zlib\trees.c
# End Source File
# Begin Source File

SOURCE=..\zlib\uncompr.c
# End Source File
# Begin Source File

SOURCE=..\zlib\zutil.c
# End Source File
# End Group
# Begin Group "Header Files "

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=..\zlib\crc32.h
# End Source File
# Begin Source File

SOURCE=..\zlib\deflate.h
# End Source File
# Begin Source File

SOURCE=..\zlib\inffast.h
# End Source File
# Begin Source File

SOURCE=..\zlib\inffixed.h
# End Source File
# Begin Source File

SOURCE=..\zlib\inflate.h
# End Source File
# Begin Source File

SOURCE=..\zlib\inftrees.h
# End Source File
# Begin Source File

SOURCE=..\zlib\trees.h
# End Source File
# Begin Source File

SOURCE=..\zlib\zconf.h
# End Source File
# Begin Source File

SOURCE=..\zlib\zconf.in.h
# End Source File
# Begin Source File

SOURCE=..\zlib\zlib.h
# End Source File
# Begin Source File

SOURCE=..\zlib\zutil.h
# End Source File
# End Group
# End Group
# End Target
# End Project
