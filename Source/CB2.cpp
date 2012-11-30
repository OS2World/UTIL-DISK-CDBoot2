/***********************************************************************\
 *                              CDBoot/2                               *
 *              Copyright (C) by Stangl Roman, 2000, 2002              *
 * This Code may be freely distributed, provided the Copyright isn't   *
 * removed, under the conditions indicated in the documentation.       *
 *                                                                     *
 * CD2.cpp      Code DLL main module                                   *
 *                                                                     *
\***********************************************************************/

#pragma strings(readonly)

#include    <stdlib.h>
#include    <conio.h>
#include    <ctype.h>

                                        /* User include files */
#include    "CDBoot2.hpp"                   
#include    "UStartup.hpp"
#include    "CB2.hpp"
#include    "CB2.h"

#define         _FILE_  "CB2.cpp "##BLDLEVEL_VERSION
static char     _VERSION_[]=_FILE_;
#define         __THREAD_1__            "CB2"

                                        /* Pointer to the Logo window passed. Will be used
                                           as an anchor to report progress, errors, ... */
UAnchor    *puAnchor;
UStartupParameters *puSP;

MRESULT  EXPENTRY CB2_DialogProc(HWND hwndDlg, ULONG msg, MPARAM mp1, MPARAM mp2);

/*--------------------------------------------------------------------------------------*\
 * This entrypoint will be exported as ordinal 1 to allow a version check.              *
 * Req:                                                                                 *
 * Ret:                                                                                 *
 * Ref:                                                                                 *
\*--------------------------------------------------------------------------------------*/
ULONG   ulSignature=BLDLEVEL_SIGNATURE;

/*--------------------------------------------------------------------------------------*\
 * This entrypoint will be exported as ordinal 2 and is the main entrypoint which will  *
 * be called from a (non-PM) thread created by the caller.                              *
 * Req:                                                                                 *
 * Ret:                                                                                 *
 *      CB2ERR_* return code                                                            *
 * Ref:                                                                                 *
\*--------------------------------------------------------------------------------------*/
ULONG _System   CB2Main(UStartupParameters *puStartupParameters)
{
    puAnchor=puStartupParameters->puAnchor;
    DEBUG_L2(__THREAD_1__, "");
    UAnchor    *puAnchorCopy=0;         /* Reference to our anchor object */
    CD2Boot    *pCD2Boot=0;             /* Our CD2BOOT object that patches the boot sector */
    char       *pcOption;               /* Commandline option */
    ULONG       ulParameterError=FALSE; /* TRUE when invalid commandline option was detected */
    ULONG       ulParameterErrorTemp;
    UCHAR       ucDrive=' ';            /* Drive to install CD2BOOT into specified on commandline */
                                        /* Time period for decission to be patched into CD2BOOT 
                                           specified on commandline */
    USHORT      usSeconds=0;
                                        /* Drive and/or Timout commandline parameter
                                           specified was invalid */
    ULONG       ulDriveTimeoutInvalid=FALSE;
                                        /* Boot media specified on commandline was invalid */
    ULONG       ulBootMediaInvalid=FALSE;
                                        /* One or more Custom messages specified on
                                           commandlien were invalid */
    ULONG       ulCustomMessageInvalid=FALSE;
                                        /* Password specified on commandline was invalid */ 
    ULONG       ulPasswordInvalid=FALSE;
    UCHAR       aucDrive[2];            /* Drive as string */
    UCHAR       aucSeconds[3];          /* Time period as string */
    ULONG       ulTemp;
                                        /* Return code */
    ULONG       ulReturnCode=CB2ERR_NO_ERROR;
#ifdef  __PM__
                                        /* CDBOOTPM user interface dialog */
    HAB         habDialog;
    HMQ         hmqDialog;
    HWND        hwndDialog;
                                        /* Accumulated error messages while checking commandline
                                           options and their parameters */
    char       *pcErrorMessage=0;         
    ERRORID     errorId;
#endif  /* __PM__ */
                                        /* Work buffer */
    char        acBuffer[CCHMAXPATH];
    ULONG       ulDialogResult=DID_CANCEL;
#ifdef  DEBUG
#ifdef  __PM__
    printf("%s (PM): %s DLL Build %s %s\n", BLDLEVEL_INFO, BLDLEVEL_PRODUCT, __DATE__, __TIME__);
#endif  /* __PM__ */
#ifdef  __AVIO__
    printf("%s (AVIO): %s DLL Build %s %s\n", BLDLEVEL_INFO, BLDLEVEL_PRODUCT, __DATE__, __TIME__);
#endif  /* __AVIO__ */
    printf("BOOTSECTOR size: %d\n", sizeof(*(BOOTSECTOR *)0));
#endif  /* DEBUG */
                                        /* Save anchor object to (startup) Logo window */
    puSP=puStartupParameters;
    puAnchorCopy=new UAnchor(puStartupParameters);
                                        /* Startup message */
    puAnchorCopy->displayMessage(puAnchorCopy->loadMessage(NLSMSG_INITIALIZECB2), TRUE);
    DosSleep(INITIALIZATION_DELAY);
                                        /* Create and initialized our CD2BOOT installation 
                                           object */
    pCD2Boot=new CD2Boot(puAnchorCopy);
                                        /* Check for /CID commandline parameter which
                                           should be followed by filename to retrieve
                                           the commandline options from */
    if((pcOption=puAnchorCopy->checkCommandlineOption("CID", TRUE))!=0)
        {
        if(pcOption!=(char *)0xFFFFFFFF)
            {
                                        /* If we have a parameter, check if it is plausible */
            while(TRUE)
                {
                if(strlen(pcOption)>=CCHMAXPATH)
                    {
                    ulParameterErrorTemp=TRUE;
                    break;
                    }
                                        /* We found a commandline option and a parameter */
                pCD2Boot->SetStatus(STATUS_CID_MODECID, (UCHAR *)pcOption, 0);
                break;
                }
            }
        }
    if(pCD2Boot->GetStatus(STATUS_CID_MODECID))
        puAnchorCopy->setUserCommandline(pcOption);
                                        /* Check commandline parameters */
    if((pcOption=puAnchorCopy->checkCommandlineOption("Drive", TRUE))!=0)
        {
                                        /* We found a commandline option (at least, possibly
                                           we'll also find a parameter) */
        pCD2Boot->SetStatus(STATUS_CID_DRIVE, NULL, 0);
        ulParameterErrorTemp=FALSE;
                                        /* If just /Drive was specified, but not d 
                                           afterwards, we have a problem */
        if(pcOption==(char *)0xFFFFFFFF)
            {
                                        /* If parameter of option is missing we have a problem */
            ulParameterErrorTemp=TRUE;
            }
        else
            {
                                        /* If we have a parameter, check if it is plausible */
            while(TRUE)
                {
                if(strlen(pcOption)>1)
                    {
                    ulParameterErrorTemp=TRUE;
                    break;
                    }
                if((pcOption[0]<'A') || 
                    (pcOption[0]>'z') ||
                    ((pcOption[0]>'Z') && (pcOption[0]<'a')))
                    {
                    ulParameterErrorTemp=TRUE;
                    break;
                    }
                ucDrive=pcOption[0];
                                        /* We found a commandline option and a parameter */
                pCD2Boot->SetStatus(STATUS_CID_DRIVE, &ucDrive, 0);
                strcpy((char *)aucDrive, pcOption);
                break;
                }    
            }
                                        /* In case of an error, display or save the error
                                           messages and flag that we now are in an error
                                           state (once more) */
        if(ulParameterErrorTemp==TRUE)
            {
            ucDrive=' ';
#ifdef  __AVIO__
            puAnchorCopy->displayMessage(puAnchorCopy->loadMessage(NLSMSG_SYNTAX_AVIO), TRUE);
            puAnchorCopy->displayMessage(puAnchorCopy->loadMessage(NLSMSG_ERR_SYNTAX_DRIVE), TRUE);
            DosSleep(INITIALIZATION_DELAY);
#endif  /* __AVIO__ */
#ifdef  __PM__
            pcErrorMessage=puAnchorCopy->concatStrings(
                puAnchorCopy->loadMessage(NLSMSG_SYNTAX_PM),
                puAnchorCopy->loadMessage(NLSMSG_ERR_SYNTAX_DRIVE));
#endif  /* __PM__ */
            ulParameterError|=TRUE;
            }
        }
    if((pcOption=puAnchorCopy->checkCommandlineOption("Timeout", TRUE))!=0)
        {
                                        /* We found a commandline option (at least, possibly
                                           we'll also find a parameter) */
        pCD2Boot->SetStatus(STATUS_CID_TIMEOUT, NULL, 0);
        ulParameterErrorTemp=FALSE;
                                        /* If just /Timeout was specified, but not tt in
                                           the range 1 ... 60 afterwards, we have a problem */
        if(pcOption==(char *)0xFFFFFFFF)
            {
                                        /* If parameter of option is missing we have a problem */
            ulParameterErrorTemp=TRUE;
            }
        else
            {
                                        /* If we have a parameter, check if it is plausible */
            while(TRUE)
                {
                if(strlen(pcOption)>2)
                    {
                    ulParameterErrorTemp=TRUE;
                    break;
                    }
                if((!isdigit(pcOption[0])) || 
                    ((pcOption[1]!='\0') && (!isdigit(pcOption[1]))))
                    {
                    ulParameterErrorTemp=TRUE;
                    break;
                    }
                usSeconds=(pcOption[0]-'0');
                if(pcOption[1]!='\0')
                    usSeconds=usSeconds*10+(pcOption[1]-'0');
                if((usSeconds<DELAY_MINIMUM) || (usSeconds>DELAY_MAXIMUM))
                    {
                    ulParameterErrorTemp=TRUE;
                    break;
                    }
                                        /* We found a commandline option and a parameter */
                pCD2Boot->SetStatus(STATUS_CID_TIMEOUT, NULL, usSeconds);
                strcpy((char *)aucSeconds, pcOption);
                break;
                }    
            }
                                        /* In case of an error, display or save the error
                                           messages and flag that we now are in an error
                                           state (once more) */
        if(ulParameterErrorTemp==TRUE)
            {
            usSeconds=0;
#ifdef  __AVIO__
            if(ulParameterError==FALSE)
                puAnchorCopy->displayMessage(puAnchorCopy->loadMessage(NLSMSG_SYNTAX_AVIO), TRUE);
            puAnchorCopy->displayMessage(puAnchorCopy->loadMessage(NLSMSG_ERR_SYNTAX_TIMEOUT), TRUE);
            DosSleep(INITIALIZATION_DELAY);
#endif  /* __AVIO__ */
#ifdef  __PM__
            if(ulParameterError==FALSE)
                pcErrorMessage=puAnchorCopy->concatStrings(
                    puAnchorCopy->loadMessage(NLSMSG_SYNTAX_PM),
                    puAnchorCopy->loadMessage(NLSMSG_ERR_SYNTAX_TIMEOUT));
            else
                pcErrorMessage=puAnchorCopy->concatStrings(
                    pcErrorMessage,
                    puAnchorCopy->loadMessage(NLSMSG_ERR_SYNTAX_TIMEOUT));
#endif  /* __PM__ */
            ulParameterError|=TRUE;
            }
        }
    if((pcOption=puAnchorCopy->checkCommandlineOption("Harddisk", TRUE))!=0)
        {
        pCD2Boot->SetStatus(STATUS_CID_HARDDISK, NULL, 0);
        }
    if((pcOption=puAnchorCopy->checkCommandlineOption("Removeable", TRUE))!=0)
        {
        pCD2Boot->SetStatus(STATUS_CID_REMOVEABLE, NULL, 0);
        }
    if((pcOption=puAnchorCopy->checkCommandlineOption("BM", TRUE))!=0)
        {
        pCD2Boot->SetStatus(STATUS_CID_BMBOOT, NULL, 0);
        }
                                        /* Check for undocumented IBM modus */
    if((pcOption=puAnchorCopy->checkCommandlineOption("IBM", TRUE))!=0)
        {
        pCD2Boot->SetStatus(STATUS_CID_MODEIBM, NULL, 0);
        }
                                        /* In case of an error, display or save the error
                                           messages and flag that we now are in an error
                                           state (once more) */
    if((pCD2Boot->GetStatus(STATUS_CID_HARDDISK|STATUS_CID_REMOVEABLE))==
        (STATUS_CID_HARDDISK|STATUS_CID_REMOVEABLE))
        {
#ifdef  __AVIO__
        if(ulParameterError==FALSE)
            puAnchorCopy->displayMessage(puAnchorCopy->loadMessage(NLSMSG_SYNTAX_AVIO), TRUE);
        puAnchorCopy->displayMessage(puAnchorCopy->loadMessage(NLSMSG_ERR_SYNTAX_PARAMETERS2), TRUE);
        DosSleep(INITIALIZATION_DELAY);
#endif  /* __AVIO__ */
#ifdef  __PM__
        if(ulParameterError==FALSE)
            pcErrorMessage=puAnchorCopy->concatStrings(
                puAnchorCopy->loadMessage(NLSMSG_SYNTAX_PM),
                puAnchorCopy->loadMessage(NLSMSG_ERR_SYNTAX_PARAMETERS2));
        else
            pcErrorMessage=puAnchorCopy->concatStrings(
                pcErrorMessage,
                puAnchorCopy->loadMessage(NLSMSG_ERR_SYNTAX_PARAMETERS2));
#endif  /* __PM__ */
        ulBootMediaInvalid=TRUE;
        ulParameterError|=TRUE;
        }
    if((pcOption=puAnchorCopy->checkCommandlineOption("Clear", TRUE))!=0)
        {
                                        /* We found a commandline option */
        pCD2Boot->SetStatus(STATUS_CID_CLEARSCREEN, NULL, 0);
        }
    if((pcOption=puAnchorCopy->checkCommandlineOption("Password", TRUE))!=0)
        {
                                        /* We found a commandline option (at least, possibly
                                           we'll also find a parameter) */
        pCD2Boot->SetStatus(STATUS_CID_PASSWORD, NULL, 0);
        ulParameterErrorTemp=FALSE;
                                        /* If just /Password was specified, but no password
                                           afterwards, we have a problem */
        if(pcOption==(char *)0xFFFFFFFF)
            {
                                        /* If parameter of option is missing we have a problem */
            ulParameterErrorTemp=TRUE;
            }
        else
            {
                                        /* If we have a parameter, check if it is plausible */
            while(TRUE)
                {
                if(strlen(pcOption)>=PWDSIZE)
                    ulParameterErrorTemp=TRUE;
                for(ulTemp=0; pcOption[ulTemp]!='\0'; ulTemp++)
                    {
                    UCHAR   ucPassword=pcOption[ulTemp];

                    if(((ucPassword<'0') || (ucPassword>'9')) &&
                        ((ucPassword<'A') || (ucPassword>'Z')) &&
                        ((ucPassword<'a') || (ucPassword>'z')))
                        {
                        ulParameterErrorTemp=TRUE;
                        break;
                        }
                    }
                if(ulParameterErrorTemp==TRUE)
                    break;
                                        /* We found a commandline option and a parameter */
                pCD2Boot->SetStatus(STATUS_CID_PASSWORD, (UCHAR *)pcOption, 0);
                break;
                }    
            }
                                        /* In case of an error, display or save the error
                                           messages and flag that we now are in an error
                                           state (once more) */
        if(ulParameterErrorTemp==TRUE)
            {
            ulPasswordInvalid=TRUE;
#ifdef  __AVIO__
            if(ulParameterError==FALSE)
                puAnchorCopy->displayMessage(puAnchorCopy->loadMessage(NLSMSG_SYNTAX_AVIO), TRUE);
            puAnchorCopy->displayMessage(puAnchorCopy->loadMessage(NLSMSG_ERR_SYNTAX_PASSWORD), TRUE);
            DosSleep(INITIALIZATION_DELAY);
#endif  /* __AVIO__ */
#ifdef  __PM__
            if(ulParameterError==FALSE)
                pcErrorMessage=puAnchorCopy->concatStrings(
                    puAnchorCopy->loadMessage(NLSMSG_SYNTAX_PM),
                    puAnchorCopy->loadMessage(NLSMSG_ERR_SYNTAX_PASSWORD));
            else
                pcErrorMessage=puAnchorCopy->concatStrings(
                    pcErrorMessage,
                    puAnchorCopy->loadMessage(NLSMSG_ERR_SYNTAX_PASSWORD));
#endif  /* __PM__ */
            ulParameterError|=TRUE;
            }
        }
    ulParameterErrorTemp=FALSE;
                                        /* Check for /MSG commandline parameters */
    for(ulTemp=1; ulTemp<=3; ulTemp++)
        {
        char   *acMsg[]={ "Msg1", "Msg2", "Msg3" };

        if((pcOption=puAnchorCopy->checkCommandlineOption(acMsg[ulTemp-1], TRUE))!=0)
            {
                                        /* If just /Msg* was specified, but not message 
                                           afterwards, we have a problem */
            if(pcOption==(char *)0xFFFFFFFF)
                {
                                        /* If parameter of option is missing we have a problem */
                ulParameterErrorTemp=TRUE;
                }
            else
                {
                                        /* If we have a parameter, check if it is plausible */
                while(TRUE)
                    {
                    if(strlen(pcOption)>=LINESIZE)
                        {
                        ulParameterErrorTemp=TRUE;
                        break;
                        }
                                        /* We found a commandline option and a parameter */
                    pCD2Boot->SetStatus(STATUS_CID_CUSTOMMESSAGE, (UCHAR *)pcOption, ulTemp);
                    break;
                    }    
                }
            }
        }
                                        /* Check for /MSGCOLOR commandline parameters */
    if((pcOption=puAnchorCopy->checkCommandlineOption("MsgColor", TRUE))!=0)
        {
                                        /* If just /MsgColor was specified, but not message 
                                           afterwards, we have a problem */
        if(pcOption!=(char *)0xFFFFFFFF)
            {
                                        /* We found a commandline option and a parameter */
            pCD2Boot->SetStatus(STATUS_CID_COLORCUSTOM, (UCHAR *)pcOption, 0);
            }
        }
                                        /* Check for /CPY commandline parameters, but only
                                           if we're running in the /IBM mode */
    if(pCD2Boot->GetStatus(STATUS_CID_MODEIBM)) for(ulTemp=1; ulTemp<=3; ulTemp++)
        {
        char   *acMsg[]={ "Cpy1", "Cpy2", "Cpy3" };

        if((pcOption=puAnchorCopy->checkCommandlineOption(acMsg[ulTemp-1], TRUE))!=0)
            {
                                        /* If just /Cpy1 was specified, but not message 
                                           afterwards, we ignore that, as the default
                                           copyright message will be used then */
            if(pcOption!=(char *)0xFFFFFFFF)
                {
                while(TRUE)
                    {
                    if(strlen(pcOption)>=LINESIZE)
                        break;
                                        /* We found a commandline option and a parameter */
                    pCD2Boot->SetStatus(STATUS_CID_MODEIBM, (UCHAR *)pcOption, ulTemp);
                    break;
                    }    
                }
            }
        }
                                        /* Check for /CPYCOLOR commandline parameters */
    if((pcOption=puAnchorCopy->checkCommandlineOption("CpyColor", TRUE))!=0)
        {
                                        /* If just /MsgColor was specified, but not message 
                                           afterwards, we have a problem */
        if(pcOption!=(char *)0xFFFFFFFF)
            {
                                        /* We found a commandline option and a parameter */
            pCD2Boot->SetStatus(STATUS_CID_COLORCOPYRIGHT, (UCHAR *)pcOption, 0);
            }
        }
                                        /* In case of an error, display or save the error
                                           messages and flag that we now are in an error
                                           state (once more) */
    if(ulParameterErrorTemp==TRUE)
        {
        ulCustomMessageInvalid=TRUE;
#ifdef  __AVIO__
        if(ulParameterError==FALSE)
            puAnchorCopy->displayMessage(puAnchorCopy->loadMessage(NLSMSG_SYNTAX_AVIO), TRUE);
        puAnchorCopy->displayMessage(puAnchorCopy->loadMessage(NLSMSG_ERR_SYNTAX_CUSTOMMESSAGE), TRUE);
        DosSleep(INITIALIZATION_DELAY);
#endif  /* __AVIO__ */
#ifdef  __PM__
        if(ulParameterError==FALSE)
            pcErrorMessage=puAnchorCopy->concatStrings(
                puAnchorCopy->loadMessage(NLSMSG_SYNTAX_PM),
                puAnchorCopy->loadMessage(NLSMSG_ERR_SYNTAX_CUSTOMMESSAGE));
        else
            pcErrorMessage=puAnchorCopy->concatStrings(
                pcErrorMessage,
                puAnchorCopy->loadMessage(NLSMSG_ERR_SYNTAX_CUSTOMMESSAGE));
#endif  /* __PM__ */
        ulParameterError|=TRUE;
        }
                                        /* If only one or more parameters were specified, but
                                           they were not valid, the user made a mistake.
                                           Display the (possibly accumualted) error messages */
    if(pCD2Boot->GetStatus(
        STATUS_CID_DRIVE|STATUS_CID_TIMEOUT|
        STATUS_CID_HARDDISK|STATUS_CID_REMOVEABLE|
        STATUS_CID_CUSTOMMESSAGE|STATUS_CID_PASSWORD)!=0)
        {
        if((pCD2Boot->GetStatus(STATUS_CID_DRIVE|STATUS_CID_TIMEOUT)!=0) &&
            ((ucDrive==' ') || (usSeconds==0)))
        ulDriveTimeoutInvalid=TRUE;
        if((ulDriveTimeoutInvalid==TRUE) ||
            (ulBootMediaInvalid==TRUE) ||
            (ulCustomMessageInvalid==TRUE) ||
            (ulPasswordInvalid==TRUE))
            {
            if(ulDriveTimeoutInvalid==TRUE)
                {
#ifdef  __AVIO__
                if(ulParameterError==FALSE)
                    puAnchorCopy->displayMessage(puAnchorCopy->loadMessage(NLSMSG_SYNTAX_AVIO), TRUE);
                puAnchorCopy->displayMessage(puAnchorCopy->loadMessage(NLSMSG_ERR_SYNTAX_PARAMETERS1), TRUE);
#endif  /* __AVIO__ */
#ifdef  __PM__
                if(ulParameterError==FALSE)
                    puAnchorCopy->displayMessage(
                        puAnchorCopy->concatStrings(
                            puAnchorCopy->loadMessage(NLSMSG_SYNTAX_PM),
                            puAnchorCopy->loadMessage(NLSMSG_ERR_SYNTAX_PARAMETERS1)),
                        FALSE);
                else
                    puAnchorCopy->displayMessage(
                        puAnchorCopy->concatStrings(
                            pcErrorMessage,
                            puAnchorCopy->loadMessage(NLSMSG_ERR_SYNTAX_PARAMETERS1)),
                        FALSE);
#endif  /* __PM__ */
                }
            else
                {
#ifdef  __AVIO__
                if(ulParameterError==FALSE)
                    puAnchorCopy->displayMessage(puAnchorCopy->loadMessage(NLSMSG_SYNTAX_AVIO), TRUE);
#endif  /* __AVIO__ */
#ifdef  __PM__
                if(ulParameterError==FALSE)
                    puAnchorCopy->displayMessage(
                        puAnchorCopy->loadMessage(NLSMSG_SYNTAX_PM), FALSE);
                else
                    puAnchorCopy->displayMessage(pcErrorMessage, FALSE);
#endif  /* __PM__ */
                }
            DosSleep(INITIALIZATION_DELAY);
            ulParameterError=TRUE;
            }
        else
            {
#ifdef  __AVIO__
            puAnchorCopy->displayMessage(
                puAnchorCopy->loadMessage(NLSMSG_CID_COMMANDLINE_OK, (char *)aucDrive, (char *)aucSeconds), TRUE);
#endif  /* __AVIO__ */
#ifdef  __PM__
            puAnchorCopy->displayMessage(
                puAnchorCopy->loadMessage(NLSMSG_CID_COMMANDLINE_OK, (char *)aucDrive, (char *)aucSeconds), TRUE);
#endif  /* __PM__ */
            DosSleep(INITIALIZATION_DELAY);
            }
        }
    if(ulParameterError==FALSE)
        {
                                        /* If no commandline parameters are specified,
                                           then load the default values */
        if(pCD2Boot->GetStatus(STATUS_CID_HARDDISK|STATUS_CID_REMOVEABLE)==0)
            pCD2Boot->SetStatus(STATUS_CID_HARDDISK, NULL, 0);
#ifdef  __PM__
                                        /* If we already have the parameters we need
                                           (e.g. due to a CID installation) then
                                           skip dialog */
        if((pCD2Boot->GetStatus(STATUS_CID_DRIVE|STATUS_CID_TIMEOUT))!=
            (STATUS_CID_DRIVE|STATUS_CID_TIMEOUT))
            {
                                        /* For CDBOOTPM, display the dialog to prompt the
                                           user for his preferences */
            try {
                                        /* Startup PM for this thread also */
                habDialog=WinInitialize(0);
                if(habDialog==NULLHANDLE)
                    throw((ULONG)NLSMSG_ERR_HABDIALOG);
                hmqDialog=WinCreateMsgQueue(habDialog, 0);    
                if(hmqDialog==NULLHANDLE)
                    throw((ULONG)NLSMSG_ERR_HMQDIALOG);
                                        /* Load and process dialog window's WM_INITDLG
                                           message */
                hwndDialog=WinLoadDlg(HWND_DESKTOP, puAnchorCopy->puStartupParameters->hwndFrame,
                    CB2_DialogProc, NULLHANDLE, CB2_DIALOG, (PVOID)pCD2Boot);
                if(hwndDialog==NULLHANDLE)
                    throw((ULONG)NLSMSG_ERR_HWNDDIALOG);
                                        /* Process our dialog window (while the logo
                                           is still showing, as it displays the progress
                                           messages) */    
                ulDialogResult=WinProcessDlg(hwndDialog);
                                        /* The following code would allow us to display
                                           a dialog another time by removing the flag for
                                           dismissed dialogs 
                                           WinSetWindowUShort(hwndDialog, QWS_FLAGS, 
                                               WinQueryWindowUShort(hwndDialog, QWS_FLAGS) & (~FF_DLGDISMISSED));
                                           WinShowWindow(hwndDialog, TRUE);
                                           WinProcessDlg(hwndDialog);
                                           */
                                        /* Close down PM environment */
                WinDestroyWindow(hwndDialog);
                WinDestroyMsgQueue(hmqDialog);
                WinTerminate(habDialog);
                }
            catch(ULONG ulErrorMessage)
                {
                                        /* In case of errors, display a message box in front
                                           of the logo window */
                errorId=WinGetLastError(habDialog);
                puAnchorCopy->displayMessage(puAnchorCopy->loadMessageHex(ulErrorMessage, errorId), FALSE);
                }
            }
        else
            {
                                        /* For a CID environment just simulate that 
                                           the user has clicked ok */
            puAnchorCopy->displayMessage(
                puAnchorCopy->loadMessage(NLSMSG_CID_UNATTENDED), TRUE);
            DosSleep(INITIALIZATION_DELAY);
            ulDialogResult=DID_OK;
            }
#endif  /* __PM__ */
#ifdef  __AVIO__
                                        /* For CDBOOT, ask the user interactive for his
                                           preferences */
        pCD2Boot->CB2_Interactive();
        ulDialogResult=DID_OK;
#endif  /* __AVIO__ */
                                        /* Now that we know what the user wants do install
                                           CD2BOOT accordingly */
        if(ulDialogResult==DID_OK)
            {
            if(pCD2Boot->InstallCD2Boot()!=NO_ERROR)
                ulReturnCode=CB2ERR_FATAL;
            }
        }
    else
        ulReturnCode=CB2ERR_WARNING;
                                        /* Clean up CD2BOOT object */
    delete pCD2Boot;
                                        /* Clear possible error message left over
                                           from initialization */
#ifdef  __PM__
    puAnchorCopy->displayMessage(puAnchorCopy->loadMessageStrip(NLSMSG_TERMINATECB2), TRUE);
#endif  /* __PM__ */
#ifdef  __AVIO__
    puAnchorCopy->displayMessage(puAnchorCopy->loadMessage(NLSMSG_TERMINATECB2), TRUE);
#endif  /* __AVIO__ */
    DosSleep(INITIALIZATION_DELAY);
                                        /* Cleanup */
    delete puAnchorCopy;
    puAnchorCopy=0;
#ifdef  DEBUG
                                        /* Check for memory leaks */
    CHECK_MEMORY(__THREAD_1__);
#endif  /* DEBUG */
    return(ulReturnCode);
}

/****************************************************************************************\
 * Class: CD2Boot                                                                       *
\****************************************************************************************/
/*--------------------------------------------------------------------------------------*\
 * CD2Boot: Constructor(s)                                                              *
\*--------------------------------------------------------------------------------------*/
CD2Boot::CD2Boot(UAnchor *pucAnchor) :
         ulStatusFlag(0),
         ulOS2BOOTOffset(0),
         pbCD2BOOT(0),
         ulCD2BOOTSize(0),
         ulBootSectorOffset(0),
         ulBootControlOffset(0),
         pBootControl(0),
         pErrorControl(0),
         puAnchorCopy(puAnchor),
         ucDrive('A'),
         usSeconds(DELAY_DEFAULT),
         ulRemoveableMap(0)
{
    DEBUG_L3(__THREAD_1__, "");
                                        /* A OS/2 bootsector of a removeable
                                           FAT media loading OS2BOOT (which
                                           will be modified to load CD2BOOT
                                           instead) */
    BYTE        abBootsectorRemoveableOS2[]=
        {
                                        /* .D.IBM 4.50..... */
        0xEB, 0x44, 0x90, 0x49, 0x42, 0x4D, 0x20, 0x34, 0x2E, 0x35, 0x30, 0x00, 0x02, 0x01, 0x01, 0x00,   
                                        /* ...@............ */
        0x02, 0xE0, 0x00, 0x40, 0x0B, 0xF0, 0x09, 0x00, 0x12, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
                                        /* @.....)..kcNO NA */
        0x40, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x29, 0x15, 0xB0, 0x6B, 0x63, 0x4E, 0x4F, 0x20, 0x4E, 0x41,   
                                        /* ME    FAT     .. */
        0x4D, 0x45, 0x20, 0x20, 0x20, 0x20, 0x46, 0x41, 0x54, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00,   
                                        /* .......3.....{.. */
        0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0xFA, 0x33, 0xDB, 0x8E, 0xD3, 0xBC, 0xFF, 0x7B, 0xFB, 0xBA,   
                                        /* .........&...... */
        0xC0, 0x07, 0x8E, 0xDA, 0xA0, 0x10, 0x00, 0x98, 0xF7, 0x26, 0x16, 0x00, 0x03, 0x06, 0x0E, 0x00,   
                                        /* P.. ..&........H */
        0x50, 0x91, 0xB8, 0x20, 0x00, 0xF7, 0x26, 0x11, 0x00, 0x8B, 0x1E, 0x0B, 0x00, 0x03, 0xC3, 0x48,   
                                        /* ..P...>......3.Y */
        0xF7, 0xF3, 0x50, 0x03, 0xC1, 0xA3, 0x3E, 0x00, 0xB8, 0x00, 0x10, 0x8E, 0xC0, 0x33, 0xFF, 0x59,   
                                        /* ..D.X.B.3..s.3.. */
        0x89, 0x0E, 0x44, 0x00, 0x58, 0xA3, 0x42, 0x00, 0x33, 0xD2, 0xE8, 0x73, 0x00, 0x33, 0xDB, 0x8B,   
                                        /* .....Q........Yt */
        0x0E, 0x11, 0x00, 0x8B, 0xFB, 0x51, 0xB9, 0x0B, 0x00, 0xBE, 0xD9, 0x01, 0xF3, 0xA6, 0x59, 0x74,   
                                        /* ... ...5&.G.&.W. */
        0x05, 0x83, 0xC3, 0x20, 0xE2, 0xED, 0xE3, 0x35, 0x26, 0x8B, 0x47, 0x1C, 0x26, 0x8B, 0x57, 0x1E,   
                                        /* .6......&.W.JJ.. */
        0xF7, 0x36, 0x0B, 0x00, 0xFE, 0xC0, 0x8A, 0xC8, 0x26, 0x8B, 0x57, 0x1A, 0x4A, 0x4A, 0xA0, 0x0D,   
                                        /* .2.....>........ */
        0x00, 0x32, 0xE4, 0xF7, 0xE2, 0x03, 0x06, 0x3E, 0x00, 0x83, 0xD2, 0x00, 0xBB, 0x00, 0x08, 0x8E,   
                                        /* .3..W.(..6...... */
        0xC3, 0x33, 0xFF, 0x06, 0x57, 0xE8, 0x28, 0x00, 0x8D, 0x36, 0x0B, 0x00, 0xCB, 0xBE, 0x9C, 0x01,   
                                        /* ................ */
        0xEB, 0x03, 0xBE, 0xB1, 0x01, 0xE8, 0x09, 0x00, 0xBE, 0xC6, 0x01, 0xE8, 0x03, 0x00, 0xFB, 0xEB,   
                                        /* ....t........... */
        0xFE, 0xAC, 0x0A, 0xC0, 0x74, 0x09, 0xB4, 0x0E, 0xBB, 0x07, 0x00, 0xCD, 0x10, 0xEB, 0xF2, 0xC3,   
                                        /* PRQ........h.0.. */
        0x50, 0x52, 0x51, 0x03, 0x06, 0x1C, 0x00, 0x13, 0x16, 0x1E, 0x00, 0x68, 0x00, 0x30, 0x0F, 0xA1,   
                                        /* df.>..I13X..M.P. */
        0x64, 0x66, 0x81, 0x3E, 0x00, 0x00, 0x49, 0x31, 0x33, 0x58, 0x0F, 0x84, 0x4D, 0x00, 0x50, 0xA1,   
                                        /* ...&....X....6.. */
        0x1A, 0x00, 0xF6, 0x26, 0x18, 0x00, 0x8B, 0xD8, 0x58, 0xF7, 0xF3, 0x92, 0xF6, 0x36, 0x18, 0x00,   
                                        /* .........*.@P... */
        0x86, 0xE0, 0x8B, 0xD8, 0xFE, 0xC3, 0xA1, 0x18, 0x00, 0x2A, 0xC3, 0x40, 0x50, 0xB4, 0x02, 0xB1,   
                                        /* ...........$.... */
        0x06, 0xD2, 0xE6, 0x0A, 0xF3, 0x8B, 0xCA, 0x86, 0xE9, 0x8A, 0x16, 0x24, 0x00, 0x8A, 0xF7, 0x8B,   
                                        /* ...r.[Y...&....Z */
        0xDF, 0xCD, 0x13, 0x72, 0x8D, 0x5B, 0x59, 0x8B, 0xC3, 0xF7, 0x26, 0x0B, 0x00, 0x03, 0xF8, 0x5A,   
                                        /* X.....*.....S..$ */
        0x58, 0x03, 0xC3, 0x83, 0xD2, 0x00, 0x2A, 0xCB, 0x7F, 0x96, 0xC3, 0x1E, 0x53, 0x8A, 0x1E, 0x24,   
                                        /* .....6...|..D..L */
        0x00, 0x0F, 0xA0, 0x1F, 0x8D, 0x36, 0x08, 0x00, 0x89, 0x7C, 0x04, 0x8C, 0x44, 0x06, 0x88, 0x4C,   
                                        /* ..D..T..D....B.. */
        0x02, 0x89, 0x44, 0x08, 0x89, 0x54, 0x0A, 0xC7, 0x44, 0x0C, 0x00, 0x00, 0xB4, 0x42, 0x8A, 0xD3,   
                                        /* ..[.r.YZX...OS/2 */
        0xCD, 0x13, 0x5B, 0x1F, 0x72, 0xBD, 0x59, 0x5A, 0x58, 0xC3, 0x12, 0x00, 0x4F, 0x53, 0x2F, 0x32,   
                                        /*  !! SYS01475.... */
        0x20, 0x21, 0x21, 0x20, 0x53, 0x59, 0x53, 0x30, 0x31, 0x34, 0x37, 0x35, 0x0D, 0x0A, 0x00, 0x12,   
                                        /* .OS/2 !! SYS0202 */
        0x00, 0x4F, 0x53, 0x2F, 0x32, 0x20, 0x21, 0x21, 0x20, 0x53, 0x59, 0x53, 0x30, 0x32, 0x30, 0x32,   
                                        /* 5.....OS/2 !! SY */
        0x35, 0x0D, 0x0A, 0x00, 0x12, 0x00, 0x4F, 0x53, 0x2F, 0x32, 0x20, 0x21, 0x21, 0x20, 0x53, 0x59,   
                                        /* S02027...OS2BOOT */
        0x53, 0x30, 0x32, 0x30, 0x32, 0x37, 0x0D, 0x0A, 0x00, 0x4F, 0x53, 0x32, 0x42, 0x4F, 0x4F, 0x54,   
                                        /*     ............ */
        0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   
                                        /* ..............U. */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0xAA    
        };
    char       *pcText;
    ULONG       ulDriveNumber;          /* Current drive (1=A, 2=B, 3=C, ...) */
                                        /* Drive map for all drives (Bit 0=A, Bit 1=B, 
                                           Bit 2=C, ...) */
    ULONG       ulLogicalDriveMap;      
    UCHAR       aucDrive[]="C:";        /* Current drive */
                                        /* Shifting bit used to remove all non removeable
                                           drives from ulRemoveableMap */
    ULONG       ulRemoveableBit;        
    ULONG       ulActionTaken;          /* Action taken on opened file (drive) */
                                        /* DSK_GETDEVICEPARAMS ioctl() packets */
    GETDEVICEPARAMS_PPF
                getdeviceparamsPPF;
    GETDEVICEPARAMS_DPF
                getdeviceparamsDPF;
    ULONG       ulParamLengthInOut;
    ULONG       ulDataLengthInOut;
    APIRET      apiretRc;               /* Return codes */

                                        /* Initialize buffers */
    memset(&bootsectorRemoveable, 0, sizeof(bootsectorRemoveable));
    usCopyrightColor=(NLSMSG_COLOR12-NLSMSG_COLOR0);
    memset(aucCopyright1, '\0', sizeof(aucCopyright1));
    memset(aucCopyright2, '\0', sizeof(aucCopyright2));
    memset(aucCopyright3, '\0', sizeof(aucCopyright3));
    pcText=puAnchorCopy->loadMessageStrip(NLSMSG_CD2BOOT_COPYRIGHT1);
    strcpy((char *)aucCopyright1, pcText);
    puAnchorCopy->allocFree(pcText);
    pcText=puAnchorCopy->loadMessageStrip(NLSMSG_CD2BOOT_COPYRIGHT2);
    strcpy((char *)aucCopyright2, pcText);
    puAnchorCopy->allocFree(pcText);
    pcText=puAnchorCopy->loadMessageStrip(NLSMSG_CD2BOOT_COPYRIGHT3);
    strcpy((char *)aucCopyright3, pcText);
    puAnchorCopy->allocFree(pcText);
    usCustomMessageColor=(NLSMSG_COLOR10-NLSMSG_COLOR0);
    memset(aucCustomMessage1, '\0', sizeof(aucCustomMessage1));
    memset(aucCustomMessage2, '\0', sizeof(aucCustomMessage2));
    memset(aucCustomMessage3, '\0', sizeof(aucCustomMessage3));
    memset(aucPassword, '\0', sizeof(aucPassword));
#ifdef  DEBUG
    if(sizeof(bootsectorRemoveableOS2)!=sizeof(abBootsectorRemoveableOS2))
        printf("Removeable OS/2 bootsector size mismatch\n");
#endif  /* DEBUG */
    memcpy(&bootsectorRemoveableOS2, abBootsectorRemoveableOS2, sizeof(BOOTSECTOR));
                                        /* Construct path where we load CD2BOOT boot loader
                                           from to patch it */
    strcpy((char *)aucCD2BootPath, (char *)puAnchor->puStartupParameters->acPath);
    strcat((char *)aucCD2BootPath, "CD2BOOT");
                                        /* Find all drives and check them for being
                                           removeable */
    DosQueryCurrentDisk(&ulDriveNumber, &ulLogicalDriveMap);
    ulRemoveableMap=ulLogicalDriveMap;
    for(aucDrive[0]='A', ulRemoveableBit=0x00000001; 
        aucDrive[0]<='Z'; 
        aucDrive[0]++, ulLogicalDriveMap>>=1, ulRemoveableBit<<=1)
        {                               /* Loop for drive A: to Z: (blocks of 0s must be
                                           expected because of network drives) */
                                        /* Now query if the media is removable (it is removeable
                                           if 0 is return in the Data Packet). The media needs not 
                                           to be inserted. For unknown reasons, that ioctl()
                                           fails on drive A: and B:, probably because they are
                                           no disks */
        memset(&getdeviceparamsPPF, 0, sizeof(getdeviceparamsPPF));
        memset(&getdeviceparamsDPF, 0, sizeof(getdeviceparamsDPF));
        getdeviceparamsPPF.ucDriveUnit=aucDrive[0]-'A';
        ulParamLengthInOut=sizeof(getdeviceparamsPPF);
        ulDataLengthInOut=sizeof(getdeviceparamsDPF);
        apiretRc=DosDevIOCtl(-1, IOCTL_DISK, DSK_GETDEVICEPARAMS,
            &getdeviceparamsPPF, ulParamLengthInOut, &ulParamLengthInOut,
            &getdeviceparamsDPF, ulDataLengthInOut, &ulDataLengthInOut);
        if(apiretRc==NO_ERROR)
            {
                                        /* For a removeable drive, it must have a FAT */
            if(getdeviceparamsDPF.biosparameterblock.cFATs==0)
                apiretRc=ERROR_NOT_SUPPORTED;
                                        /* For non removeable types ignore them */
            switch(getdeviceparamsDPF.biosparameterblock.bDeviceType)
            {
            case DEVTYPE_FIXED:
            case DEVTYPE_TAPE:
                apiretRc=ERROR_NOT_SUPPORTED;
                break;
            }
                                        /* For non removeable media ignore them (does
                                           not work currently, maybe the packing of the
                                           Data Packet is incorrect) */
            if(getdeviceparamsDPF.usDeviceAttributes & ATTRIBUTE_NONREMOVEABLE)
                apiretRc=ERROR_NOT_SUPPORTED;
            }
        if(apiretRc!=NO_ERROR)
            ulRemoveableMap&=(~ulRemoveableBit);
        }
                                        /* Drive A is always assumed to be removeable */
    ulRemoveableMap|=0x00000001;
}

/*--------------------------------------------------------------------------------------*\
 * CD2BOOT: Destructor                                                                  *
\*--------------------------------------------------------------------------------------*/
CD2Boot::~CD2Boot(void)
{
    DEBUG_L3(__THREAD_1__, "");

                                        /* Clear allocations */
    if(pbCD2BOOT)
        delete(pbCD2BOOT);
}

/*--------------------------------------------------------------------------------------*\
 * CD2Boot: InstallCD2Boot()                                                            *
 *          Install the modified removeable media boot process                          *
\*--------------------------------------------------------------------------------------*/
ULONG                   CD2Boot::InstallCD2Boot()
{
    DEBUG_L1(__THREAD_1__, "");

    ULONG       ulError=NO_ERROR;
    ULONG       ulSearchOffset;
    UCHAR      *pucError;
    UCHAR       aucDriveCD2Boot[]="A:";
    UCHAR       aucSourcePathCD2Boot[CCHMAXPATH];
    UCHAR       aucDestinationPathCD2Boot[CCHMAXPATH];
    UCHAR       aucSearchString[SECTORSIZE+1];
    ULONG       ulIndex;

                                        /* Initialize */
    DosError(FERR_DISABLEHARDERR);
    aucDriveCD2Boot[0]=ucDrive;
    memset(aucSourcePathCD2Boot, '\0', sizeof(aucSourcePathCD2Boot));
    strcpy((char *)aucSourcePathCD2Boot, puAnchorCopy->puStartupParameters->acPath);
    strcat((char *)aucSourcePathCD2Boot, "CD2BOOT");
    memset(aucDestinationPathCD2Boot, '\0', sizeof(aucDestinationPathCD2Boot));
    aucDestinationPathCD2Boot[0]=ucDrive;
    strcat((char *)aucDestinationPathCD2Boot, ":\\CD2BOOT");
                                        /* Load the boot sector of a removeable media into 
                                           memory */
    puAnchorCopy->displayMessage(
        puAnchorCopy->loadMessage(NLSMSG_READBOOTSECTOR, (char *)aucDriveCD2Boot), TRUE);
    DosSleep(INITIALIZATION_DELAY);
    pucError=BootSectorIO(aucDriveCD2Boot, FALSE);
    if(pucError)
        {
        puAnchorCopy->displayMessage((char *)pucError, FALSE);
        DosSleep(INITIALIZATION_DELAY);
        return(ERROR_NOT_SUPPORTED);    
        }
                                        /* Load CD2BOOT image from the path CD Boot/2 is running
                                           from into memory. Then find the buffer to be
                                           replaced with the original OS/2 removeable media boot 
                                           sector code and copy the boot sector there */
    puAnchorCopy->displayMessage(
        puAnchorCopy->loadMessage(NLSMSG_READCD2BOOT, (char *)aucSourcePathCD2Boot), TRUE);
    DosSleep(INITIALIZATION_DELAY);
    ulError=CD2BootIO(aucSourcePathCD2Boot, FALSE);
    if(ulError!=NO_ERROR)
        {
        puAnchorCopy->displayMessage(
            puAnchorCopy->loadMessage(NLSMSG_ERR_CD2BOOTIO), FALSE);
        DosSleep(INITIALIZATION_DELAY);
        return(ERROR_NOT_SUPPORTED);    
        }
                                        /* Now find the sector sized (512 bytes) buffer
                                           within CD2BOOT containing the repetitive
                                           sequence "CD2BOOT " which is going to be
                                           replaced by the OS/2 boot sector code */
    memset(&aucSearchString, '\0', sizeof(aucSearchString));
    for(ulIndex=(SECTORSIZE/(sizeof("CD2BOOT ")-1));
        ulIndex>0;
        ulIndex--)
        strcat((char *)aucSearchString, "CD2BOOT ");
    ulSearchOffset=SearchBuffer((UCHAR *)pbCD2BOOT, ulCD2BOOTSize, aucSearchString);
    if(ulSearchOffset!=(ULONG)-1)
        {
                                        /* Replace repetitive sequence "CD2BOOT " with
                                           removeable media boot sector code we
                                           loaded earlier. Assume that the boot sector
                                           either contains "OS2BOOT    " (if it is an
                                           original OS/2 boot sector) or "CD2BOOT    "
                                           (if CD Boot/2 has already been run against
                                           that drive). If none of those 2 strings
                                           can be found, we only can assume that this
                                           is a "normal" FAT removeable bootsector
                                           e.g. from DOS */
        ulBootSectorOffset=ulSearchOffset;
        memset(&aucSearchString, '\0', sizeof(aucSearchString));
        strcpy((char *)aucSearchString, SIGNATURE_CD2BOOT);
        ulSearchOffset=SearchBuffer((UCHAR *)&bootsectorRemoveable, sizeof(bootsectorRemoveable), aucSearchString);
        if(ulSearchOffset!=(ULONG)-1)
            {
                                        /* We found an OS/2 removeable bootsector that
                                           already has been modified by CD Boot/2 */
            ulOS2BOOTOffset=ulSearchOffset;
            memcpy(&((BYTE *)&bootsectorRemoveable)[ulOS2BOOTOffset], 
                SIGNATURE_OS2BOOT, sizeof(SIGNATURE_OS2BOOT)-1);
            puAnchorCopy->displayMessage(
                puAnchorCopy->loadMessage(NLSMSG_CD2BOOTALREADYRUN, (char *)aucSourcePathCD2Boot), TRUE);
            DosSleep(INITIALIZATION_DELAY);
                                        /* Write the OS/2 removeable bootsector, modified
                                           to load OS2BOOT, into CD2BOOT */
            memcpy(&pbCD2BOOT[ulBootSectorOffset],
                &bootsectorRemoveable,
                SECTORSIZE);     
            }
        else
            {
                                        /* Check if we find a OS/2 removeable bootsector */
            memset(&aucSearchString, '\0', sizeof(aucSearchString));
            strcpy((char *)aucSearchString, SIGNATURE_OS2BOOT);
            ulSearchOffset=SearchBuffer((UCHAR *)&bootsectorRemoveable, sizeof(bootsectorRemoveable), aucSearchString);
            if(ulSearchOffset!=(ULONG)-1)
                {
                                        /* We found an OS/2 removeable bootsector,
                                           which we write into CD2BOOT */
                memcpy(&pbCD2BOOT[ulBootSectorOffset],
                    &bootsectorRemoveable,
                    SECTORSIZE);     
                }
            else
                {
                                        /* Hmm, does not seem to be a OS/2 removeable
                                           media bootsector, so we have to rely on
                                           the bootsector being a "normal" (e.g. DOS)
                                           FAT removeable bootsector.
                                           We will replace that bootsector by a modified
                                           OS/2 removeable bootsector that loads CD2BOOT
                                           and upon user selection the orignal removeable
                                           FAT bootsector or boots from the first harddisk.
                                           We do copy the original bootsector's BIOS
                                           PARAMETER BLOCK though, as we don't know
                                           which media (e.g. 1MB, 2MB, 4MB diskette)
                                           we handle */
                memset(&aucSearchString, '\0', sizeof(aucSearchString));
                strcpy((char *)aucSearchString, SIGNATURE_FAT);
                ulSearchOffset=SearchBuffer((UCHAR *)&bootsectorRemoveable.extbpbMedia.abFileSysType, 
                    sizeof(bootsectorRemoveable.extbpbMedia.abFileSysType), 
                    aucSearchString);
                if(ulSearchOffset!=(ULONG)-1)
                    {
                    puAnchorCopy->displayMessage(
                        puAnchorCopy->loadMessage(NLSMSG_FOUNDFATBOOTSECTOR), TRUE);
                    }
                else
                    {
                    puAnchorCopy->displayMessage(
                        puAnchorCopy->loadMessage(NLSMSG_FOUNDUNKNOWNBOOTSECTOR), TRUE);
                    }
                DosSleep(INITIALIZATION_DELAY);
                                        /* Regardless what bootsector we found, we
                                           try to use it anyway */
                memcpy(&bootsectorRemoveableOS2.extbpbMedia,
                    &bootsectorRemoveable.extbpbMedia,
                    sizeof(EXT_BPB));
                                        /* We write the original bootsector into CD2BOOT */
                memcpy(&pbCD2BOOT[ulBootSectorOffset],
                    &bootsectorRemoveable,
                    SECTORSIZE);     
                                        /* Finally, replace the bootsector loaded from
                                           the non-OS/2 formatted removeable media with
                                           the BPB-adapted OS/2 one */
                memcpy(&bootsectorRemoveable, 
                    &bootsectorRemoveableOS2, 
                    sizeof(bootsectorRemoveable));
                }
            }
        }
    else
        {
                                        /* We couldn't find the signature in our
                                           file CD2BOOT we shipped, something must
                                           have been corrupted */
        puAnchorCopy->displayMessage(
            puAnchorCopy->loadMessage(NLSMSG_ERR_CD2BOOTSIGNATURE), FALSE);
        DosSleep(INITIALIZATION_DELAY);
        return(ERROR_NOT_SUPPORTED);    
        }
                                        /* Find the CD2BOOT control structure within CD2BOOT
                                           image in memory */
    memset(&aucSearchString, '\0', sizeof(aucSearchString));
    strcpy((char *)aucSearchString, SIGNATURE);
    ulSearchOffset=SearchBuffer((UCHAR *)pbCD2BOOT, ulCD2BOOTSize, aucSearchString);
    if(ulSearchOffset!=(ULONG)-1)
        {
        ulBootControlOffset=ulSearchOffset;
        pBootControl=(BOOTCTL *)&pbCD2BOOT[ulBootControlOffset];
        pBootControl->usSeconds=usSeconds;
        pErrorControl=(ERRORCTL *)&pbCD2BOOT[ERRORCTL_OFFSET];
        LoadBootControlMessages();
        }
    else
        {
        puAnchorCopy->displayMessage(
            puAnchorCopy->loadMessage(NLSMSG_ERR_CD2BOOTSIGNATURE), FALSE);
        DosSleep(INITIALIZATION_DELAY);
        return(ERROR_NOT_SUPPORTED);    
        }
                                        /* Write back CD2BOOT to the destination drive
                                           containing the removeable media */
    puAnchorCopy->displayMessage(
        puAnchorCopy->loadMessage(NLSMSG_WRITECD2BOOT, (char *)aucDestinationPathCD2Boot), TRUE);
    DosSleep(INITIALIZATION_DELAY);
    LoadErrorControlChecksum();
    ulError=CD2BootIO(aucDestinationPathCD2Boot, TRUE);
                                        /* Find the string "OS2BOOT   " within boot sector 
                                           and replace it with "CD2BOOT    " (the trailing
                                           spaces are required for the FAT 8.3 filename
                                           convention). If we can't find "OS2BOOT    " but
                                           "CD2BOOT    " instead, then we already have a
                                           boot sector run through CD Boot/2 previously,
                                           so we have to take care to do the things right */
    puAnchorCopy->displayMessage(
        puAnchorCopy->loadMessage(NLSMSG_ADJUSTBOOTSECTOR), TRUE);
    DosSleep(INITIALIZATION_DELAY);
    memset(&aucSearchString, '\0', sizeof(aucSearchString));
    strcpy((char *)aucSearchString, SIGNATURE_OS2BOOT);
    ulSearchOffset=SearchBuffer((UCHAR *)&bootsectorRemoveable, sizeof(bootsectorRemoveable), aucSearchString);
    if(ulSearchOffset!=(ULONG)-1)
        {
        ulOS2BOOTOffset=ulSearchOffset;
        memcpy(&((BYTE *)&bootsectorRemoveable)[ulOS2BOOTOffset], 
            SIGNATURE_CD2BOOT, sizeof(SIGNATURE_CD2BOOT)-1);
        }
    else
        {
        puAnchorCopy->displayMessage(
            puAnchorCopy->loadMessage(NLSMSG_ERR_OS2BOOTMISSING), FALSE);
        DosSleep(INITIALIZATION_DELAY);
        return(ERROR_NOT_SUPPORTED);    
        }
                                        /* Write boot sector back onto disk */
    puAnchorCopy->displayMessage(
        puAnchorCopy->loadMessage(NLSMSG_WRITEBOOTSECTOR, (char *)aucDriveCD2Boot), TRUE);
    DosSleep(INITIALIZATION_DELAY);
    pucError=BootSectorIO(aucDriveCD2Boot, TRUE);
    if(pucError)
        {
        puAnchorCopy->displayMessage((char *)pucError, FALSE);
        DosSleep(INITIALIZATION_DELAY);
        return(ERROR_NOT_SUPPORTED);    
        }
    return(ulError);
}

/*--------------------------------------------------------------------------------------*\
 * CD2Boot: CB2_DialogProc()                                                            *
 *          CDBOOTPM dialog procedure called from function stub (as we can't pass       *
 *          member functions as window procedures to the PM API (as the this pointer    *
 *          would be missing then), we have to use the trick of calling the member      *
 *          function via pointer passed to the function window procedure as a           *
 *          parameter.                                                                  *
\*--------------------------------------------------------------------------------------*/
MRESULT                 CD2Boot::CB2_DialogProc(HWND hwndDlg, ULONG msg, MPARAM mp1, MPARAM mp2)
{
#ifdef  __PM__
    static ULONG        ulInitialized=FALSE; 
    static ULONG        ulRecursion=FALSE; 
                                        /* Current drive */
    static UCHAR        aucDrive[]="A"; 
    static UCHAR        aucBuffer[4];
                                        /* Password processing rule ensurance */
    static UCHAR        aucPasswordCurrent[PWDSIZE];
    static UCHAR        aucPasswordPrevious[PWDSIZE];

    switch(msg)
    {
    case WM_INITDLG:
        {
#define DIALOG_CONTROLS                 18
#define DIALOG_ELEMENTS                 2
        char       *pcText;
        int         iControls[DIALOG_CONTROLS][DIALOG_ELEMENTS]=
                        { {CDGRP_CDBOOT2                ,NLSMSG_GRP_CDBOOT2},
                          {CDGRP_SETTINGS               ,NLSMSG_GRP_SETTINGS},
                          {CDST_DRIVE                   ,NLSMSG_ST_DRIVE},   
                          {CDST_CD2BOOT                 ,NLSMSG_ST_CD2BOOT},
                          {CDST_SECONDS                 ,NLSMSG_ST_SECONDS}, 
                          {CDRB_DEFAULTHARDDISK         ,NLSMSG_RB_DEFAULTHARDDISK}, 
                          {CDRB_DEFAULTREMOVEABLE       ,NLSMSG_RB_DEFAULTREMOVEABLE}, 
                          {CDCB_CLEARSCREEN             ,NLSMSG_CB_CLEARSCREEN}, 
                          {CDGRP_PASSWORD               ,NLSMSG_GRP_PASSWORD},
                          {CDCB_PASSWORD                ,NLSMSG_CB_PASSWORD},
                          {CDGRP_ADVANCED               ,NLSMSG_GRP_ADVANCED},
                          {CDCB_BMBOOT                  ,NLSMSG_CB_BMBOOT},
                          {CDGRP_COPYRIGHT              ,NLSMSG_GRP_COPYRIGHT},
                          {CDST_COPYRIGHT_COLOR         ,NLSMSG_ST_COLOR},
                          {CDGRP_CUSTOMMESSAGE          ,NLSMSG_GRP_CUSTOMMESSAGE},
                          {CDST_CUSTOMMESSAGE_COLOR     ,NLSMSG_ST_COLOR},
                          {DID_OK                       ,NLSMSG_ST_DID_OK},  
                          {DID_CANCEL                   ,NLSMSG_ST_DID_CANCEL} };
        int         iIndex;
        ULONG       ulDriveIndex;

        WinSendMsg(hwndDlg, WM_SETICON,
            (MPARAM)WinLoadPointer(HWND_DESKTOP, NULLHANDLE, ID_MAINWINDOW), NULL);
                                        /* Replace dialog control text with NLS texts */
        for(iIndex=0; iIndex<DIALOG_CONTROLS; iIndex++)
            {
            pcText=puAnchorCopy->loadMessageStrip(iControls[iIndex][1]);
            WinSetDlgItemText(hwndDlg, iControls[iIndex][0], pcText);
            puAnchor->allocFree(pcText);
            }
                                        /* Fill in drive control */
        WinSendDlgItemMsg(hwndDlg, CDCBX_DRIVE, EM_SETTEXTLIMIT,
            MPFROMSHORT(1), (MPARAM)NULL);
        for(aucDrive[0]='A', ulDriveIndex=0x00000001;
            aucDrive[0]<='Z';
            aucDrive[0]++, ulDriveIndex<<=1)
            {
            if(ulRemoveableMap & ulDriveIndex)
                {
                WinSendDlgItemMsg(hwndDlg, CDCBX_DRIVE,
                    LM_INSERTITEM, MPFROMSHORT(LIT_SORTASCENDING), MPFROMP(aucDrive));
                }
            }
        WinSendDlgItemMsg(hwndDlg, CDCBX_DRIVE,
            LM_SELECTITEM, MPFROMSHORT(0), MPFROMSHORT(TRUE));
                                        /* Fill in CD2BOOT path info */
        WinSendDlgItemMsg(hwndDlg, CDEF_CD2BOOT, EM_SETTEXTLIMIT,
            MPFROMSHORT(CCHMAXPATH), (MPARAM)NULL);
        WinSetDlgItemText(hwndDlg, CDEF_CD2BOOT, (PCSZ)aucCD2BootPath);
                                        /* Fill in Timout Period info */
        WinSendDlgItemMsg(hwndDlg, CDHSB_SECONDS,
            SBM_SETTHUMBSIZE, MPFROM2SHORT(1, 60), (MPARAM)NULL);
        WinSendDlgItemMsg(hwndDlg, CDHSB_SECONDS,
            SBM_SETSCROLLBAR, MPFROMSHORT(DELAY_DEFAULT),
            MPFROM2SHORT(0, 60));
        WinSetDlgItemText(hwndDlg, CDEF_SECONDS, 
            (CHAR *)_itoa(usSeconds, (char *)aucBuffer, 10));
                                        /* Fill in media to continue boot from */
        if(ulStatusFlag & STATUS_CID_HARDDISK)
            WinSendDlgItemMsg(hwndDlg, CDRB_DEFAULTHARDDISK, BM_SETCHECK,
                MPFROMSHORT(TRUE), (MPARAM)NULL);
        else
            WinSendDlgItemMsg(hwndDlg, CDRB_DEFAULTREMOVEABLE, BM_SETCHECK,
                MPFROMSHORT(TRUE), (MPARAM)NULL);
                                        /* Fill in Clear Screen info */
        if(ulStatusFlag & STATUS_CID_CLEARSCREEN)
            WinSendDlgItemMsg(hwndDlg, CDCB_CLEARSCREEN, BM_SETCHECK,
                MPFROMSHORT(TRUE), (MPARAM)NULL);
                                        /* Fill in Password info */
        memset(aucPasswordCurrent, '\0', sizeof(aucPasswordCurrent));
        memset(aucPasswordPrevious, '\0', sizeof(aucPasswordPrevious));
        WinSendDlgItemMsg(hwndDlg, CDEF_PASSWORD, EM_SETTEXTLIMIT,
            MPFROMSHORT(PWDSIZE-1), (MPARAM)NULL);
        if(ulStatusFlag & STATUS_CID_PASSWORD)
            {
            WinSendDlgItemMsg(hwndDlg, CDCB_PASSWORD, BM_SETCHECK,
                MPFROMSHORT(TRUE), (MPARAM)NULL);
            WinEnableWindow(WinWindowFromID(hwndDlg, CDEF_PASSWORD), TRUE);
            WinSetDlgItemText(hwndDlg, CDEF_PASSWORD, (PCSZ)aucPassword);
            }
                                        /* Fill in Clear Screen info */
        if(ulStatusFlag & STATUS_CID_BMBOOT)
            WinSendDlgItemMsg(hwndDlg, CDCB_BMBOOT, BM_SETCHECK,
                MPFROMSHORT(TRUE), (MPARAM)NULL);
                                        /* Fill in copyright and custom messages color
                                           comboboxes (both use the same texts, just
                                           different ones will be selected) */
        for(iIndex=NLSMSG_COLOR0; iIndex<=NLSMSG_COLOR15; iIndex++)
            {
            pcText=puAnchorCopy->loadMessageStrip(iIndex);
            WinSendDlgItemMsg(hwndDlg, CDCBX_COPYRIGHT_COLOR,
                LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(pcText));
            WinSendDlgItemMsg(hwndDlg, CDCBX_CUSTOMMESSAGE_COLOR,
                LM_INSERTITEM, MPFROMSHORT(LIT_END), MPFROMP(pcText));
            puAnchor->allocFree(pcText);
            }
                                        /* Fill in copyright messages. If the undocumented
                                           IBM mode is specified, remove the read-only
                                           mode from the copyright messages */
        WinSendDlgItemMsg(hwndDlg, CDCBX_COPYRIGHT_COLOR,
            LM_SELECTITEM, MPFROMSHORT(usCopyrightColor), MPFROMSHORT(TRUE));
        WinSendDlgItemMsg(hwndDlg, CDEF_COPYRIGHT1, EM_SETTEXTLIMIT,
            MPFROMSHORT(LINESIZE-1), (MPARAM)NULL);
        WinSendDlgItemMsg(hwndDlg, CDEF_COPYRIGHT2, EM_SETTEXTLIMIT,
            MPFROMSHORT(LINESIZE-1), (MPARAM)NULL);
        WinSendDlgItemMsg(hwndDlg, CDEF_COPYRIGHT3, EM_SETTEXTLIMIT,
            MPFROMSHORT(LINESIZE-1), (MPARAM)NULL);
        WinSetDlgItemText(hwndDlg, CDEF_COPYRIGHT1, (PCSZ)aucCopyright1);
        WinSetDlgItemText(hwndDlg, CDEF_COPYRIGHT2, (PCSZ)aucCopyright2);
        WinSetDlgItemText(hwndDlg, CDEF_COPYRIGHT3, (PCSZ)aucCopyright3);
        if(ulStatusFlag & STATUS_CID_MODEIBM)
            {
            WinEnableWindow(WinWindowFromID(hwndDlg, CDCBX_COPYRIGHT_COLOR), TRUE);
            WinSendDlgItemMsg(hwndDlg, CDEF_COPYRIGHT1, EM_SETREADONLY,
                MPFROMSHORT(FALSE), NULL);
            WinSendDlgItemMsg(hwndDlg, CDEF_COPYRIGHT2, EM_SETREADONLY,
                MPFROMSHORT(FALSE), NULL);
            WinSendDlgItemMsg(hwndDlg, CDEF_COPYRIGHT3, EM_SETREADONLY,
                MPFROMSHORT(FALSE), NULL);
            }
                                        /* Fill in custom messages */
        WinSendDlgItemMsg(hwndDlg, CDCBX_CUSTOMMESSAGE_COLOR,
            LM_SELECTITEM, MPFROMSHORT(usCustomMessageColor), MPFROMSHORT(TRUE));
        WinSendDlgItemMsg(hwndDlg, CDEF_CUSTOMMESSAGE1, EM_SETTEXTLIMIT,
            MPFROMSHORT(LINESIZE-1), (MPARAM)NULL);
        WinSendDlgItemMsg(hwndDlg, CDEF_CUSTOMMESSAGE2, EM_SETTEXTLIMIT,
            MPFROMSHORT(LINESIZE-1), (MPARAM)NULL);
        WinSendDlgItemMsg(hwndDlg, CDEF_CUSTOMMESSAGE3, EM_SETTEXTLIMIT,
            MPFROMSHORT(LINESIZE-1), (MPARAM)NULL);
        if(ulStatusFlag & STATUS_CID_CUSTOMMESSAGE)
            {
            WinSetDlgItemText(hwndDlg, CDEF_CUSTOMMESSAGE1, (PCSZ)aucCustomMessage1);
            WinSetDlgItemText(hwndDlg, CDEF_CUSTOMMESSAGE2, (PCSZ)aucCustomMessage2);
            WinSetDlgItemText(hwndDlg, CDEF_CUSTOMMESSAGE3, (PCSZ)aucCustomMessage3);
            }
                                        /* Finally, show it to the user */
        WinShowWindow(hwndDlg, TRUE);
                                        /* Let focus be set by PM */
        ulInitialized=TRUE;
        return((MRESULT)FALSE);
        }
        break;

    case WM_HSCROLL:
        {
        SHORT       sSliderPosition=SHORT1FROMMP(mp2);

        if(SHORT1FROMMP(mp1)==CDHSB_SECONDS)
            {
            switch(SHORT2FROMMP(mp2))
            {
            case SB_LINELEFT:
                usSeconds-=1;
                break;
    
            case SB_PAGELEFT:
                usSeconds-=5;
                break;
    
            case SB_LINERIGHT:
                usSeconds+=1;
                break;
    
            case SB_PAGERIGHT:
                usSeconds+=5;
                break;
    
            case SB_SLIDERTRACK:
            case SB_SLIDERPOSITION:
                usSeconds=sSliderPosition;
                break;
            }
            if(((SHORT)usSeconds)<=DELAY_MINIMUM) 
                usSeconds=DELAY_MINIMUM;
            if(usSeconds>DELAY_MAXIMUM) 
                usSeconds=DELAY_MAXIMUM;
            WinSetDlgItemText(hwndDlg, CDEF_SECONDS,
                (CHAR *)_itoa(usSeconds, (char *)aucBuffer, 10));
            WinSendDlgItemMsg(hwndDlg, CDHSB_SECONDS,
                SBM_SETPOS, MPFROMSHORT(usSeconds), (MPARAM)NULL);
            }
        }
        break;

    case WM_CONTROL:
                                        /* Ensure that we allow to enter only
                                           drive letters, which are alpha characters
                                           from A to Z. Note that we have made a
                                           suggestion, but do not doubt when the user
                                           wants to enter something that we haven't
                                           suggested (though installing CD2BOOT later
                                           on may fail then) */
        if((SHORT1FROMMP(mp1)==CDCBX_DRIVE) && (ulInitialized==TRUE) && (ulRecursion==FALSE))
            {
            ulRecursion=TRUE;
            if(SHORT2FROMMP(mp1)==CBN_EFCHANGE)
                {
                WinQueryDlgItemText(hwndDlg, CDCBX_DRIVE, sizeof(aucDrive), (PCSZ)aucDrive);
                if(!isalpha(aucDrive[0]))
                    {
                    aucDrive[0]='A';
                    WinSetDlgItemText(hwndDlg, CDCBX_DRIVE, (PCSZ)aucDrive);
                    }
                }
            ulRecursion=FALSE;
            }
                                        /* Check if button class window clicked or 
                                           double-clicked */
        if((SHORT2FROMMP(mp1)==BN_CLICKED) || (SHORT2FROMMP(mp1)==BN_DBLCLICKED))
            {
            ULONG   ulButtonChecked;
                                        /* Enable/Disable password entryfield */
                {
                ulButtonChecked=(ULONG)WinSendDlgItemMsg(hwndDlg, CDCB_PASSWORD, 
                    BM_QUERYCHECK, (MPARAM)NULL, (MPARAM)NULL);
                WinEnableWindow(WinWindowFromID(hwndDlg, CDEF_PASSWORD),
                    (ulButtonChecked==TRUE ? TRUE : FALSE));
                }
            }
                                        /* Check for entryfield change messages */
        if((SHORT2FROMMP(mp1)==EN_CHANGE) && (ulInitialized==TRUE) && (ulRecursion==FALSE))
            {
            ULONG   ulIndex;
            UCHAR   ucPassword;

            ulRecursion=TRUE;
            
            if(SHORT1FROMMP(mp1)==CDEF_PASSWORD)
                {
                WinQueryDlgItemText(hwndDlg, CDEF_PASSWORD, sizeof(aucPasswordCurrent), (PCSZ)aucPasswordCurrent);
                for(ulIndex=0; (ucPassword=aucPasswordCurrent[ulIndex])!='\0'; ulIndex++)
                    {
                    if(((ucPassword<'0') || (ucPassword>'9')) &&
                        ((ucPassword<'A') || (ucPassword>'Z')) &&
                        ((ucPassword<'a') || (ucPassword>'z')))
                        {
                        strcpy((char *)aucPasswordCurrent, (char *)aucPasswordPrevious);
                        ulIndex=(ULONG)-1;
                        break;
                        }
                    }
                if(ulIndex!=(ULONG)-1)
                    strcpy((char *)aucPasswordPrevious, (char *)aucPasswordCurrent);
                WinSetDlgItemText(hwndDlg, CDEF_PASSWORD, (PCSZ)aucPasswordCurrent);
                if(ulIndex==(ULONG)-1)
                    WinAlarm(HWND_DESKTOP, WA_WARNING);
                }
            ulRecursion=FALSE;
            }
        break;

    case WM_COMMAND:
        switch(SHORT1FROMMP(mp1))
        {
        case DID_OK:                    /* Enter key pressed */
            {                           /* Dialog terminated with DID_OK */
            int     iIndex;

            WinQueryDlgItemText(hwndDlg, CDCBX_DRIVE, sizeof(aucDrive), (PCSZ)aucDrive);
            ucDrive=aucDrive[0];
            if(WinSendDlgItemMsg(hwndDlg, CDRB_DEFAULTHARDDISK,
                BM_QUERYCHECK, (MPARAM)NULL, (MPARAM)NULL)==(MRESULT)TRUE)
                {
                ulStatusFlag&=(~STATUS_CID_REMOVEABLE);
                ulStatusFlag|=(STATUS_CID_HARDDISK);
                }
            else
                {
                ulStatusFlag&=(~STATUS_CID_HARDDISK);
                ulStatusFlag|=(STATUS_CID_REMOVEABLE);
                }
            if(WinSendDlgItemMsg(hwndDlg, CDCB_CLEARSCREEN,
                BM_QUERYCHECK, (MPARAM)NULL, (MPARAM)NULL)==(MRESULT)TRUE)
                {
                ulStatusFlag|=(STATUS_CID_CLEARSCREEN);
                }
            else
                {
                ulStatusFlag&=(~STATUS_CID_CLEARSCREEN);
                }
            if(WinSendDlgItemMsg(hwndDlg, CDCB_BMBOOT,
                BM_QUERYCHECK, (MPARAM)NULL, (MPARAM)NULL)==(MRESULT)TRUE)
                {
                ulStatusFlag|=(STATUS_CID_BMBOOT);
                }
            else
                {
                ulStatusFlag&=(~STATUS_CID_BMBOOT);
                }
            WinQueryDlgItemText(hwndDlg, CDEF_PASSWORD, sizeof(aucPassword), (PCSZ)aucPassword);
            if(WinSendDlgItemMsg(hwndDlg, CDCB_PASSWORD, 
                BM_QUERYCHECK, (MPARAM)NULL, (MPARAM)NULL)==(MRESULT)TRUE)
                {
                ulStatusFlag|=(STATUS_CID_PASSWORD);
                }
            else
                {
                ulStatusFlag&=(~STATUS_CID_PASSWORD);
                memset(aucPassword, '\0', sizeof(aucPassword));
                }
            iIndex=(int)WinSendDlgItemMsg(hwndDlg, CDCBX_COPYRIGHT_COLOR, LM_QUERYSELECTION,
                MPFROMSHORT(LIT_FIRST), (MPARAM)NULL);
            if(iIndex!=LIT_NONE)
                usCopyrightColor=iIndex;
            WinQueryDlgItemText(hwndDlg, CDEF_COPYRIGHT1, LINESIZE, (PCSZ)aucCopyright1);
            WinQueryDlgItemText(hwndDlg, CDEF_COPYRIGHT2, LINESIZE, (PCSZ)aucCopyright2);
            WinQueryDlgItemText(hwndDlg, CDEF_COPYRIGHT3, LINESIZE, (PCSZ)aucCopyright3);
            iIndex=(int)WinSendDlgItemMsg(hwndDlg, CDCBX_CUSTOMMESSAGE_COLOR, LM_QUERYSELECTION,
                MPFROMSHORT(LIT_FIRST), (MPARAM)NULL);
            if(iIndex!=LIT_NONE);
                usCustomMessageColor=iIndex;
            WinQueryDlgItemText(hwndDlg, CDEF_CUSTOMMESSAGE1, LINESIZE, (PCSZ)aucCustomMessage1);
            WinQueryDlgItemText(hwndDlg, CDEF_CUSTOMMESSAGE2, LINESIZE, (PCSZ)aucCustomMessage2);
            WinQueryDlgItemText(hwndDlg, CDEF_CUSTOMMESSAGE3, LINESIZE, (PCSZ)aucCustomMessage3);
            if((strlen((char *)aucCustomMessage1)) || 
                (strlen((char *)aucCustomMessage2)) || 
                (strlen((char *)aucCustomMessage3)))
                ulStatusFlag|=(STATUS_CID_CUSTOMMESSAGE);
            WinDismissDlg(hwndDlg, DID_OK);
            }
            break;
    
        case DID_CANCEL:                /* Cancel key pressed */
                                        /* Dialog terminated with DID_CANCEL */
            WinDismissDlg(hwndDlg, DID_CANCEL);
            break;
    
        default:
            return(WinDefDlgProc(hwndDlg, msg, mp1, mp2));
        }
        break;
    
    default:                            /* Default window procedure must be called */
        return(WinDefDlgProc(hwndDlg, msg, mp1, mp2));
    }
#endif  /* __PM__ */
    return((MRESULT)FALSE);                 /* We have handled the message */
}

/*--------------------------------------------------------------------------------------*\
 * CD2Boot: CB2_Interactive()                                                           *
 *          CDBOOT interactively presents the user the recommended choices and ask him  *
 *          to input his perferences.                                                   *
\*--------------------------------------------------------------------------------------*/
MRESULT                 CD2Boot::CB2_Interactive(void)
{
#ifdef  __AVIO__
                                        /* Buffer for all possible removeable drives
                                           in the sequence "A, B, C, ... Z" */
    char        acRemoveableDrives[26*3+1];
    char        acDrive[]="C";
    ULONG       ulIndex;
    ULONG       ulCount;
    int         iChar;
    int         iSeconds[2];

                                        /* If we already have the parameters we need
                                           (e.g. due to a CID installation) then
                                           just return */
    if((ulStatusFlag & (STATUS_CID_DRIVE|STATUS_CID_TIMEOUT))==
        (STATUS_CID_DRIVE|STATUS_CID_TIMEOUT))
        {
                                        /* We do not have to aske the user */
        puAnchorCopy->displayMessage(puAnchorCopy->loadMessage(NLSMSG_CID_UNATTENDED), TRUE);
        return((MRESULT)TRUE);          
        }
                                        /* Build up the list of all removeable drives
                                           we could find out */
    memset(&acRemoveableDrives, '\0', sizeof(acRemoveableDrives));
    for(ulIndex=0, ulCount=0; ulIndex<26; ulIndex++)
        {
        if(ulRemoveableMap & (0x00000001<<ulIndex))
            {
            if(acRemoveableDrives[0]!='\0')
                strcat(acRemoveableDrives, ", ");
            acDrive[0]='A'+ulIndex;
            strcat(acRemoveableDrives, acDrive);
            }
        }
                                        /* Display available drives (as we suggest it)
                                           and query what the user wants. Be sure that
                                           a drive letter (A to Z) is entered */
    puAnchorCopy->displayMessage(puAnchorCopy->loadMessage(NLSMSG_INFORMDRIVE, 
        (char *)acRemoveableDrives), TRUE);
    puAnchorCopy->displayMessage(puAnchorCopy->loadMessageStrip(NLSMSG_QUERYDRIVE), TRUE);
    while(TRUE)  
        {
        iChar=_getch();
        if((iChar==0) || (iChar==0xE0))
            iChar=_getch();
        if(isalpha(iChar))
            break;
        }
    iChar=toupper(iChar);
    ucDrive=(UCHAR)iChar;
    _putch(iChar);
    puAnchorCopy->displayMessage(puAnchorCopy->allocString("\r\n"), TRUE);
                                        /* Display info about the timer limited selections
                                           that can be made and query the period. Be sure
                                           that it stays within the limits */
    puAnchorCopy->displayMessage(
        puAnchorCopy->loadMessage(NLSMSG_INFORMSECONDS, DELAY_MINIMUM, DELAY_MAXIMUM), TRUE);
    puAnchorCopy->displayMessage(
        puAnchorCopy->loadMessageStrip(NLSMSG_QUERYSECONDS), TRUE);
    while(TRUE)  
        {
        iSeconds[0]=_getch();
        if((iSeconds[0]==0) || (iSeconds[0]==0xE0))
            iSeconds[0]=_getch();
        if(!isdigit(iSeconds[0]))
            continue;
        _putch(iSeconds[0]);
        while(TRUE)
            {
            iSeconds[1]=_getch();
            if((iSeconds[1]==0) || (iSeconds[1]==0xE0))
                iSeconds[1]=_getch();
            if(iSeconds[1]=='\r')
                {
                iSeconds[1]=iSeconds[0];
                iSeconds[0]='0';
                goto CB2I_SkipChar;
                }
            if(!isdigit(iSeconds[1]))
                continue;
            _putch(iSeconds[1]);
CB2I_SkipChar:
            break;
            }
        usSeconds=(iSeconds[0]-'0')*10+(iSeconds[1]-'0');
        if((usSeconds>=DELAY_MINIMUM) && (usSeconds<=DELAY_MAXIMUM))
            break;
        puAnchorCopy->displayMessage(puAnchorCopy->allocString("\r"), TRUE);
        puAnchorCopy->displayMessage(
            puAnchorCopy->loadMessageStrip(NLSMSG_QUERYSECONDS, DELAY_MINIMUM, DELAY_MAXIMUM), TRUE);
        }
    puAnchorCopy->displayMessage(puAnchorCopy->allocString("\r\n"), TRUE);
#endif  /* __AVIO__ */
    return((MRESULT)FALSE);                 /* We have asked the user */
}

/*--------------------------------------------------------------------------------------*\
 * CD2Boot: SetStatus()                                                                 *
 *          Or the contents of parameter ulMask into the status flag.                   *
\*--------------------------------------------------------------------------------------*/
MRESULT                 CD2Boot::SetStatus(ULONG ulStatusFlagMask, UCHAR *pucParameter, ULONG ulParameter)
{
    int         iTemp;

    if(ulStatusFlagMask & STATUS_CID_MODECID)
        {
        strcpy((char *)this->aucCIDFile, (char *)pucParameter);
        }
    if(ulStatusFlagMask & STATUS_CID_DRIVE)
        {
        if(pucParameter)
            this->ucDrive=*pucParameter;
        else
            this->ucDrive=' ';
        }
    if(ulStatusFlagMask & STATUS_CID_TIMEOUT)
        this->usSeconds=(USHORT)ulParameter;
    if(ulStatusFlagMask & STATUS_CID_MODEIBM)
        {
        if(ulParameter==1)
            strcpy((char *)this->aucCopyright1, (char *)pucParameter);
        else if(ulParameter==2)
            strcpy((char *)this->aucCopyright2, (char *)pucParameter);
        else if(ulParameter==3)
            strcpy((char *)this->aucCopyright3, (char *)pucParameter);
        }
    if(ulStatusFlagMask & STATUS_CID_CUSTOMMESSAGE)
        {
        if(ulParameter==1)
            strcpy((char *)this->aucCustomMessage1, (char *)pucParameter);
        else if(ulParameter==2)
            strcpy((char *)this->aucCustomMessage2, (char *)pucParameter);
        else if(ulParameter==3)
            strcpy((char *)this->aucCustomMessage3, (char *)pucParameter);
        }
    if(ulStatusFlagMask & STATUS_CID_PASSWORD)
        {
        if(pucParameter)
            strcpy((char *)this->aucPassword, (char *)pucParameter);
        }
    if(ulStatusFlagMask & STATUS_CID_COLORCOPYRIGHT)
        {
        usCopyrightColor=atoi((char *)pucParameter);
        }
    if(ulStatusFlagMask & STATUS_CID_COLORCUSTOM)
        {
        usCustomMessageColor=atoi((char *)pucParameter);
        }
    ulStatusFlag|=ulStatusFlagMask;
    return((MRESULT)TRUE);                 
}

/*--------------------------------------------------------------------------------------*\
 * CD2Boot: GetStatus()                                                                 *
 *          Return the status or'ed with the mask ulMask passed.                        *
\*--------------------------------------------------------------------------------------*/
ULONG                   CD2Boot::GetStatus(ULONG ulStatusFlagMask)
{
    return(ulStatusFlag & ulStatusFlagMask);
}

/*--------------------------------------------------------------------------------------*\
 * CD2Boot: LoadBootControlMessages()                                                   *
 *          Load the NLS messages that CD2BOOT will be using during execution from      *
 *          message file and copy them into the BOOTCTL structure.                      *
 *          NO_ERROR will be returned for no error, otherwise ERROR_NOT_SUPPORTED       *
\*--------------------------------------------------------------------------------------*/
ULONG                   CD2Boot::LoadBootControlMessages(void)
{
    DEBUG_L1(__THREAD_1__, "");
    
    ULONG       ulError=NO_ERROR;
    char       *pcText;

    if((pBootControl==0) || (pErrorControl==0))
        ulError=ERROR_NOT_SUPPORTED;
    else
        {
        pBootControl->usStatusFlag=(USHORT)ulStatusFlag;
                                        /* If the undocumented IBM mode was specified,
                                           use the text specified from the user, otherwise
                                           take it from the message file */
        pBootControl->usColorCopyright=((usCopyrightColor & 0x000F) | 0x0010);
        memset((char *)pBootControl->aucCopyright1, '\0', sizeof(pBootControl->aucCopyright1));
        memset((char *)pBootControl->aucCopyright2, '\0', sizeof(pBootControl->aucCopyright2));
        memset((char *)pBootControl->aucCopyright3, '\0', sizeof(pBootControl->aucCopyright3));
        pBootControl->usColorCustomMessage=((usCustomMessageColor & 0x000F) | 0x0010);
        if(ulStatusFlag & STATUS_CID_MODEIBM)
            {
            strncpy((char *)pBootControl->aucCopyright1, (char *)aucCopyright1, LINESIZE);
            strncpy((char *)pBootControl->aucCopyright2, (char *)aucCopyright2, LINESIZE);
            strncpy((char *)pBootControl->aucCopyright3, (char *)aucCopyright3, LINESIZE);
            }
        else
            {
            pcText=puAnchorCopy->loadMessageStrip(NLSMSG_CD2BOOT_COPYRIGHT1);
            strncpy((char *)pBootControl->aucCopyright1, pcText, LINESIZE);
            puAnchor->allocFree(pcText);
            pcText=puAnchorCopy->loadMessageStrip(NLSMSG_CD2BOOT_COPYRIGHT2);
            strncpy((char *)pBootControl->aucCopyright2, pcText, LINESIZE);
            puAnchor->allocFree(pcText);
            pcText=puAnchorCopy->loadMessageStrip(NLSMSG_CD2BOOT_COPYRIGHT3);
            strncpy((char *)pBootControl->aucCopyright3, pcText, LINESIZE);
            puAnchor->allocFree(pcText);
            }
        if(strlen((char *)aucPassword))
            {
            EncryptPassword();
            strncpy((char *)pBootControl->aucPassword, (char *)aucPassword, PWDSIZE-1);
            }
        if(strlen((char *)aucCustomMessage1))
            strncpy((char *)pBootControl->aucCustomMessage1, (char *)aucCustomMessage1, LINESIZE);
        if(strlen((char *)aucCustomMessage2))
            strncpy((char *)pBootControl->aucCustomMessage2, (char *)aucCustomMessage2, LINESIZE);
        if(strlen((char *)aucCustomMessage3))
            strncpy((char *)pBootControl->aucCustomMessage3, (char *)aucCustomMessage3, LINESIZE);
        pcText=puAnchorCopy->loadMessageStrip(NLSMSG_CD2BOOT_SELECTMESSAGE1);
        strncpy((char *)pBootControl->aucSelectMessage1, pcText, LINESIZE);
        puAnchor->allocFree(pcText);
        pcText=puAnchorCopy->loadMessageStrip(NLSMSG_CD2BOOT_SELECTMESSAGE2);
        strncpy((char *)pBootControl->aucSelectMessage2, pcText, LINESIZE);
        puAnchor->allocFree(pcText);
        pcText=puAnchorCopy->loadMessageStrip(NLSMSG_CD2BOOT_SELECTMESSAGE3);
        strncpy((char *)pBootControl->aucSelectMessage3, pcText, LINESIZE);
        puAnchor->allocFree(pcText);
        if(ulStatusFlag & STATUS_CID_HARDDISK)
            pBootControl->aucSelectMessage3[2]=' ';  
        else 
            pBootControl->aucSelectMessage2[2]=' ';  
        pcText=puAnchorCopy->loadMessageStrip(NLSMSG_CD2BOOT_SELECTMESSAGE4);
        strncpy((char *)pBootControl->aucSelectMessage4, pcText, LINESIZE);
        puAnchor->allocFree(pcText);
        pcText=puAnchorCopy->loadMessageStrip(NLSMSG_CD2BOOT_BOOTREMOVEABLE);
        strncpy((char *)pBootControl->aucBootRemoveable, pcText, LINESIZE);
        puAnchor->allocFree(pcText);
        pcText=puAnchorCopy->loadMessageStrip(NLSMSG_CD2BOOT_BOOTHARDDISK);
        strncpy((char *)pBootControl->aucBootHarddisk, pcText, LINESIZE);
        puAnchor->allocFree(pcText);
        pcText=puAnchorCopy->loadMessageStrip(NLSMSG_CD2BOOT_SYS02025);
        strncpy((char *)pBootControl->aucSys02025, pcText, LINESIZE);
        puAnchor->allocFree(pcText);
        pcText=puAnchorCopy->loadMessageStrip(NLSMSG_CD2BOOT_SYS02027);
        strncpy((char *)pBootControl->aucSys02027, pcText, LINESIZE);
        puAnchor->allocFree(pcText);
        pcText=puAnchorCopy->loadMessageStrip(NLSMSG_CD2BOOT_PASSWORDENTER);
        strncpy((char *)pBootControl->aucPasswordEnter, pcText, LINESIZE);
        puAnchor->allocFree(pcText);
        pcText=puAnchorCopy->loadMessageStrip(NLSMSG_CD2BOOT_PASSWORDINVALID);
        strncpy((char *)pBootControl->aucPasswordInvalid, pcText, LINESIZE);
        puAnchor->allocFree(pcText);
        pcText=puAnchorCopy->loadMessageStrip(NLSMSG_CD2BOOT_ADVANCEDMESSAGE1);
        strncpy((char *)pBootControl->aucAdvancedMessage1, pcText, LINESIZE);
        puAnchor->allocFree(pcText);
        pcText=puAnchorCopy->loadMessageStrip(NLSMSG_CD2BOOT_ADVANCEDMESSAGE2);
        strncpy((char *)pBootControl->aucAdvancedMessage2, pcText, LINESIZE);
        puAnchor->allocFree(pcText);
                                        /* Fill ERRORCTL structure */
        pErrorControl->ulChecksum=0;
        pcText=puAnchorCopy->loadMessageStrip(NLSMSG_CD2BOOT_FRAGMENTMESSAGE);
        strncpy((char *)pErrorControl->aucFragmentMessage, pcText, LINESIZE);
        puAnchor->allocFree(pcText);
        }
    return(ulError);
}

/*--------------------------------------------------------------------------------------*\
 * CD2Boot: LoadErrorControlChecksum()                                                  *
 *          Calculate the checksum over CD2BOOT - which by now has been filled with all *
 *          the messages and control data CD2BOOT requires during execution - and write *
 *          it into pErrorControl.                                                      *
 *          NO_ERROR will be returned for no error, otherwise ERROR_NOT_SUPPORTED       *
\*--------------------------------------------------------------------------------------*/
ULONG                   CD2Boot::LoadErrorControlChecksum(void)
{
    DEBUG_L1(__THREAD_1__, "");
    
    ULONG       ulError=NO_ERROR;
    ULONG       ulChecksum;
    ULONG      *pulData;
    ULONG       ulIndex;

    if((pBootControl==0) || (pErrorControl==0))
        ulError=ERROR_NOT_SUPPORTED;
    else
        {
                                        /* Calculate the checksum of CD2BOOT while the
                                           checksum in pErrorControl is still 0 assuming
                                           that will give the expected total of 
                                           0x55AA55AA */
        pErrorControl->ulChecksum=0;
        ulChecksum=0;
        pulData=(ULONG *)pbCD2BOOT;
        for(ulIndex=0; ulIndex<ulCD2BOOTSize; ulIndex+=sizeof(ulChecksum), pulData++)
            {
            ulChecksum^=(*pulData);
            }
                                        /* Now calculate what the checksum in pErrorControl
                                           should have been instead of 0 to give the 
                                           expected checksum total of 0x55AA55AA */
        ulChecksum^=(CD2BOOT_CHECKSUM);
                                        /* Write the checksum correction back into CD2BOOT,
                                           so that CD2BOOT will calculate the same
                                           total checksum of 0x55AA55AA during its 
                                           execution as we did here */
        pErrorControl->ulChecksum=ulChecksum;
        }
    return(ulError);
}

/*--------------------------------------------------------------------------------------*\
 * CD2Boot: BootSectorIO()                                                              *
 *          Read or write the boot sector into/from memory of the drive specified,      *
 *          which should be a removeable media (as non-removeable media will have the   *
 *          partition table on that position).                                          *
 *          NULL will be returned for no error, otherwise the error loaded message      *
\*--------------------------------------------------------------------------------------*/
UCHAR                  *CD2Boot::BootSectorIO(UCHAR *pucDrive, BOOL bWrite)
{
    DEBUG_L1(__THREAD_1__, "");

    HFILE       hfileDrive;
    ULONG       ulAction;
    TRACKLAYOUT tracklayoutDrive;
    ULONG       ulParameterPacketSize;
    ULONG       ulDataPacketSize;
    APIRET      apiretRc;

    try {
                                        /* Open the removeable media */
        apiretRc=DosOpen((PCSZ)pucDrive, &hfileDrive, &ulAction, 0, FILE_NORMAL, 
            OPEN_ACTION_OPEN_IF_EXISTS, 
            OPEN_FLAGS_DASD |
            OPEN_FLAGS_WRITE_THROUGH | OPEN_FLAGS_NOINHERIT | 
            OPEN_SHARE_DENYNONE | OPEN_ACCESS_READWRITE, 0);
        if(apiretRc!=NO_ERROR)
            throw((ULONG)NLSMSG_ERR_DOSOPEN);
                                        /* Lock it */
        apiretRc=DosDevIOCtl(hfileDrive, IOCTL_DISK, DSK_LOCKDRIVE,
            0, 0, 0, 0, 0, 0);
        if(apiretRc!=NO_ERROR)
            throw((ULONG)NLSMSG_ERR_LOCKING);
                                        /* Read/Write the boot sector or partition table */
        memset(&tracklayoutDrive, 0, sizeof(tracklayoutDrive));
        tracklayoutDrive.bCommand=1;
        tracklayoutDrive.usHead=0;
        tracklayoutDrive.usCylinder=0;
        tracklayoutDrive.usFirstSector=0;
        tracklayoutDrive.cSectors=1;
        tracklayoutDrive.TrackTable[0].usSectorNumber=1;
        tracklayoutDrive.TrackTable[0].usSectorSize=512;
        ulParameterPacketSize=sizeof(tracklayoutDrive);
        ulDataPacketSize=sizeof(bootsectorRemoveable);
        apiretRc=DosDevIOCtl(hfileDrive, IOCTL_DISK, 
            (bWrite==FALSE ? DSK_READTRACK : DSK_WRITETRACK),
            &tracklayoutDrive, ulParameterPacketSize, &ulParameterPacketSize,
            &bootsectorRemoveable, ulDataPacketSize, &ulDataPacketSize);
        if(apiretRc!=NO_ERROR)
            {
            DosDevIOCtl(hfileDrive, IOCTL_DISK, DSK_UNLOCKDRIVE,
                0, 0, 0, 0, 0, 0);
            DosClose(hfileDrive);
            throw((ULONG)NLSMSG_ERR_SECTORIO);
            }
                                        /* Unlock it */
        apiretRc=DosDevIOCtl(hfileDrive, IOCTL_DISK, DSK_UNLOCKDRIVE,
            0, 0, 0, 0, 0, 0);
        if(apiretRc!=NO_ERROR)
            throw((ULONG)NLSMSG_ERR_LOCKING);
                                        /* Close the removeable media */
        apiretRc=DosClose(hfileDrive);
        if(apiretRc!=NO_ERROR)
            throw((ULONG)NLSMSG_ERR_DOSCLOSE);
        }
    catch(ULONG ulErrorMessage)
        {
        return((UCHAR *)puAnchorCopy->loadMessage(ulErrorMessage, apiretRc));
        }
    return(NULL);
}

/*--------------------------------------------------------------------------------------*\
 * CD2Boot: CD2BootIO()                                                                 *
 *          Read or write the image of the file "CD2BOOT" (which is built as part of    *
 *          this package) into/from memory. When reading, the buffer will be allocated  *
 *          first.                                                                      *
 *          NO_ERROR will be returned for no error, otherwise the OS/2 error code       *
\*--------------------------------------------------------------------------------------*/
ULONG                   CD2Boot::CD2BootIO(UCHAR *pucCD2Boot, BOOL bWrite)
{
    DEBUG_L1(__THREAD_1__, "");
    FILE       *pFile=0;
    ULONG       ulFileSize=0;

    try {
        pFile=fopen((const char *)pucCD2Boot, (bWrite==FALSE ? "rb" : "wb"));
        if(pFile==NULLHANDLE)
            throw((ULONG)ERROR_NOT_SUPPORTED);
        if(bWrite==FALSE)
            {
            ulFileSize=fseek(pFile, 0, SEEK_END);
            if(ulFileSize!=0)
                throw((ULONG)ERROR_NOT_SUPPORTED);
            ulFileSize=ftell(pFile);
            pbCD2BOOT=new BYTE[ulFileSize];
            ulCD2BOOTSize=ulFileSize;
            ulFileSize=fseek(pFile, 0, SEEK_SET);
            if(ulFileSize!=0)
                throw((ULONG)ERROR_NOT_SUPPORTED);
            ulFileSize=fread(pbCD2BOOT, sizeof(BYTE), ulCD2BOOTSize, pFile); 
            }
        else
            {
            ulFileSize=fwrite(pbCD2BOOT, sizeof(BYTE), ulCD2BOOTSize, pFile); 
            }
        fclose(pFile);
        }
    catch(ULONG ulError)
        {
        return(ulError);
        }
    return(NO_ERROR);
}

/*--------------------------------------------------------------------------------------*\
 * CD2Boot: SearchBuffer()                                                              *
 *          Search a string, that optionally contains asterisks (*) as a single         *
 *          character wildcard, in a specified buffer.                                  *
 *          (ULONG)-1 will be returned if the string could not be found, otherwise the  *
 *          first offset of the string within the buffer is returned.                   *
\*--------------------------------------------------------------------------------------*/
ULONG                   CD2Boot::SearchBuffer(UCHAR *pucBuffer, ULONG ulBufferSize, UCHAR *pucString)
{
    DEBUG_L1(__THREAD_1__, "");
    ULONG       ulOffset=(ULONG)-1;
    ULONG       ulStringSize;
    UCHAR      *pucBufferIndex;    
    UCHAR      *pucBufferEnd;    
    ULONG       iIndex;

                                        /* Try to find the string (with wildcards)
                                           within the buffer, ensure not to get
                                           outside the buffer limits */
    ulStringSize=strlen((const char *)pucString);
    pucBufferEnd=pucBuffer+ulBufferSize-ulStringSize;
    for(pucBufferIndex=pucBuffer; pucBufferIndex<=pucBufferEnd; pucBufferIndex++)    
        {
        if((*pucBufferIndex==*pucString) || (*pucString=='*'))
            {
            for(iIndex=0; iIndex<ulStringSize; iIndex++)
                {
                if(pucString[iIndex]=='*')
                    continue;
                if(pucBufferIndex[iIndex]!=pucString[iIndex])
                    break;
                }
            if(iIndex>=ulStringSize)
                return((ULONG)(pucBufferIndex-pucBuffer));
            }
        }
    return(ulOffset);
}

/*--------------------------------------------------------------------------------------*\
 * CD2Boot: EncryptPassword()                                                           *
 *          Encrypt the password given in aucPassword in place. In C/C++ we lack of one *
 *          of the low level bit manipulation instructions which we have in assembler,  *
 *          so it looks like unnecessary complex.                                       *
\*--------------------------------------------------------------------------------------*/
ULONG                   CD2Boot::EncryptPassword(void)
{
    DEBUG_L1(__THREAD_1__, "");
    UCHAR       aucPasswordCopy[PWDSIZE];
    ULONG       ulIndex;
    ULONG       ulTemp;

    strcpy((char *)aucPasswordCopy, (char *)aucPassword);
    for(ulIndex=0; aucPasswordCopy[ulIndex]!='\0'; ulIndex++)
        {
        ulTemp=(ULONG)aucPasswordCopy[ulIndex];
        if(ulIndex & 0x01)
            {
            ulTemp<<=(8-ulIndex);
            aucPassword[ulIndex]=(UCHAR)((ulTemp|((ulTemp&0x0FF00)>>8))^0xA5);
            }
        else
            {
            ulTemp<<=ulIndex;
            aucPassword[ulIndex]=(UCHAR)((ulTemp|(ulTemp>>8))^0x3C);
            }
        }    
    return(NO_ERROR);
}

#ifdef  __PM__
/****************************************************************************************\
 * CD Boot/2 dialog window procedure that transfers execution context back to the       *
 * CD2BOOT object.                                                                      *
\****************************************************************************************/
MRESULT  EXPENTRY CB2_DialogProc(HWND hwndDlg, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    static CD2Boot *pCD2Boot=0;

                                        /* Save the reference to the CD2BOOT object
                                           passed during dialog initialization so that
                                           for other messages we can pass those messages
                                           on to the object's window procedure method */
    if(msg==WM_INITDLG)
        pCD2Boot=(CD2Boot *)mp2;
    if(pCD2Boot!=0)
        return((MRESULT)pCD2Boot->CB2_DialogProc(hwndDlg, msg, mp1, mp2));
    return(WinDefDlgProc(hwndDlg, msg, mp1, mp2));
}
#endif  /* __PM__ */
