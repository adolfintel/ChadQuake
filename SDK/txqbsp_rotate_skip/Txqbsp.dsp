# Microsoft Developer Studio Project File - Name="Txqbsp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=Txqbsp - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Txqbsp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Txqbsp.mak" CFG="Txqbsp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Txqbsp - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Txqbsp - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Txqbsp - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "DOUBLEVEC_T" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x41d /d "NDEBUG"
# ADD RSC /l 0x41d /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /map /machine:I386 /opt:nowin98
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Txqbsp - Win32 Debug"

# PROP BASE Use_MFC 0
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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "DOUBLEVEC_T" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x41d /d "_DEBUG"
# ADD RSC /l 0x41d /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Txqbsp - Win32 Release"
# Name "Txqbsp - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\brush.c

!IF  "$(CFG)" == "Txqbsp - Win32 Release"

# ADD CPP /O2

!ELSEIF  "$(CFG)" == "Txqbsp - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cmdlib.c

!IF  "$(CFG)" == "Txqbsp - Win32 Release"

# ADD CPP /O2

!ELSEIF  "$(CFG)" == "Txqbsp - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\csg4.c

!IF  "$(CFG)" == "Txqbsp - Win32 Release"

# ADD CPP /O2

!ELSEIF  "$(CFG)" == "Txqbsp - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dyna1.c

!IF  "$(CFG)" == "Txqbsp - Win32 Release"

# ADD CPP /O2

!ELSEIF  "$(CFG)" == "Txqbsp - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\map.c

!IF  "$(CFG)" == "Txqbsp - Win32 Release"

# ADD CPP /O2

!ELSEIF  "$(CFG)" == "Txqbsp - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\merge.c

!IF  "$(CFG)" == "Txqbsp - Win32 Release"

# ADD CPP /O2

!ELSEIF  "$(CFG)" == "Txqbsp - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\nodraw.c

!IF  "$(CFG)" == "Txqbsp - Win32 Release"

# ADD CPP /O2

!ELSEIF  "$(CFG)" == "Txqbsp - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\outside.c

!IF  "$(CFG)" == "Txqbsp - Win32 Release"

# ADD CPP /O2

!ELSEIF  "$(CFG)" == "Txqbsp - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\portals.c

!IF  "$(CFG)" == "Txqbsp - Win32 Release"

# ADD CPP /O2

!ELSEIF  "$(CFG)" == "Txqbsp - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\qbsp.c

!IF  "$(CFG)" == "Txqbsp - Win32 Release"

# ADD CPP /O2

!ELSEIF  "$(CFG)" == "Txqbsp - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\region.c

!IF  "$(CFG)" == "Txqbsp - Win32 Release"

# ADD CPP /O2

!ELSEIF  "$(CFG)" == "Txqbsp - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\solidbsp.c
# End Source File
# Begin Source File

SOURCE=.\surfaces.c
# End Source File
# Begin Source File

SOURCE=.\tjunc.c
# End Source File
# Begin Source File

SOURCE=.\txbspfile.c
# End Source File
# Begin Source File

SOURCE=.\txmathlib.c
# End Source File
# Begin Source File

SOURCE=.\writebsp.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\bsp5.h
# End Source File
# Begin Source File

SOURCE=.\bspfile.h
# End Source File
# Begin Source File

SOURCE=.\cmdlib.h
# End Source File
# Begin Source File

SOURCE=.\dyna1.h
# End Source File
# Begin Source File

SOURCE=.\map.h
# End Source File
# Begin Source File

SOURCE=.\mathlib.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
