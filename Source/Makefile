#                              CD2Boot/2
#              Copyright (C) by Stangl Roman, 2000, 2001
#                       Roman_Stangl@at.ibm.com
#                 http://www.geocities.com/warpguru/
# This Code may be freely distributed, provided the Copyright isn't
# removed, under the conditions indicated in the documentation.    
#
# Makefile      CD2Boot/CDBoot/CDBootPM Makefile
#
# Requirements: Microsoft Macro Assembler 6.1 (or compatible)
#               IBM VisualAge C++ V3 (or compatible)
#               IBM Warp Toolkit V4.5 (or compatible)


# Macro definitions

!IFDEF DEBUG

MLFLAGS   = /Fl /Sa /c /DDEBUG 
LINKFLAGS = /TINY /MAP /LINE
ICCFLAGS  = /DDEBUG
CFLAGS    = /c /Tdp /G5 /Ti        /Tm /Se /Ms /Re     /ss /W3 /Gm          /DDEBUG
CFLAGSDYN = /c /Tdp /G5 /Ti        /Tm /Se /Ms /Re     /ss /W3 /Gm      /Gd /DDEBUG
CFLAGSSUB = /c /Tdp /G5 /Ti        /Tm /Se /Ms     /Rn /ss /W3              /DDEBUG
CFLAGSDLL = /c /Tdp /G5 /Ti        /Tm /Se /Ms /Re     /ss /W3     /Ge-     /DDEBUG
LFLAGS    = /DEBUG /NOE /NOD /ALIGN:16 /MAP /NOI /E    /PACKC:0x40000
LFLAGR    = /DEBUG           /ALIGN:1  /MAP

!ELSE

MLFLAGS   = /Fl /Sa /c 
LINKFLAGS = /TINY /MAP
ICCFLAGS  = 
CFLAGS    = /c /Tdp /G5 /Ti /Gs /O     /Se /Ms /Re     /ss /W3 /Gm          
CFLAGSDYN = /c /Tdp /G5 /Ti /Gs /O     /Se /Ms /Re     /ss /W3 /Gm      /Gd 
CFLAGSSUB = /c /Tdp /G5 /Ti /Gs /O     /Se /Ms     /Rn /ss /W3              
CFLAGSDLL = /c /Tdp /G5 /Ti /Gs /O     /Se /Ms /Re     /ss /W3     /Ge-     
LFLAGS    = /DEBUG /NOE /NOD /ALIGN:16 /MAP /NOI /E    /PACKC:0x40000
LFLAGR    = /DEBUG           /ALIGN:1  /MAP

!ENDIF  # DEBUG

RCFLAGS = -i .

LIBS    =  CPPOM30 + OS2386
LIBSDYN =  CPPOM30I + CPPOOC3I + OS2386
LIBSSUB =  CPPON30 + OS2386

CC      = icc $(CFLAGS)
CCDYN   = icc $(CFLAGSDYN)
CCSUB   = icc $(CFLAGSSUB)
CCDLL   = icc $(CFLAGSDLL)

LINK    = ILINK /NOFREE $(LFLAGS)
LINKSUB = ILINK /NOFREE $(LFLAGS)
LINKDYN = ILINK /NOFREE $(LFLAGS)
LINKRES = ILINK /NOFREE $(LFLAGR)

RC      = RC $(RCFLAGS)
BLDLEVEL = BuildLevel

# Without any macro defined, display instructions

!IFNDEF  CDBOOT
!IFNDEF  CD2BOOT

All :
    @echo 1. To built this package you need:
    @echo       *) IBM VisualAge C/C++ V3.0 (or similar) 
    @echo       *) IBM OS/2 Warp Toolkit V4.5 (or similar)
    @echo       *) Microsoft Macro Assembler 6.00 (or similar)
    @echo 2. Run 'NMAKE Clean' to clean up for the build process
    @echo 3. Run 'NMAKE "CD2BOOT=1"' to build the CD-ROM boot enhancement CD2BOOT
    @echo 4. Run 'NMAKE "CDBOOT=1"' to build the installation programs CDBOOT/CDBOOTPM
    @echo    For both builds you can specify '"DEBUG=1"' to build the debug versions
    @echo    Note! You can build CD2BOOT either by running the assembler in real-mode or 
    @echo          protected-mode
    @echo 5. Run 'NMAKE Install' to copy the built files into the CD Boot/2 directory
    @echo 6. The file 'BUILD.LOG' contains the log of an actual build process in case
    @echo    you have problems redoing the build

Install :
    @echo Copying CD2BOOT to ..
    @copy CD2BOOT ..\ /v >NUL
    @echo Copying CDBOOT*.EXE to ..
    @copy CDBOOT*.EXE ..\ /v >NUL
    @echo Copying CB2*.DLL to ..
    @copy CB2*.DLL ..\ /v >NUL
    @echo Copying CB2*.MSG to ..
    @copy CB2*.MSG ..\ /v >NUL

Clean :
    @echo Cleaning up the source directory for a new build
    -@del CD2BOOT /f >NUL
    -@del *.exe /f >NUL
    -@del *.dll /f >NUL
    -@del *.msg /f >NUL
    -@del *.ini /f >NUL
    -@del *.obj /f >NUL
    -@del *.map /f >NUL
    -@del *.res /f >NUL
    -@del *.lst /f >NUL
    -@del *.com /f >NUL
    -@del *.l /f >NUL

!ENDIF  #CD2BOOT
!ENDIF  #CDBOOT

# When defined "CD2BOOT=1" build real-mode boot selector CD2BOOT
# which replaces OS2BOOT on the bootable OS/2 installation diskette

!IFDEF  CD2BOOT

CD2Boot.bin : CD2Boot.obj
    link CD2Boot.obj $(LINKFLAGS), CD2Boot.com;
    copy CD2Boot.com CD2Boot

CD2Boot.obj : CD2Boot.asm Makefile
    ml $(MLFLAGS) CD2Boot.asm

!ENDIF # CD2BOOT

# When defined "CDBOOT=1" build OS/2 control program CDBOOT and CDBOOTPM
# to install CD2BOOT boot selector onto a bootable OS/2 installation
# diskette

!IFDEF  CDBOOT

all : CB2Us.msg CB2Gr.msg CDBootPM.exe CDBoot.exe CB2.dll CB2PM.dll

UStartup.obj : UStartup.cpp UStartup.hpp
    $(CCSUB) /D__AVIO__ /FoUStartup.obj UStartup.cpp

UStartupPM.obj : UStartup.cpp UStartup.hpp
    $(CCSUB) /D__PM__ /FoUStartupPM.obj UStartup.cpp

CB2Us.msg: CB2Us.txt CDBootPM.exe 
    MKMSGF CB2Us.txt CB2Us.msg /l 9,1,ENU /P 437

CB2Gr.msg: CB2Gr.txt
    MKMSGF CB2Gr.txt CB2Gr.msg /l 7,1,DEU /P 437

CDBoot2.obj : CDBoot2.cpp CDBoot2.hpp UStartup.hpp
    $(CCSUB) /D__AVIO__ /FoCDBoot2.obj CDBoot2.cpp

CDBoot2PM.obj : CDBoot2.cpp CDBoot2.hpp UStartup.hpp
    $(CCSUB) /D__PM__ /FoCDBoot2PM.obj CDBoot2.cpp

CDBootPM.l : Makefile
    echo CDBoot2PM.obj UStartupPM.obj printf.obj  >  CDBootPM.l
    echo CDBootPM.exe                           >> CDBootPM.l
    echo CDBootPM.map                           >> CDBootPM.l
    echo $(LIBSSUB)                             >> CDBootPM.l
    echo CDBootPM.def                           >> CDBootPM.l

CDBootPM.res : CDBootPM.rc CDBoot2.hpp CB2Dlg.dlg CB2.h CDBootPM.bmp
    $(RC) -r CDBootPM.rc
    @copy CDBootPM.res CB2Dlg.res

CDBootPM.exe : CDBoot2PM.obj UStartupPM.obj CDBootPM.def CDBootPM.l CDBootPM.res
    $(BLDLEVEL) CDBootPM.def CDBoot2.hpp
    $(LINK) @CDBootPM.l
    rc CDBootPM.res CDBootPM.exe

CDBoot.l : Makefile
    echo CDBoot2.obj UStartup.obj printf.obj    >  CDBoot.l
    echo CDBoot.exe                             >> CDBoot.l
    echo CDBoot.map                             >> CDBoot.l
    echo $(LIBSSUB)                             >> CDBoot.l
    echo CDBoot.def                             >> CDBoot.l

CDBoot.exe : CDBoot2.obj UStartup.obj CDBoot.def CDBoot.l
    $(BLDLEVEL) CDBoot.def CDBoot2.hpp
    $(LINK) @CDBoot.l

CB2PM.l : Makefile
    echo CB2PM.obj UStartupPM.obj printf.obj    >  CB2PM.l
    echo CB2PM.dll                              >> CB2PM.l
    echo CB2.map                                >> CB2PM.l
    echo $(LIBS)                                >> CB2PM.l
    echo CB2PM.def                              >> CB2PM.l

CB2PM.obj : CB2.cpp CDBoot2.hpp UStartup.hpp
    $(CCDLL) /D__PM__ /FoCB2PM.obj CB2.cpp

CB2PM.dll : CB2PM.obj UStartup.obj CB2.def CB2PM.l
    $(BLDLEVEL) CB2PM.def CDBoot2.hpp
    $(LINK) @CB2PM.l

CB2.l : Makefile
    echo CB2.obj UStartup.obj printf.obj        >  CB2.l
    echo CB2.dll                                >> CB2.l
    echo CB2.map                                >> CB2.l
    echo $(LIBS)                                >> CB2.l
    echo CB2.def                                >> CB2.l

CB2.obj : CB2.cpp CB2.hpp CDBoot2.hpp UStartup.hpp
    $(CCDLL) /D__AVIO__ /FoCB2.obj CB2.cpp

CB2.dll : CB2.obj UStartup.obj CB2.def CB2.l
    $(BLDLEVEL) CB2.def CDBoot2.hpp
    $(LINK) @CB2.l

!ENDIF  # CDBOOT
