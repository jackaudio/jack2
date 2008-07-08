# Microsoft Developer Studio Project File - Name="libjackmp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=libjackmp - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libjackmp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libjackmp.mak" CFG="libjackmp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libjackmp - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libjackmp - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libjackmp - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "./Release"
# PROP Intermediate_Dir "./Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBJACKMP_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /I "." /I "../common" /I "../common/jack" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBJACKMP_EXPORTS" /D "__STDC__" /D "REGEX_MALLOC" /D "STDC_HEADERS" /D "__SMP__" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /entry:"DllEntryPoint" /dll /machine:I386 /out:"./Release/bin/libjackmp.dll" /libpath:"./Release" /libpath:"./Release/bin"

!ELSEIF  "$(CFG)" == "libjackmp - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "./Debug"
# PROP Intermediate_Dir "./Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBJACKMP_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I "." /I "../common" /I "../common/jack" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBJACKMP_EXPORTS" /D "__STDC__" /D "REGEX_MALLOC" /D "STDC_HEADERS" /D "__SMP__" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"./Debug/bin/libjackmp.dll" /pdbtype:sept /libpath:"./Debug" /libpath:"./Debug/bin"

!ENDIF 

# Begin Target

# Name "libjackmp - Win32 Release"
# Name "libjackmp - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\common\JackActivationCount.cpp
# End Source File
# Begin Source File

SOURCE=..\common\JackAPI.cpp
# End Source File
# Begin Source File

SOURCE=..\common\JackAudioPort.cpp
# End Source File
# Begin Source File

SOURCE=..\common\JackClient.cpp
# End Source File
# Begin Source File

SOURCE=..\common\JackConnectionManager.cpp
# End Source File
# Begin Source File

SOURCE=..\common\JackEngineControl.cpp
# End Source File
# Begin Source File

SOURCE=..\common\JackError.cpp
# End Source File
# Begin Source File

SOURCE=..\common\JackFrameTimer.cpp
# End Source File
# Begin Source File

SOURCE=..\common\JackGlobals.cpp
# End Source File
# Begin Source File

SOURCE=..\common\JackGraphManager.cpp
# End Source File
# Begin Source File

SOURCE=..\common\JackLibAPI.cpp
# End Source File
# Begin Source File

SOURCE=..\common\JackLibClient.cpp
# End Source File
# Begin Source File

SOURCE=..\common\JackMessageBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\common\JackMidiAPI.cpp
# End Source File
# Begin Source File

SOURCE=..\common\JackMidiPort.cpp
# End Source File
# Begin Source File

SOURCE=..\common\JackPort.cpp
# End Source File
# Begin Source File

SOURCE=..\common\JackPortType.cpp
# End Source File
# Begin Source File

SOURCE=..\common\JackShmMem.cpp
# End Source File
# Begin Source File

SOURCE=..\common\JackTime.c
# End Source File
# Begin Source File

SOURCE=..\common\JackTools.cpp
# End Source File
# Begin Source File

SOURCE=..\common\JackTransportEngine.cpp
# End Source File
# Begin Source File

SOURCE=.\JackWinNamedPipe.cpp
# End Source File
# Begin Source File

SOURCE=.\JackWinNamedPipeClientChannel.cpp
# End Source File
# Begin Source File

SOURCE=.\JackWinProcessSync.cpp
# End Source File
# Begin Source File

SOURCE=.\JackWinSemaphore.cpp
# End Source File
# Begin Source File

SOURCE=.\JackWinThread.cpp
# End Source File
# Begin Source File

SOURCE=.\regex.c
# End Source File
# Begin Source File

SOURCE=.\resource.rc
# End Source File
# Begin Source File

SOURCE=..\common\shm.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
