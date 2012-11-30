/***********************************************************************\
 *                           BuildLevel.cmd                            *
 *             Copyright (C) by Stangl Roman, 1996, 1999               *
 *                                                                     *
 * This Code may be freely distributed, provided the Copyright isn't   *
 * removed.                                                            *
 *                                                                     *
 * BuildLevel.cmd                                                      *
 *              Batch file to replace the DESCRIPTION statement in a   *
 *              module's (EXE or DLL) with one the OS/2 BLDLEVEL       *
 *              command is able to read.                               *
 *              Syntax: BuildLevel ModuleDefinitionFile Headerfile     *
 *              e.g. BuildLevel PC2.def PC2.h                          *
 *                                                                     *
\***********************************************************************/

/* static char     _VERSION_[]="BuildLevel.cmd - V2.20"; */


Call RxFuncAdd 'SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs'
Call SysLoadFuncs
                                        /* Trap CTRL+BRK processing to avoid data corruption */
Signal On HALT NAME SignalHandler

                                        /* OS/2 returncode */
ReturnCode=1
                                        /* 1st commandline argument (1 *.DEF file) */
DefinitionFile=""
                                        /* As the contents of the 1st commandline argument
                                           file get changed, we make a temporary copy to read
                                           from and write the new contents to the original file */
DefinitionCopy="$Temp$"
                                        /* 2nd commandline argument (1 *.H* file) */
HeaderFile=""
                                        /* Name of the binary module which gets filled from
                                           the NAME (for *.EXE) or LIBRARY (for *.DLL) statement */
ModuleName=""
                                        /* Description statement which gets constructed from the
                                           header (*.H*) file to replace/add the DESCRIPTION
                                           statement in the *.DEF file */
Description=""
Description_Header="Description '"
Description_Trailer="'"
                                        /* The build level part of the DESCRIPTION statement is built 
                                           from the following substrings */
BldLevel=""
BldLevel_Header="$@#"
BldLevel_Vendor=""
BldLevel_Version=""
BldLevel_Info=""
BldLevel_Trailer="@#"

/*--------------------------------------------------------------------------------------*\
 * Main entry point.                                                                    *
 * Req:                                                                                 *
 *      none                                                                            *
 * Returns:                                                                             *
 *      ReturnCode .... Error code (0 if no error)                                      *
\*--------------------------------------------------------------------------------------*/
                                        /* Get the DefinitionFile (*.DEF) we want to check
                                           the BLDLEVEL and the Headerfile (*.H*) where
                                           the required BLDLEVEL data is defined in */
Parse Arg DefinitionFile HeaderFile .
                                        /* If no arguments are specified, display syntax */
If DefinitionFile="" | HeaderFile="" Then
    Do
    Say
    Say 'Syntax: BUILDLEVEL module.def header.h'
    Say
    Say 'Where: module.def ... Module definition file where the BLDLEVEL information'
    Say '                      will be written at the DESCRIPTION statement.'
    Say '       header.h ..... Header file where the macros BLDLEVEL_VENDOR,'
    Say '                      BLDLEVEL_VERSION and BLDLEVEL_INFO will be taken from.'
    Say
    Say 'Error '||ReturnCode||' incorrect commandline arguments specified.'
    Say
cmd='dir'
cmd
    Exit
    End
Do While (DefinitionFile\="" & HeaderFile\="")
                                        /* We have valid commandline arguments, so first see
                                           if the Headerfile contents are ok for us */
    ReturnCode=ParseHeaderFile()
    If ReturnCode\=0 Then Leave
                                        /* Parse the definition file (*.DEF) for the module
                                           name, to see if we got a valid file */
    ReturnCode=ParseDefinitionFile()
    If ReturnCode\=0 Then Leave
                                        /* Copy the definition file, because we're going to
                                           change the original */
    Command="@copy "||DefinitionFile||" "||DefinitionCopy||" /v >NUL"
    Command
    Command="@del "||DefinitionFile||" >NUL 2>&1"
    Command
    If (rc\=0) Then Do
        ReturnCode=2
        Leave
    End
                                        /* Build DESCRIPTION statement for *.DEF file */
    BldLevel=BldLevel_Header||BldLevel_Vendor||":"||BldLevel_Version||" ("||ModuleName||")#@"||BldLevel_Info
    Description=Description_Header||BldLevel||Description_Trailer
                                        /* Copy the module definition file to be able to change
                                           the contents of the original one. In case of an error
                                           copy back the original one */
    ReturnCode=ModifyDefinitionFile()
    If ReturnCode\=0 Then Do
        Command="@copy "||DefinitionCopy||" "||DefinitionFile||" /v >NUL"
        Command
        Command="@del "||DefinitionCopy||" >NUL 2>&1"
        Command
        Leave
    End

                                        /* Exit with success */
    Command="@del "||DefinitionCopy||" >NUL 2>&1"
    Command
    ReturnCode=0
    Leave
End
                                        /* Exit to commandline */
If (ReturnCode\=0) Then Do
    Say ""
    Say "Error "||ReturnCode||" modifying "||DefinitionFile||", no changes made"
    Say ""
End
Else Do
    Say ""
    Say "Module definition file "||DefinitionFile||" successfully modified"
    Say ""
End
Return ReturnCode 

/*--------------------------------------------------------------------------------------*\
 * Parse a C/C++ header file to test that the macros we build the BLDLEVEL information  *
 * from (BLDLEVEL_VENDOR, BLDLEVEL_VERSION, BLDLEVEL_INFO) are defined.                 *
 * Req:                                                                                 *
 *      none                                                                            *
 * Returns:                                                                             *
 *      ReturnCode .... Error code (0 if no error)                                      *
\*--------------------------------------------------------------------------------------*/
ParseHeaderFile: 
    LinesLeft=2
                                        /* Read ahead 1st line */
    CurrentLine=LineIn(HeaderFile, 1, 1)
                                        /* Do while there are lines in the file */
    Do While(LinesLeft>0)
                                        /* Parse for constructs of the form:
                                           #define BLDLEVEL_Macro "Value" Comment */
                                        /* Kill that ... tabs */
        CurrentLine=Translate(CurrentLine, ' ', X2C(9))
        Parse Var CurrentLine Define Macro '"' Value '"' .
        Parse Upper Var Define Define
        Define=Space(Define, 0)
        Parse Upper Var Macro Macro
        Macro=Space(Macro, 0)
                                        /* Test for the macros we expect */
        If (Define="#DEFINE") Then Do
            Select
            When (Macro="BLDLEVEL_VENDOR") Then Do
                BldLevel_Vendor=Value
            End
            When (Macro="BLDLEVEL_VERSION") Then Do
                BldLevel_Version=Value
            End
            When (Macro="BLDLEVEL_INFO") Then Do
                BldLevel_Info=Value
            End
            Otherwise
            End
        End
        CurrentLine=LineIn(HeaderFile)
                                        /* As Lines() returns 0 after having read the last line
                                           we have to ensure that this last line will be processed too */
        If (Lines(HeaderFile)=0) Then
            LinesLeft=LinesLeft-1
    End
    If (BldLevel_Vendor\="" & BldLevel_Version\="" & BldLevel_Info\="") Then
        Return 0
    Else
        Return 10

/*--------------------------------------------------------------------------------------*\
 * Parse a C/C++ module definition file to test if this file is used to build a OS/2    *
 * EXE or DLL file and if it contains a DESCRIPTION line.                               *
 * Req:                                                                                 *
 *      none                                                                            *
 * Returns:                                                                             *
 *      ReturnCode .... Error code (0 if no error)                                      *
\*--------------------------------------------------------------------------------------*/
ParseDefinitionFile:
                                        /* Read ahead 1st line */
    Command=Stream(DefinitionFile, 'C', 'OPEN READ')
    LinesLeft=2
    CurrentLine=LineIn(DefinitionFile)
                                        /* Do while there are lines in the file */
    Do While(LinesLeft>0)
                                        /* Parse for constructs of the form:
                                           NAME/LIBRARY ModuleName Options */
                                        /* Kill that ... tabs */
        CurrentLine=Translate(CurrentLine, ' ', X2C(9))
        Parse Var CurrentLine Statement Module Options
        Parse Upper Var Statement Statement
        Statement=Space(Statement, 0)
        Module=Space(Module, 0)
        If (Module\="") Then Do
            If (Statement="NAME") Then 
                ModuleName=Module||".exe"
            If (Statement="LIBRARY") Then
                ModuleName=Module||".dll"
        End
                                        /* See if there is a DESCRIPTION statement */
        Parse Var CurrentLine Statement Options
        Parse Upper Var Statement Statement
        Statement=Space(Statement, 0)
        If (Statement="DESCRIPTION") Then
            DescriptionStatement=Options
        CurrentLine=LineIn(DefinitionFile)
                                        /* As Lines() returns 0 after having read the last line
                                           we have to ensure that this last line will be processed too */
        If (Lines(DefinitionFile)=0) Then
            LinesLeft=LinesLeft-1
    End
                                        /* Close the stream */
    Command=Stream(DefinitionFile, 'C', 'CLOSE')
    If (ModuleName\="" & DescriptionStatement\="") Then
        Return 0
    Else
        Return 20

/*--------------------------------------------------------------------------------------*\
 * Parse a C/C++ module definition file to modify the DESCRIPTION statement into a form *
 * that is readable by BLDLEVEL (the original gets discarded).                          *
 * Req:                                                                                 *
 *      none                                                                            *
 * Returns:                                                                             *
 *      ReturnCode .... Error code (0 if no error)                                      *
\*--------------------------------------------------------------------------------------*/
ModifyDefinitionFile:
                                        /* Read and write ahead 1st line */
    Command=Stream(DefinitionFile, 'C', 'OPEN WRITE')
    If (Command="ERROR") Then 
        Return 30
    Command=Stream(DefinitionCopy, 'C', 'OPEN READ')
    If (Command="ERROR") Then 
        Return 31
    LinesLeft=2
    CurrentLine=LineIn(DefinitionCopy)
                                        /* Do while there are lines in the file */
    Do While(LinesLeft>0)
                                        /* Parse for constructs of the form:
                                           DESCRIPTION Options */
                                        /* Kill that ... tabs */
        CurrentLine=Translate(CurrentLine, ' ', X2C(9))
        Parse Var CurrentLine Statement Options
        Parse Upper Var Statement Statement
        Statement=Space(Statement, 0)
        If (Statement="DESCRIPTION") Then Do
            Command=LineOut(DefinitionFile, Description)
        End
        Else Do
            Command=LineOut(DefinitionFile, CurrentLine)
        End
        CurrentLine=LineIn(DefinitionCopy)
                                        /* As Lines() returns 0 after having read the last line
                                           we have to ensure that this last line will be processed too */
        If (Lines(DefinitionCopy)=0) Then
            LinesLeft=LinesLeft-1
    End
                                        /* Close the streams */
    Command=Stream(DefinitionFile, 'C', 'CLOSE')
    Command=Stream(DefinitionCopy, 'C', 'CLOSE')
    Return 0

/*--------------------------------------------------------------------------------------*\
 * Signal Handler that avoids interruption and just tells user that CTRL+BRK is not     *
 * allowed currently.                                                                   *
 * Req:                                                                                 *
 *      none                                                                            *
 * Returns:                                                                             *
 *      none                                                                            *
\*--------------------------------------------------------------------------------------*/
SignalHandler:
    Say "    CTRL+BRK currently disabled"
    Return 0

