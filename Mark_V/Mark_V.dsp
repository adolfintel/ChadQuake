# Microsoft Developer Studio Project File - Name="winquake" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=winquake - Win32 SDL Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Mark_V.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Mark_V.mak" CFG="winquake - Win32 SDL Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "winquake - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "winquake - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "winquake - Win32 GL Release" (based on "Win32 (x86) Application")
!MESSAGE "winquake - Win32 GL Debug" (based on "Win32 (x86) Application")
!MESSAGE "winquake - Win32 DX8 Release" (based on "Win32 (x86) Application")
!MESSAGE "winquake - Win32 DX8 Debug" (based on "Win32 (x86) Application")
!MESSAGE "winquake - Win32 SDL Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Object_Files\Release_MSVC_WinQuake"
# PROP Intermediate_Dir ".\Object_Files\Release_MSVC_WinQuake"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G5 /MT /GX /O2 /Ob2 /I "../sdk/" /I "../sdk/core" /I "../sdk/dxsdk/sdk8/include" /I "../SDK/pthreads-w32-2-9-1-release/include" /D "NDEBUG" /D id386=1 /D "WIN32" /D "_WINDOWS" /D "QUAKE_GAME" /D "CORE_PTHREADS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 advapi32.lib dsound.lib dxguid.lib gdi32.lib kernel32.lib ole32.lib oleaut32.lib shell32.lib user32.lib winmm.lib wsock32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib /nologo /subsystem:windows /profile /machine:I386 /out:"c:\quake\mark_v_winquake.exe" /libpath:"../sdk/dxsdk/sdk8/lib" /libpath:"../SDK/pthreads-w32-2-9-1-release/lib/x86" /DELAYLOAD:libcurl.dll

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Object_Files\Debug_MSVC_WinQuake"
# PROP Intermediate_Dir ".\Object_Files\Debug_MSVC_WinQuake"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G5 /MTd /W3 /GX /ZI /Od /I "../sdk/" /I "../sdk/core" /I "../sdk/dxsdk/sdk8/include" /I "../SDK/pthreads-w32-2-9-1-release/include" /D "_DEBUG" /D id386=0 /D "WIN32" /D "_WINDOWS" /D "QUAKE_GAME" /D "CORE_PTHREADS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 advapi32.lib dsound.lib dxguid.lib gdi32.lib kernel32.lib ole32.lib oleaut32.lib shell32.lib user32.lib winmm.lib wsock32.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"c:\quake\mark_v_winquake_debug.exe" /libpath:"../sdk/dxsdk/sdk8/lib" /libpath:"../SDK/pthreads-w32-2-9-1-release/lib/x86" /DELAYLOAD:libcurl.dll
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Object_Files\Release_MSVC_GLQuake"
# PROP Intermediate_Dir ".\Object_Files\Release_MSVC_GLQuake"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /GX /Ox /Ot /Ow /I "../sdk/" /I "../sdk/core" /I "../sdk/dxsdk/sdk8/include" /I "../sdk/libcurl-7.18.0-win32-msvc/include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# SUBTRACT BASE CPP /Oa /Og
# ADD CPP /nologo /G5 /W3 /GX /O2 /Ob2 /I "../sdk/" /I "../sdk/core" /I "../sdk/dxsdk/sdk8/include" /I "../SDK/pthreads-w32-2-9-1-release/include" /D "NDEBUG" /D "GLQUAKE" /D "WIN32" /D "_WINDOWS" /D "QUAKE_GAME" /D "CORE_PTHREADS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 advapi32.lib dsound.lib dxguid.lib gdi32.lib kernel32.lib ole32.lib oleaut32.lib shell32.lib user32.lib winmm.lib wsock32.lib /nologo /subsystem:windows /profile /machine:I386
# SUBTRACT BASE LINK32 /map /debug
# ADD LINK32 kernel32.lib user32.lib /nologo /subsystem:windows /profile /machine:I386 /out:"c:\quake\mark_v.exe" /libpath:"../sdk/dxsdk/sdk8/lib" /libpath:"../SDK/pthreads-w32-2-9-1-release/lib/x86" /DELAYLOAD:libcurl.dll

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Object_Files\Debug_MSVC_GLQuake"
# PROP Intermediate_Dir ".\Object_Files\Debug_MSVC_GLQuake"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /ML /GX /ZI /Od /I "../sdk/" /I "../sdk/core" /I "../sdk/dxsdk/sdk8/include" /I "../sdk/libcurl-7.18.0-win32-msvc/include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "GLQUAKE" /FR /YX /FD /c
# ADD CPP /nologo /G5 /MTd /W3 /GX /ZI /Od /I "../sdk/" /I "../sdk/core" /I "../sdk/dxsdk/sdk8/include" /I "../SDK/pthreads-w32-2-9-1-release/include" /D "_DEBUG" /D "GLQUAKE" /D "DEBUGGL" /D "WIN32" /D "_WINDOWS" /D "QUAKE_GAME" /D "CORE_PTHREADS" /FR /YX /FD /c
# SUBTRACT CPP /WX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 advapi32.lib dsound.lib dxguid.lib gdi32.lib kernel32.lib ole32.lib oleaut32.lib shell32.lib user32.lib winmm.lib wsock32.lib /nologo /subsystem:windows /debug /machine:I386 /out:".\Binaries\mark_v_debug.exe" /DELAYLOAD:libcurl.dll
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"c:\quake\mark_v_debug.exe" /libpath:"../sdk/dxsdk/sdk8/lib" /libpath:"../SDK/pthreads-w32-2-9-1-release/lib/x86" /DELAYLOAD:libcurl.dll
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Object_Files\Release_MSVC_DX8Quake"
# PROP Intermediate_Dir ".\Object_Files\Release_MSVC_DX8Quake"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /W3 /GX /O1 /I "../sdk/" /I "../sdk/core" /I "../sdk/dxsdk/sdk8/include" /I "../sdk/libcurl-7.18.0-win32-msvc/include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "GLQUAKE" /FR /YX /FD /c
# ADD CPP /nologo /G5 /W3 /GX /O1 /Ob2 /I "../sdk/" /I "../sdk/core" /I "../sdk/dxsdk/sdk8/include" /I "../SDK/pthreads-w32-2-9-1-release/include" /D "NDEBUG" /D "GLQUAKE" /D "DIRECT3D_WRAPPER" /D "WIN32" /D "_WINDOWS" /D "QUAKE_GAME" /D "CORE_PTHREADS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 advapi32.lib dsound.lib dxguid.lib gdi32.lib kernel32.lib ole32.lib oleaut32.lib shell32.lib user32.lib winmm.lib wsock32.lib /nologo /subsystem:windows /profile /machine:I386 /out:"c:\quake\mark_v.exe" /DELAYLOAD:libcurl.dll
# ADD LINK32 kernel32.lib user32.lib /nologo /subsystem:windows /profile /machine:I386 /out:"c:\quake\dx8_mark_v.exe" /libpath:"../sdk/dxsdk/sdk8/lib" /libpath:"../SDK/pthreads-w32-2-9-1-release/lib/x86" /DELAYLOAD:libcurl.dll

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Object_Files\Debug_MSVC_DX8Quake"
# PROP Intermediate_Dir ".\Object_Files\Debug_MSVC_DX8Quake"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /ML /W3 /GX /ZI /Od /I "../sdk/" /I "../sdk/core" /I "../sdk/dxsdk/sdk8/include" /I "../sdk/libcurl-7.18.0-win32-msvc/include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "GLQUAKE" /D "DEBUGGL" /D "GLQUAKE_HARDWARE_GAMMA" /FR /YX /FD /c
# ADD CPP /nologo /G5 /MTd /W3 /GX /ZI /Od /I "../sdk/" /I "../sdk/core" /I "../sdk/dxsdk/sdk8/include" /I "../SDK/pthreads-w32-2-9-1-release/include" /D "_DEBUG" /D "GLQUAKE" /D "DEBUGGL" /D "DIRECT3D_WRAPPER" /D "WIN32" /D "_WINDOWS" /D "QUAKE_GAME" /D "CORE_PTHREADS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 advapi32.lib dsound.lib dxguid.lib gdi32.lib kernel32.lib ole32.lib oleaut32.lib shell32.lib user32.lib winmm.lib wsock32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"c:\quake\mark_v_debug.exe" /DELAYLOAD:libcurl.dll
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"c:\quake\dx8_mark_v_debug.exe" /libpath:"../sdk/dxsdk/sdk8/lib" /libpath:"../SDK/pthreads-w32-2-9-1-release/lib/x86" /DELAYLOAD:libcurl.dll
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Object_Files\Debug_MSVC_SDL_Quake"
# PROP Intermediate_Dir ".\Object_Files\Debug_MSVC_SDL_Quake"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /ML /W3 /GX /ZI /Od /I "../sdk/" /I "../sdk/core" /I "../sdk/dxsdk/sdk8/include" /I "../SDK/pthreads-w32-2-9-1-release/include" /D "_DEBUG" /D "GLQUAKE" /D "DEBUGGL" /D "WIN32" /D "_WINDOWS" /D "QUAKE_GAME" /D "CORE_PTHREADS" /FR /YX /FD /c
# SUBTRACT BASE CPP /WX
# ADD CPP /nologo /G5 /MTd /W3 /GX /ZI /Od /I "../sdk/" /I "../sdk/core" /I "../SDK/pthreads-w32-2-9-1-release/include" /I "..\SDK\SDL2-devel-2.0.3-VC\SDL2-2.0.3\include" /D "_DEBUG" /D "GLQUAKE" /D "DEBUGGL" /D "WIN32" /D "_WINDOWS" /D "QUAKE_GAME" /D "SDLQUAKE" /FR /YX /FD /c
# SUBTRACT CPP /WX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"c:\quake\mark_v_debug.exe" /libpath:"../sdk/dxsdk/sdk8/lib" /libpath:"../SDK/pthreads-w32-2-9-1-release/lib/x86" /DELAYLOAD:libcurl.dll
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"c:\quake\mark_v_sdl_debug.exe" /libpath:"../SDK/pthreads-w32-2-9-1-release/lib/x86" /libpath:"..\SDK\SDL2-devel-2.0.3-VC\SDL2-2.0.3\lib\x86" /DELAYLOAD:libcurl.dll
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "winquake - Win32 Release"
# Name "winquake - Win32 Debug"
# Name "winquake - Win32 GL Release"
# Name "winquake - Win32 GL Debug"
# Name "winquake - Win32 DX8 Release"
# Name "winquake - Win32 DX8 Debug"
# Name "winquake - Win32 SDL Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Group "ASM"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\asm_i386.h

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_draw.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\Object_Files\Release_MSVC_WinQuake
InputPath=.\d_draw.s
InputName=d_draw

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_draw16.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\Object_Files\Release_MSVC_WinQuake
InputPath=.\d_draw16.s
InputName=d_draw16

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_parta.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\Object_Files\Release_MSVC_WinQuake
InputPath=.\d_parta.s
InputName=d_parta

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_polysa.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\Object_Files\Release_MSVC_WinQuake
InputPath=.\d_polysa.s
InputName=d_polysa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_scana.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\Object_Files\Release_MSVC_WinQuake
InputPath=.\d_scana.s
InputName=d_scana

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_spr8.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\Object_Files\Release_MSVC_WinQuake
InputPath=.\d_spr8.s
InputName=d_spr8

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_varsa.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\Object_Files\Release_MSVC_WinQuake
InputPath=.\d_varsa.s
InputName=d_varsa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\math.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\Object_Files\Release_MSVC_WinQuake
InputPath=.\math.s
InputName=math

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\quakeasm.h

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_aclipa.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\Object_Files\Release_MSVC_WinQuake
InputPath=.\r_aclipa.s
InputName=r_aclipa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_aliasa.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\Object_Files\Release_MSVC_WinQuake
InputPath=.\r_aliasa.s
InputName=r_aliasa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_drawa.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\Object_Files\Release_MSVC_WinQuake
InputPath=.\r_drawa.s
InputName=r_drawa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_edgea.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\Object_Files\Release_MSVC_WinQuake
InputPath=.\r_edgea.s
InputName=r_edgea

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_varsa.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\Object_Files\Release_MSVC_WinQuake
InputPath=.\r_varsa.s
InputName=r_varsa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\surf16.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\Object_Files\Release_MSVC_WinQuake
InputPath=.\surf16.s
InputName=surf16

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\surf8.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\Object_Files\Release_MSVC_WinQuake
InputPath=.\surf8.s
InputName=surf8

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sys_wina.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# Begin Custom Build - mycoolbuild
OutDir=.\Object_Files\Release_MSVC_WinQuake
InputPath=.\sys_wina.s
InputName=sys_wina

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP > $(OUTDIR)\$(InputName).spp $(InputPath) 
	gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                    $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\worlda.s

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "Driver"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\d_edge.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_fill.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_init.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_modech.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_part.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_polyse.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_scan.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_sky.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_sprite.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_surf.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_vars.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_zpoint.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "Ref GL"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\anorm_dots.h

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_alias.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_brush.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_common.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_draw.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_fog.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_mesh.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_rmain.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_rmisc.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_sky.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_sprite.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_test.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_texmgr.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_texmgr.h

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_warp.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_warp_sin.h

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_world.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\glquake.h

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vid_wgl.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vid_wglext.h

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "Ref Software"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\adivtab.h

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_iface.h

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\d_local.h

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\draw.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_aclip.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_alias.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_bsp.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_draw.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_edge.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_local.h

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_main.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# ADD CPP /O1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_misc.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_shared.h

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_sky.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_sprite.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\r_surf.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vid_wsoft.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "Ref DX8"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\SDK\core\dx8_mh_wrapper.c
# End Source File
# Begin Source File

SOURCE=..\SDK\core\dx8_mh_wrapper.h
# End Source File
# Begin Source File

SOURCE=.\gl_renderer.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gl_renderer.h
# End Source File
# End Group
# Begin Group "Mac OS X"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cd_osx.m

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\downloads_osx.m

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\in_osx.m

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\QAboutPanel.m

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\QApplication.m

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\QController.m

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\QSettingsPanel.m

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\QSettingsWindow.m

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\snd_osx.m

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sys_osx.m

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vid_osx.m

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "Core"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\SDK\core\base64.c
# End Source File
# Begin Source File

SOURCE=..\SDK\core\base64.h
# End Source File
# Begin Source File

SOURCE=..\SDK\core\core.c
# End Source File
# Begin Source File

SOURCE=..\SDK\core\core.h
# End Source File
# Begin Source File

SOURCE=..\SDK\Core\core_linux.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\SDK\core\core_opengl.h
# End Source File
# Begin Source File

SOURCE=..\SDK\core\core_win.c
# End Source File
# Begin Source File

SOURCE=..\SDK\core\core_windows.h
# End Source File
# Begin Source File

SOURCE=..\SDK\core\dirent_win.c
# End Source File
# Begin Source File

SOURCE=..\SDK\core\dirent_win.h
# End Source File
# Begin Source File

SOURCE=..\SDK\core\download.h
# End Source File
# Begin Source File

SOURCE=..\SDK\core\download_curl.c
# End Source File
# Begin Source File

SOURCE=..\SDK\Core\download_http.c
# End Source File
# Begin Source File

SOURCE=..\SDK\Core\download_procs.c
# End Source File
# Begin Source File

SOURCE=..\SDK\Core\download_procs.h
# End Source File
# Begin Source File

SOURCE=..\SDK\core\enumbits.c
# End Source File
# Begin Source File

SOURCE=..\SDK\core\enumbits.h
# End Source File
# Begin Source File

SOURCE=..\SDK\core\environment.h
# End Source File
# Begin Source File

SOURCE=..\SDK\core\file.c
# End Source File
# Begin Source File

SOURCE=..\SDK\core\file.h
# End Source File
# Begin Source File

SOURCE=..\SDK\core\gl_constants.h
# End Source File
# Begin Source File

SOURCE=..\SDK\core\image.c
# End Source File
# Begin Source File

SOURCE=..\SDK\core\image.h
# End Source File
# Begin Source File

SOURCE=..\SDK\core\interface.c
# End Source File
# Begin Source File

SOURCE=..\SDK\core\interface.h
# End Source File
# Begin Source File

SOURCE=..\SDK\core\lists.c
# End Source File
# Begin Source File

SOURCE=..\SDK\core\lists.h
# End Source File
# Begin Source File

SOURCE=..\SDK\core\lodepng.c
# End Source File
# Begin Source File

SOURCE=..\SDK\core\lodepng.h
# End Source File
# Begin Source File

SOURCE=..\SDK\core\math_general.c
# End Source File
# Begin Source File

SOURCE=..\SDK\core\math_general.h
# End Source File
# Begin Source File

SOURCE=..\SDK\core\math_matrix.c
# End Source File
# Begin Source File

SOURCE=..\SDK\core\math_matrix.h
# End Source File
# Begin Source File

SOURCE=..\SDK\core\math_vector.c
# End Source File
# Begin Source File

SOURCE=..\SDK\core\math_vector.h
# End Source File
# Begin Source File

SOURCE=..\SDK\core\miniz.h
# End Source File
# Begin Source File

SOURCE=..\SDK\Core\net_simple.c
# End Source File
# Begin Source File

SOURCE=..\SDK\Core\net_simple.h
# End Source File
# Begin Source File

SOURCE=..\SDK\core\pak.c
# End Source File
# Begin Source File

SOURCE=..\SDK\core\pak.h
# End Source File
# Begin Source File

SOURCE=..\SDK\Core\pthreads_core.h
# End Source File
# Begin Source File

SOURCE=..\SDK\core\stringlib.c
# End Source File
# Begin Source File

SOURCE=..\SDK\core\stringlib.h
# End Source File
# Begin Source File

SOURCE=..\SDK\Core\system.c
# End Source File
# Begin Source File

SOURCE=..\SDK\core\timelib.c
# End Source File
# Begin Source File

SOURCE=..\SDK\core\timelib.h
# End Source File
# Begin Source File

SOURCE=..\SDK\core\unzip_win.cpp
# End Source File
# Begin Source File

SOURCE=..\SDK\core\unzip_win.h
# End Source File
# Begin Source File

SOURCE=..\SDK\core\zip.c
# End Source File
# Begin Source File

SOURCE=..\SDK\core\zip.h
# End Source File
# Begin Source File

SOURCE=..\SDK\core\zip_win.cpp
# End Source File
# Begin Source File

SOURCE=..\SDK\core\zip_win.h
# End Source File
# End Group
# Begin Group "Dzip"

# PROP Default_Filter ""
# Begin Group "zlib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\SDK\Dzip\zlib\adler32.c
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\zlib\deflate.c
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\zlib\deflate.h
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\zlib\infblock.c
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\zlib\infblock.h
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\zlib\infcodes.c
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\zlib\infcodes.h
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\zlib\inffast.c
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\zlib\inffast.h
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\zlib\inffixed.h
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\zlib\inflate.c
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\zlib\inftrees.c
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\zlib\inftrees.h
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\zlib\infutil.c
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\zlib\infutil.h
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\zlib\trees.c
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\zlib\trees.h
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\zlib\zconf.h
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\zlib\zlib.h
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\zlib\zutil.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\SDK\Dzip\compress.c
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\conmain.c
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\crc32.c
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\decode.c
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\delete.c
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\dzip.h
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\dzipcon.h
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\encode.c
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\list.c
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\mainx.c
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\uncompress.c
# End Source File
# Begin Source File

SOURCE=..\SDK\Dzip\v1code.c
# End Source File
# End Group
# Begin Group "HTTP"

# PROP Default_Filter ""
# Begin Group "API"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\SDK\HTTPClient\API\HTTPClient.c
# End Source File
# Begin Source File

SOURCE=..\SDK\HTTPClient\API\HTTPClient.h
# End Source File
# Begin Source File

SOURCE=..\SDK\HTTPClient\API\HTTPClientAuth.c
# End Source File
# Begin Source File

SOURCE=..\SDK\HTTPClient\API\HTTPClientAuth.h
# End Source File
# Begin Source File

SOURCE=..\SDK\HTTPClient\API\HTTPClientCommon.h
# End Source File
# Begin Source File

SOURCE=..\SDK\HTTPClient\API\HTTPClientString.c
# End Source File
# Begin Source File

SOURCE=..\SDK\HTTPClient\API\HTTPClientString.h
# End Source File
# Begin Source File

SOURCE=..\SDK\HTTPClient\API\HTTPClientWrapper.c
# End Source File
# Begin Source File

SOURCE=..\SDK\HTTPClient\API\HTTPClientWrapper.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\SDK\HTTPClient\HTTPClientSample.c
# End Source File
# Begin Source File

SOURCE=..\SDK\HTTPClient\HTTPClientSample.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\anorms.h
# End Source File
# Begin Source File

SOURCE=.\arch_def.h
# End Source File
# Begin Source File

SOURCE=.\bspfile.h
# End Source File
# Begin Source File

SOURCE=.\buffers.c
# End Source File
# Begin Source File

SOURCE=.\buffers.h
# End Source File
# Begin Source File

SOURCE=.\cd_sdl.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cd_win.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chase.c
# End Source File
# Begin Source File

SOURCE=.\cl_demo.c
# End Source File
# Begin Source File

SOURCE=.\cl_download.c
# End Source File
# Begin Source File

SOURCE=.\cl_input.c
# End Source File
# Begin Source File

SOURCE=.\cl_main.c
# End Source File
# Begin Source File

SOURCE=.\cl_parse.c
# End Source File
# Begin Source File

SOURCE=.\cl_tent.c
# End Source File
# Begin Source File

SOURCE=.\client.h
# End Source File
# Begin Source File

SOURCE=.\cmd.c
# End Source File
# Begin Source File

SOURCE=.\cmd.h
# End Source File
# Begin Source File

SOURCE=.\cmd_list_sheet.h
# End Source File
# Begin Source File

SOURCE=.\cmdline_list_sheet.h
# End Source File
# Begin Source File

SOURCE=.\common.c
# End Source File
# Begin Source File

SOURCE=.\common.h
# End Source File
# Begin Source File

SOURCE=.\console.c
# End Source File
# Begin Source File

SOURCE=.\console.h
# End Source File
# Begin Source File

SOURCE=.\crc.c
# End Source File
# Begin Source File

SOURCE=.\crc.h
# End Source File
# Begin Source File

SOURCE=.\cvar.c
# End Source File
# Begin Source File

SOURCE=.\cvar.h
# End Source File
# Begin Source File

SOURCE=.\cvar_list_sheet.h
# End Source File
# Begin Source File

SOURCE=.\dedicated.c
# End Source File
# Begin Source File

SOURCE=.\dedicated_win.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dedicated_win.h

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\draw.h
# End Source File
# Begin Source File

SOURCE=.\dshow_mp3.c
# End Source File
# Begin Source File

SOURCE=.\host.c
# End Source File
# Begin Source File

SOURCE=.\host_cmd.c
# End Source File
# Begin Source File

SOURCE=.\in_sdl.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\in_win.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\input.c
# End Source File
# Begin Source File

SOURCE=.\input.h
# End Source File
# Begin Source File

SOURCE=.\keys.c
# End Source File
# Begin Source File

SOURCE=.\keys.h
# End Source File
# Begin Source File

SOURCE=.\location.c
# End Source File
# Begin Source File

SOURCE=.\location.h
# End Source File
# Begin Source File

SOURCE=.\main_central.c
# End Source File
# Begin Source File

SOURCE=.\mark_v_lmp.h
# End Source File
# Begin Source File

SOURCE=.\menu.c
# End Source File
# Begin Source File

SOURCE=.\menu.h
# End Source File
# Begin Source File

SOURCE=.\model.c
# End Source File
# Begin Source File

SOURCE=.\model.h
# End Source File
# Begin Source File

SOURCE=.\modelgen.h
# End Source File
# Begin Source File

SOURCE=.\movie.c
# End Source File
# Begin Source File

SOURCE=.\movie.h
# End Source File
# Begin Source File

SOURCE=.\movie_win.c
# End Source File
# Begin Source File

SOURCE=.\nehahra.c
# End Source File
# Begin Source File

SOURCE=.\nehahra.h
# End Source File
# Begin Source File

SOURCE=.\net.h
# End Source File
# Begin Source File

SOURCE=.\net_admin.c
# End Source File
# Begin Source File

SOURCE=.\net_admin.h
# End Source File
# Begin Source File

SOURCE=.\net_bsd.c
# End Source File
# Begin Source File

SOURCE=.\net_defs.h
# End Source File
# Begin Source File

SOURCE=.\net_dgrm.c
# End Source File
# Begin Source File

SOURCE=.\net_dgrm.h
# End Source File
# Begin Source File

SOURCE=.\net_loop.c
# End Source File
# Begin Source File

SOURCE=.\net_loop.h
# End Source File
# Begin Source File

SOURCE=.\net_main.c
# End Source File
# Begin Source File

SOURCE=.\net_sys.h
# End Source File
# Begin Source File

SOURCE=.\net_udp.c
# End Source File
# Begin Source File

SOURCE=.\net_udp.h
# End Source File
# Begin Source File

SOURCE=.\pr_cmds.c
# End Source File
# Begin Source File

SOURCE=.\pr_comp.h
# End Source File
# Begin Source File

SOURCE=.\pr_edict.c
# End Source File
# Begin Source File

SOURCE=.\pr_exec.c
# End Source File
# Begin Source File

SOURCE=.\progdefs.h
# End Source File
# Begin Source File

SOURCE=.\progs.h
# End Source File
# Begin Source File

SOURCE=.\protocol.h
# End Source File
# Begin Source File

SOURCE=.\q_image.c
# End Source File
# Begin Source File

SOURCE=.\q_image.h
# End Source File
# Begin Source File

SOURCE=.\q_lists.c
# End Source File
# Begin Source File

SOURCE=.\q_lists.h
# End Source File
# Begin Source File

SOURCE=.\q_mathlib.c
# End Source File
# Begin Source File

SOURCE=.\q_mathlib.h
# End Source File
# Begin Source File

SOURCE=.\q_music.c
# End Source File
# Begin Source File

SOURCE=.\q_music.h
# End Source File
# Begin Source File

SOURCE=.\q_sound.h
# End Source File
# Begin Source File

SOURCE=.\q_stdinc.h
# End Source File
# Begin Source File

SOURCE=.\quakedef.h
# End Source File
# Begin Source File

SOURCE=.\r_common.c
# End Source File
# Begin Source File

SOURCE=.\r_efrag.c
# End Source File
# Begin Source File

SOURCE=.\r_light.c
# End Source File
# Begin Source File

SOURCE=.\r_part.c
# End Source File
# Begin Source File

SOURCE=.\r_part_qmb.c
# End Source File
# Begin Source File

SOURCE=.\recent_file.c
# End Source File
# Begin Source File

SOURCE=.\recent_file.h
# End Source File
# Begin Source File

SOURCE=.\render.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\sbar.c
# End Source File
# Begin Source File

SOURCE=.\sbar.h
# End Source File
# Begin Source File

SOURCE=.\screen.c
# End Source File
# Begin Source File

SOURCE=.\screen.h
# End Source File
# Begin Source File

SOURCE=.\sdl_main.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sdlquake.h

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\server.h
# End Source File
# Begin Source File

SOURCE=.\snd_dma.c
# End Source File
# Begin Source File

SOURCE=.\snd_mem.c
# End Source File
# Begin Source File

SOURCE=.\snd_mix.c
# End Source File
# Begin Source File

SOURCE=.\snd_sdl.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\snd_win.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sound.h
# End Source File
# Begin Source File

SOURCE=.\spritegn.h
# End Source File
# Begin Source File

SOURCE=.\msinttypes\stdint.h
# End Source File
# Begin Source File

SOURCE=.\sv_main.c
# End Source File
# Begin Source File

SOURCE=.\sv_move.c
# End Source File
# Begin Source File

SOURCE=.\sv_phys.c
# End Source File
# Begin Source File

SOURCE=.\sv_user.c
# End Source File
# Begin Source File

SOURCE=.\sys.h
# End Source File
# Begin Source File

SOURCE=.\sys_sdl.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sys_win.c

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\talk_macro.c
# End Source File
# Begin Source File

SOURCE=.\talk_macro.h
# End Source File
# Begin Source File

SOURCE=.\text_autocomplete.c
# End Source File
# Begin Source File

SOURCE=.\text_autocomplete.h
# End Source File
# Begin Source File

SOURCE=.\text_edit.c
# End Source File
# Begin Source File

SOURCE=.\text_edit.h
# End Source File
# Begin Source File

SOURCE=.\text_history.c
# End Source File
# Begin Source File

SOURCE=.\text_history.h
# End Source File
# Begin Source File

SOURCE=.\text_key.c
# End Source File
# Begin Source File

SOURCE=.\text_key.h
# End Source File
# Begin Source File

SOURCE=.\text_undo.c
# End Source File
# Begin Source File

SOURCE=.\text_undo.h
# End Source File
# Begin Source File

SOURCE=.\tool_inspector.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tool_inspector.h

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tool_texturepointer.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tool_texturepointer.h

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\utilities.c
# End Source File
# Begin Source File

SOURCE=.\utilities.h
# End Source File
# Begin Source File

SOURCE=.\utilities_install.c
# End Source File
# Begin Source File

SOURCE=.\vid.c
# End Source File
# Begin Source File

SOURCE=.\vid.h
# End Source File
# Begin Source File

SOURCE=.\vid_sdlgl.c

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\view.c
# End Source File
# Begin Source File

SOURCE=.\view.h
# End Source File
# Begin Source File

SOURCE=.\wad.c
# End Source File
# Begin Source File

SOURCE=.\wad.h
# End Source File
# Begin Source File

SOURCE=.\winquake.h
# End Source File
# Begin Source File

SOURCE=.\world.c
# End Source File
# Begin Source File

SOURCE=.\world.h
# End Source File
# Begin Source File

SOURCE=.\wsaerror.h
# End Source File
# Begin Source File

SOURCE=.\zone.c
# End Source File
# Begin Source File

SOURCE=.\zone.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\mark_v_glquake.ico
# End Source File
# Begin Source File

SOURCE=.\mark_v_glquake.rc

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mark_v_glquake2.ico
# End Source File
# Begin Source File

SOURCE=.\mark_v_winquake.ico
# End Source File
# Begin Source File

SOURCE=.\mark_v_winquake.rc

!IF  "$(CFG)" == "winquake - Win32 Release"

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\documentation.txt

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\progdefs.q1
# End Source File
# Begin Source File

SOURCE=.\progdefs.q2
# End Source File
# Begin Source File

SOURCE=.\r_part_qmb.h
# End Source File
# Begin Source File

SOURCE=.\todo.txt

!IF  "$(CFG)" == "winquake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 GL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 DX8 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "winquake - Win32 SDL Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\todo_done.txt
# End Source File
# End Target
# End Project
