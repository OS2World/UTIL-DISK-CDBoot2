/***********************************************************************\
 *                              UStartup                               *
 *              Copyright (C) by Stangl Roman, 1999, 2002              *
 * This Code may be freely distributed, provided the Copyright isn't   *
 * removed, under the conditions indicated in the documentation.       *
 *                                                                     *
 * UStartup.cpp General purpose application entry code that displays a *
 *              Logo window, loads resources and starts the real part  *
 *              of the application from those resources.               *
 *                                                                     *
\***********************************************************************/

#pragma strings(readonly)

#define     INCL_WINWINDOWMGR
#define     INCL_WINSHELLDATA
#ifdef  __PM__
#define     INCL_WINFRAMEMGR
#define     INCL_WINDIALOGS
#define     INCL_WINBUTTONS
#define     INCL_WINSTATICS
#define     INCL_WINLISTBOXES
#define     INCL_GPIBITMAPS
#define     INCL_WINSYS
#endif  /* __PM__ */
#define     INCL_DOSFILEMGR
#define     INCL_DOSPROCESS
#define     INCL_DOSNLS
#define     INCL_DOSMODULEMGR
#define     INCL_DOSERRORS
#define     INCL_DOSMISC
#define     INCL_DOSMEMMGR
#define     INCL_ORDINALS
#include    <os2.h>

#include    <io.h>
#include    <fcntl.h>
#include    <memory.h>
#include    <string.h>
#include    <stdio.h>
#include    <stdlib.h>

#include    "CDBoot2.hpp"
#include    "UStartup.hpp"

                                        /* Debugging */
#define     _FILE_                      "UStartup.cpp "##BLDLEVEL_VERSION
static char _VERSION_[]=_FILE_;
#define     __THREAD_1__                "UStartup-Main"
#define     __THREAD_2__                "UStartup-Trd"

                                        /* Posted before entering message loop to do startup processing */
#define     WM_INITIALIZE               (WM_USER+1)
                                        /* Posted during WM_INITIALIZE processing to startup environment */
#define     WM_STARTUP                  (WM_USER+2)
                                        /* Output message into logo window */
#define     WM_STARTUPMESSAGE           (WM_USER+3)
                                        /* Display a message box modal to the logo window */
#define     WM_MESSAGEBOX               (WM_USER+4)
                                        /* Show or hide Logo window and/or Close pushbutton */
#define     WM_SHOWLOGOWINDOW           (WM_USER+5)


                                        /* Undocumented styles of WinDrawBorder() */
#define DB_RAISED                   0x0400
#define DB_DEPRESSED                0x0800
#define DB_TROUGH                   0x1000
#define DB_FENCE                    0x2000
#define DB_FIELD                    0x4000
#define DB_CORNERBORDER             0x8000

/****************************************************************************************\
 * Class: UAnchor                                                                       *
\****************************************************************************************/
/*--------------------------------------------------------------------------------------*\
 * UAnchor: Constructor(s)                                                              *
\*--------------------------------------------------------------------------------------*/
UAnchor::UAnchor(UStartupParameters *puStartupParameters) :
         pcCommandlineCopy(0),
         pcUserCommandline(0),
         puStartupParameters(puStartupParameters),
         puAnchor(puStartupParameters->puAnchor),
         ulDebug(puStartupParameters->ulDebug)
{
    DEBUG_L2(__THREAD_1__, "");
}

UAnchor::UAnchor(int argc, char **argv, int iStatus, char *pcApplicationName, char *pcPrefix) :
         pcCommandlineCopy(0),
         pcUserCommandline(0),
         pfnPrfOpenProfile(0),
         pfnPrfWriteProfileData(0),
         pfnPrfQueryProfileData(0),
         pfnPrfCloseProfile(0),
         puStartupParameters(0),
         ulDebug(DEBUG_LEVEL_0),
         ulReturnCode(CB2ERR_NO_ERROR),
         puAnchor(0)
{
    puAnchor=this;
    APIRET              apiretRc=NO_ERROR;
    UCHAR              *pucEnvironmentVariable=0;
    TIB                *ptib=0;         /* Thread information block */
    PIB                *ppib=0;         /* Process information block */
    char               *pcOption=0;     /* Commandline option */
    COUNTRYCODE         countrycode={ 0 };
    COUNTRYINFO         countryinfo={ 0 };
    ULONG               ulInfoLength=0;
    int                 iTemp=0;
    UCountryTranslate   ucountrytranslate[]= { 
                                               {  01L, "Us" },
                                               {  02L, "Fr" },
                                               {  07L, "Ru" },
                                               {  27L, "Us" },
                                               {  31L, "Nl" },
                                               {  32L, "Be" },
                                               {  33L, "Fr" },
                                               {  34L, "Es" },
                                               {  39L, "It" },
                                               {  41L, "Gr" },
                                               {  43L, "Gr" },
                                               {  44L, "Us" },
                                               {  45L, "Dk" },
                                               {  46L, "Se" },
                                               {  47L, "No" },
                                               {  49L, "Gr" },
                                               {  61L, "Us" },
                                               {  64L, "Us" },
                                               {  81L, "Jp" },
                                               {  99L, "Us" },
                                               { 353L, "Us" },
                                               { 358L, "Fi" },
                                               { 972L, "Il" },
                                             };
    HMODULE             hmoduleDOSCALL1=0;
    HMODULE             hmodulePMSHAPI=0;
    DOSSMPMPRESENT     *pfnDosSmPMPresent;
    UCHAR               ucBuffer[CCHMAXPATH];
    SHORT               sFlag;

                                        /* Initialize debugging environment */
#ifdef  DEBUG
    ulDebug=DEBUG_LEVEL_1;
#endif
#ifdef  DEBUG_1
    ulDebug=DEBUG_LEVEL_1;
#endif
#ifdef  DEBUG_2
    ulDebug=DEBUG_LEVEL_2;
#endif
#ifdef  DEBUG_3
    ulDebug=DEBUG_LEVEL_3;
#endif
    if(DosScanEnv("DEBUG", (PCSZ *)&pucEnvironmentVariable)==NO_ERROR)
        {
        if(pucEnvironmentVariable[0]=='0')
            ulDebug=DEBUG_LEVEL_0;
        if(pucEnvironmentVariable[0]=='1')
            ulDebug=DEBUG_LEVEL_1;
        if(pucEnvironmentVariable[0]=='2')
            ulDebug=DEBUG_LEVEL_2;
        if(pucEnvironmentVariable[0]=='3')
            ulDebug=DEBUG_LEVEL_3;
        }
    DEBUG_L1S(__THREAD_1__, "Debug level: %ld\n", ulDebug);
                                        /* Allocate parameter structure memory */
    apiretRc=DosAllocMem((VOID **)&puStartupParameters, sizeof(UStartupParameters),
        PAG_COMMIT | PAG_READ | PAG_WRITE);
    if(apiretRc!=NO_ERROR)
        throw(apiretRc);
    memset(puStartupParameters, 0, sizeof(UStartupParameters));
    puStartupParameters->iStatus=iStatus;
    puStartupParameters->ulDebug=ulDebug;
    puStartupParameters->puAnchor=this;
    if((DosQuerySysInfo(QSV_VERSION_MAJOR, QSV_VERSION_MAJOR, 
            &puStartupParameters->ulVersionMajor, sizeof(puStartupParameters->ulVersionMajor))) ||
        (DosQuerySysInfo(QSV_VERSION_MINOR, QSV_VERSION_MINOR, 
            &puStartupParameters->ulVersionMinor, sizeof(puStartupParameters->ulVersionMinor))))
        {
        puStartupParameters->ulVersionMajor=OS2_MAJOR;
        puStartupParameters->ulVersionMinor=OS2_MINOR_200;
        }
                                        /* Query country settings, to try to use the NLS
                                           prefix even if no /Language commandline
                                           option has been specified */
    DosQueryCtryInfo(sizeof(countryinfo), &countrycode, &countryinfo, &ulInfoLength);
    for(iTemp=0; iTemp<(sizeof(ucountrytranslate)/sizeof(ucountrytranslate[0])); iTemp++)
        {
        if(countryinfo.country==ucountrytranslate[iTemp].ulCountry)
            {
            strcpy(puStartupParameters->acNLSPrefix, ucountrytranslate[iTemp].acCountry);
            break;
            }
        }
                                        /* First, try to find the NLS message file, which
                                           of course can override our assumptions */
    if((pcOption=checkCommandlineOption("Language", TRUE))!=0)
        {
                                        /* If just /Language was specified, but not xx afterwards,
                                           for now use the Us NLS prefix */
        if(pcOption==(char *)0xFFFFFFFF)
            strcpy(puStartupParameters->acNLSPrefix, "Us");
        else
            strcpy(puStartupParameters->acNLSPrefix, pcOption);;
        }
                                        /* Second, see if help is requested */
    if((pcOption=checkCommandlineOption("?", TRUE))!=0)
        {
        puStartupParameters->iStatus|=FEATURE_SHOW_HELP;
        }
                                        /* Check if we're running under PM */
    try
        {
        apiretRc=DosLoadModule((PCSZ)ucBuffer, sizeof(ucBuffer)-1, "DOSCALL1", &hmoduleDOSCALL1);
        if(apiretRc!=NO_ERROR)
            throw((ULONG)apiretRc);
        apiretRc=DosQueryProcAddr(hmoduleDOSCALL1, ORD_DOSSMPMPRESENT, NULL, (PFN *)(&pfnDosSmPMPresent));
        if(apiretRc!=NO_ERROR)
            throw((ULONG)apiretRc);
        apiretRc=pfnDosSmPMPresent(&sFlag);
        if(apiretRc!=NO_ERROR)
            throw((ULONG)apiretRc);
                                        /* If we run under AVIO, dynamically load the
                                           profile API (dynamically, because if we
                                           statically link them for AVIO applications
                                           they will no longer run in a fullscreen
                                           obtained via ALT+F1 but hang in the
                                           initialization of some PM DLLs) */
        if(sFlag==TRUE)
            {
#ifdef  __AVIO__
            apiretRc=DosLoadModule((PCSZ)ucBuffer, sizeof(ucBuffer)-1, "PMSHAPI", &hmodulePMSHAPI);
            if(apiretRc!=NO_ERROR)
                throw((ULONG)apiretRc);
            apiretRc=DosQueryProcAddr(hmodulePMSHAPI, ORD_PRF32OPENPROFILE, NULL, (PFN *)(&pfnPrfOpenProfile));
            if(apiretRc!=NO_ERROR)
                throw((ULONG)apiretRc);
            apiretRc=DosQueryProcAddr(hmodulePMSHAPI, ORD_PRF32WRITEPROFILEDATA, NULL, (PFN *)(&pfnPrfWriteProfileData));
            if(apiretRc!=NO_ERROR)
                throw((ULONG)apiretRc);
            apiretRc=DosQueryProcAddr(hmodulePMSHAPI, ORD_PRF32QUERYPROFILEDATA, NULL, (PFN *)(&pfnPrfQueryProfileData));
            if(apiretRc!=NO_ERROR)
                throw((ULONG)apiretRc);
            apiretRc=DosQueryProcAddr(hmodulePMSHAPI, ORD_PRF32CLOSEPROFILE, NULL, (PFN *)(&pfnPrfCloseProfile));
            if(apiretRc!=NO_ERROR)
                throw((ULONG)apiretRc);
#endif  /* __AVIO__ */
                                        /* Finally, if all things worked so far and
                                           we came here, we can now savely assume we're
                                           running under PM */
            puStartupParameters->iStatus|=FEATURE_PM_PRESENT;
            }
        }
    catch(ULONG ulRc) 
        {

        }
#ifdef  __AVIO__
    if(hmodulePMSHAPI);
        DosFreeModule(hmodulePMSHAPI);
#endif  /* __AVIO__ */
    if(hmoduleDOSCALL1);
        DosFreeModule(hmoduleDOSCALL1);
}

/*--------------------------------------------------------------------------------------*\
 * UAnchor: Destructor                                                                  *
\*--------------------------------------------------------------------------------------*/
UAnchor::~UAnchor(void)
{
    if(pcCommandlineCopy!=0)
        allocFree(pcCommandlineCopy);
    if(pcUserCommandline!=0)
        allocFree(pcUserCommandline);
    DEBUG_L1(__THREAD_1__, "");
}

/*--------------------------------------------------------------------------------------*\
 * UAnchor: setUserCommandline()                                                        *
 *          The user may supply a user defined commandline anytime. In addition to the  *
 *          commandline supplied during the application startup, this user define       *
 *          commandline will be taken into account too.                                 *
\*--------------------------------------------------------------------------------------*/
ULONG                   UAnchor::setUserCommandline(char *pcCIDFile)
{
#define CID_FILE_BUFFER                 8192
    char               *pcCIDFileData=0;
    char               *pcS;
    char               *pcD;
    int                 iCIDFile=0;
    int                 iCIDFileSize=0;
    ULONG               ulRc=NO_ERROR;

    pcCIDFileData=allocMemory(CID_FILE_BUFFER);
    memset(pcCIDFileData, '\0', CID_FILE_BUFFER);
                                        /* Try to open and read from the CID file
                                           specified. Not that the whole file up
                                           to the size of the buffer will be read */
    while(TRUE)
        {
        iCIDFile=open(pcCIDFile, O_RDONLY|O_TEXT, 0);    
        if(iCIDFile==-1)
            {
            ulRc=ERROR_ACCESS_DENIED;
            break;
            }
        iCIDFileSize=read(iCIDFile, pcCIDFileData, CID_FILE_BUFFER);
        if(iCIDFileSize==-1)
            {
            ulRc=ERROR_NO_DATA;
            break;
            }
                                        /* If the CID file could be opened, read through
                                           it looking for commandline arguments, that is
                                           /Key Parameter\r\n 
                                           or
                                           -Key Parameter\r\n */
        pcCIDFileData[iCIDFileSize]='\0';
        iCIDFileSize++;
        pcUserCommandline=allocMemory(iCIDFileSize);
        memset(pcUserCommandline, '\0', iCIDFileSize);
        for(pcS=pcCIDFileData, pcD=pcUserCommandline;
            *pcS!='\0';
            )
            {
                                        /* If the current line in the CID file did not
                                           start with / or -, we skip that line, otherwise
                                           copy that line */
            if((*pcS!='/') && (*pcS!='-'))
                {
                while((*pcS!='\r') && (*pcS!='\n') && (*pcS!='\0'))
                    pcS++;
                }
            else
                {
                while((*pcS!='\r') && (*pcS!='\n') && (*pcS!='\0'))
                    *pcD++=*pcS++;
                }
                                        /* If we found the end of a line, terminate the
                                           user commandline by a space, and skip over all
                                           CRLFs in the CID file */
            if((*pcS=='\r') || (*pcS=='\n'))
                {
                *pcD++=' ';
                while((*pcS=='\r') || (*pcS=='\n'))
                    pcS++;
                }
            }
        break;
        }
    if(iCIDFile!=0)
        close(iCIDFile);
    if(pcCIDFileData!=0)
        allocFree(pcCIDFile);
    return(ulRc);
}

/*--------------------------------------------------------------------------------------*\
 * UAnchor: checkCommandlineOption()                                                    *
 *          Check if the commandline contains the passed parameter pcOption within the  *
 *          commandline. This parameter is either a single or a composite argument and  *
 *          may be case sensitive. When specified multiple times, the first one will be *
 *          returned. If the 2nd parameter of a composite parameter is not found, -1    *
 *          will be returned. On error 0 will be returned.                              *
\*--------------------------------------------------------------------------------------*/
char                   *UAnchor::checkCommandlineOption(char *pcOption, BOOL bCompositeOption, BOOL bCaseSensitive)
{
#define     INDEX_CIDFILECOMMANDLINE    1
#define     INDEX_SYSTEMCOMMANDLINE     2
    DEBUG_L2(__THREAD_1__, "");
                                        /* Index to which commandline (specified to started
                                           application, CID file specified by the user, ...)
                                           we look for an option */
    int                 iCommandlineIndex;
    TIB                *ptib=0;         /* Thread information block */
    PIB                *ppib=0;         /* Process information block */
    char               *pcCommandline;  /* Pointer to commandline from PIB */
                                        /* Pointer to the current commandline parameter */
    char               *pcNextParameter;
                                        /* (Composite) option found */
    char               *pcOptionFound=0;
                                        /* Returned option */
    char               *pcOptionReturned=0;
    APIRET              apiretRc;

    for(iCommandlineIndex=INDEX_CIDFILECOMMANDLINE; 
        iCommandlineIndex<=INDEX_SYSTEMCOMMANDLINE;
        iCommandlineIndex++)
        {
                                        /* Free as we always allocate a fresh copy */
        if(pcCommandlineCopy!=0)
            {
            allocFree(pcCommandlineCopy);
            pcCommandlineCopy=0;
            }
                                        /* If no CID file was specified where we have
                                           read some commandline options, skip that
                                           run */
        if(iCommandlineIndex==INDEX_CIDFILECOMMANDLINE)
            {
            if(pcUserCommandline==0)
                continue;
            pcCommandlineCopy=allocString(pcUserCommandline);
            }
                                        /* If the commandline passed to the application
                                           should be searched, get access to it */
        if(iCommandlineIndex==INDEX_SYSTEMCOMMANDLINE)
            {
            apiretRc=DosGetInfoBlocks(&ptib, &ppib);
            if(apiretRc!=NO_ERROR)
                return(pcOptionReturned);
            if((ptib==0) || (ppib==0)) 
                return(pcOptionReturned);
                                        /* The commandline immediately follows the (fully qualified)
                                           ASCIIZ path the application was invoked from, we just now
                                           get a fresh copy as a previous match has modified it */
            pcCommandlineCopy=allocString(strchr(ppib->pib_pchcmd, '\0')+1);
            }
                                        /* Loop as long as commandline parameters are available and
                                           we haven't found it yet */
        for(pcNextParameter=pcCommandline=pcCommandlineCopy;
            (pcNextParameter!=0 && *pcNextParameter!='\0');
            pcNextParameter=strchr(++pcNextParameter, ' '))
            {
                                        /* Skip spaces and see if - or / starts a commandline
                                           parameter */
            while(*pcNextParameter==' ')
                pcNextParameter++;
            if((*pcNextParameter!='/') && (*pcNextParameter!='-'))
                continue;
                                        /* Skip - or / of parameter */
            pcNextParameter++;
                                        /* Test for /"pcOption" or -"pcOption" to get a
                                           option name */
            if(bCaseSensitive==FALSE)
                {
                char   *pcS;
                char   *pcD;
    
                pcOptionFound=pcNextParameter;
                for(pcS=pcNextParameter, pcD=pcOption;
                    *pcS!='\0' && *pcD!='\0';
                    pcS++, pcD++)
                    if((*pcS | 0x20)!=(*pcD | 0x20))
                        {
                        pcOptionFound=0;
                        break;
                        }
                                        /* An option can only be match successfully if the
                                           string we compare has the same length */
                if((*pcD!='\0') || ((*pcS!=' ') && (*pcS!='\0')))
                    pcOptionFound=0;
                }
            else
                {
                char   *pcS=pcNextParameter+strlen(pcOption);
    
                if(strncmp(pcNextParameter, pcOption, strlen(pcOption))==0)
                    pcOptionFound=pcNextParameter;
                if((*pcS!=' ') && (*pcS!='\0'))
                    pcOptionFound=0;
                }
                                        /* For composite parameters check for next one */
            if((bCompositeOption==TRUE) && (pcOptionFound!=0))
                {
                char   *pcCompositeOption=0;
                ULONG   ulCounter=0;
    
                                        /* Find next parameter */
                pcNextParameter=strchr(pcNextParameter, ' ');
                if(pcNextParameter==0)
                    {
                    pcOptionReturned=(char *)0xFFFFFFFF;
                    break;
                    }
                while(*pcNextParameter==' ')
                    pcNextParameter++;
                                        /* If we reached the end-of-line, return
                                           failure */
                if(*pcNextParameter=='\0')
                    {
                    pcOptionReturned=(char *)0xFFFFFFFF;
                    break;
                    }
                                        /* If we reached another option, return
                                           failure */
                if((*pcNextParameter=='-') || (*pcNextParameter=='/'))
                    {
                    pcOptionReturned=(char *)0xFFFFFFFF;
                    break;
                    }
                                        /* For e.g. HPFS filenames, the option may be enclosed 
                                           in quotes ("), which we will remove. Otherwise we
                                           take everything up to the next option or end of
                                           string */
                if(*pcNextParameter=='"')
                    {
                    pcNextParameter++;
                    while((*(pcNextParameter+ulCounter)!='"') && (*(pcNextParameter+ulCounter)!='\0'))
                        ulCounter++;
                    }
                else
                    {
                    while((*(pcNextParameter+ulCounter)!='/') &&
                        (*(pcNextParameter+ulCounter)!='-') && 
                        (*(pcNextParameter+ulCounter)!='\0'))
                        ulCounter++;
                                        /* Adjust for trailing characters */
                    while((*(pcNextParameter+ulCounter-1)=='\0') ||
                        (*(pcNextParameter+ulCounter-1)==' '))
                        ulCounter--;
                    }
                                        /* Ensure correct termination of filename */
                *(pcNextParameter+ulCounter)='\0';
                                        /* Save the composite option found */
                pcOptionFound=pcNextParameter;
                }
            if(pcOptionFound!=0)
                pcOptionReturned=pcOptionFound;
            }
                                        /* If we found a match, we don't need further
                                           searching */
        if((pcOptionReturned!=0) && (pcOptionReturned!=(char *)0xFFFFFFFF))
            break;
        }
    return(pcOptionReturned);
}

/*--------------------------------------------------------------------------------------*\
 * UAnchor: accessProfile()                                                             *
 *          Read or write a value from or to the profile (INI file).                    *
\*--------------------------------------------------------------------------------------*/
int                     UAnchor::accessProfile(char *pcApplication, char *pcKey, char *pcValue, ULONG ulValueSize, BOOL bRead)
{
    DEBUG_L3(__THREAD_1__, "");
    int     iSuccess=TRUE;
    HINI   *phiniProfile=(HINI *)&(puStartupParameters->ahProfile[PATH_FILE_INI]);

#ifdef  __AVIO__
                                        /* Prevent access to profile APIs when not running
                                           under PM to allow AVIO version to run without PM 
                                           (e.g. at a booted command fullscreen from ALT+F1) */
    if(puStartupParameters->iStatus & FEATURE_PM_PRESENT)
        {
        if(*phiniProfile==0)
            *phiniProfile=pfnPrfOpenProfile(puStartupParameters->hab, puStartupParameters->acProfile[PATH_FILE_INI]);
        if(*phiniProfile!=NULLHANDLE)
            {
            if(bRead==TRUE)
                {       
                                        /* If a size is passed, we should read from the profile */
                iSuccess=pfnPrfQueryProfileData(*phiniProfile, pcApplication, pcKey, pcValue, &ulValueSize);
                }
            else
                {
                                        /* If no size is passed, we should write to the profile */
                iSuccess=pfnPrfWriteProfileData(*phiniProfile, pcApplication, pcKey, pcValue, ulValueSize);
                }
            }
        }
    else
        iSuccess=FALSE;
#endif  /* __AVIO__ */
#ifdef  __PM__
    if(*phiniProfile==0)
        *phiniProfile=PrfOpenProfile(puStartupParameters->hab, puStartupParameters->acProfile[PATH_FILE_INI]);
    if(*phiniProfile!=NULLHANDLE)
        {
        if(bRead==TRUE)
            {       
                                        /* If a size is passed, we should read from the profile */
            iSuccess=PrfQueryProfileData(*phiniProfile, pcApplication, pcKey, pcValue, &ulValueSize);
            }
        else
            {
                                        /* If no size is passed, we should write to the profile */
            iSuccess=PrfWriteProfileData(*phiniProfile, pcApplication, pcKey, pcValue, ulValueSize);
            }
        }
#endif  /* __PM__ */
    return(iSuccess);
}

int                     UAnchor::accessProfile(char *pcApplication, char *pcKey, LONG *plValue, BOOL bRead)
{
    DEBUG_L3(__THREAD_1__, "");
    ULONG   ulValueSize=sizeof(*plValue);
    int     iSuccess=TRUE;
    HINI   *phiniProfile=(HINI *)&(puStartupParameters->ahProfile[PATH_FILE_INI]);

#ifdef  __AVIO__
                                        /* Prevent access to profile APIs when not running
                                           under PM to allow AVIO version to run without PM 
                                           (e.g. at a booted command fullscreen from ALT+F1) */
    if(puStartupParameters->iStatus & FEATURE_PM_PRESENT)
        {
        if(*phiniProfile==0)
            *phiniProfile=pfnPrfOpenProfile(puStartupParameters->hab, puStartupParameters->acProfile[PATH_FILE_INI]);
        if(*phiniProfile!=NULLHANDLE)
            {
            if(bRead==TRUE)
                {       
                                        /* If a size is passed, we should read from the profile */
                iSuccess=pfnPrfQueryProfileData(*phiniProfile, pcApplication, pcKey, plValue, &ulValueSize);
                }
            else
                {
                                        /* If no size is passed, we should write to the profile */
                iSuccess=pfnPrfWriteProfileData(*phiniProfile, pcApplication, pcKey, plValue, ulValueSize);
                }
            }
        }
    else
        iSuccess=FALSE;
#endif  /* __AVIO__ */
#ifdef  __PM__
    if(*phiniProfile==0)
        *phiniProfile=PrfOpenProfile(puStartupParameters->hab, puStartupParameters->acProfile[PATH_FILE_INI]);
    if(*phiniProfile!=NULLHANDLE)
        {
        if(bRead==TRUE)
            {       
                                        /* If a size is passed, we should read from the profile */
            iSuccess=PrfQueryProfileData(*phiniProfile, pcApplication, pcKey, plValue, &ulValueSize);
            }
        else
            {
                                        /* If no size is passed, we should write to the profile */
            iSuccess=PrfWriteProfileData(*phiniProfile, pcApplication, pcKey, plValue, ulValueSize);
            }
        }
#endif  /* __PM__ */
    return(iSuccess);
}

/*--------------------------------------------------------------------------------------*\
 * UAnchor: loadMessage()                                                               *
 *          Loads a message from the NLS enabled message file                           *
 *          acProfile[PATH_FILE_MESSAGE_NLS] and strip trailing whitespaces. If message *
 *          couldn't be found acProfile[PATH_FILE_MESSAGE_US] is tried instead.         *
\*--------------------------------------------------------------------------------------*/
char                   *UAnchor::loadMessage(ULONG ulMessage, char *pcParm0, char *pcParm1, 
                                                              char *pcParm2, char *pcParm3,
                                                              char *pcParm4)
{
    DEBUG_L3(__THREAD_1__, "");
                                        /* Table of message parameters */
    char   *acParameters[5]={0, 0, 0, 0, 0};
    char    acMessage[2048]="";         /* Message text loaded from message file */
    char   *pcMessageFormatted;         /* Message text formatted to common syntax */
    ULONG   ulParameters=0;             /* Number of message parameters passed */
    ULONG   ulMessageLength=0;          /* Length of loaded and parsed message */
    APIRET  apiretRc=NO_ERROR;  
    
                                        /* Load message parameters into table */
    if(pcParm0!=0)
        acParameters[ulParameters++]=pcParm0;
    if(pcParm1!=0)
        acParameters[ulParameters++]=pcParm1;
    if(pcParm2!=0)
        acParameters[ulParameters++]=pcParm2;
    if(pcParm3!=0)
        acParameters[ulParameters++]=pcParm3;
    if(pcParm4!=0)
        acParameters[ulParameters++]=pcParm4;
    memset(acMessage, 0, sizeof(acMessage));
    apiretRc=DosGetMessage((CHAR **)acParameters, ulParameters, acMessage, 
        sizeof(acMessage)-sizeof("???0000I: "), ulMessage, 
        (PSZ)puStartupParameters->acProfile[PATH_FILE_MESSAGE_NLS], &ulMessageLength);
    if(apiretRc!=NO_ERROR)
        {
        if(ulMessage!=NLSMSG_MESSAGEFILETEST)
            {
                                        /* If the NLS messagefile failed, fall back to the English
                                           message. Directly load the Us English message which we
                                           know exists as the Us English message file was shipped */
            memset(acMessage, 0, sizeof(acMessage));
            apiretRc=DosGetMessage((CHAR **)acParameters, ulParameters, acMessage, 
                sizeof(acMessage)-sizeof("???0000I: "), ulMessage, 
                (PSZ)puStartupParameters->acProfile[PATH_FILE_MESSAGE_US], &ulMessageLength);
            if(apiretRc!=NO_ERROR)
                {
                sprintf(acMessage, "%s0001E: Can't load message %d from messagefile %s. "
                    "Please report this problem to the author.", 
                    BLDLEVEL_PREFIX, ulMessage, 
                    puStartupParameters->acProfile[PATH_FILE_MESSAGE_US]);
                displayMessage(allocString(acMessage), FALSE);
                return(allocString(""));
                }
            }
        else
                                        /* For the test message return NULL to signal it
                                           could not be loaded */
            return(0);
        }
                                        /* DosGetMessage() returns message number only for error and
                                           warning messages, so we have to adjust them ourselves */
    if(!memcmp(acMessage, puStartupParameters->pcPrefix, strlen(puStartupParameters->pcPrefix)))
        {
                                        /* We have a error or warning message */
        pcMessageFormatted=allocMemory(ulMessageLength+sizeof("???0000I: "));       
        sprintf(pcMessageFormatted, "%s%04d%c: %s",
            puStartupParameters->pcPrefix, ulMessage, acMessage[9], &acMessage[11]);         
        }
    else
        {
                                        /* We have anything but an error or warning message, 
                                           remove prefix for userdefined messages (reserve
                                           some space to manipulations) */
        pcMessageFormatted=allocMemory(ulMessageLength+sizeof("???0000I: ")+16);
                                        /* Messages prefixed with 'X' should exclude the
                                           message number (e.g. useful to load strings that
                                           will be used for dialog controls */
        if(acMessage[0]=='X')
            sprintf(pcMessageFormatted, "%s", &acMessage[2]);
                                        /* For all other messages format the message */
        else         
            sprintf(pcMessageFormatted, "%s%04d%c: %s",
                puStartupParameters->pcPrefix, ulMessage, acMessage[0], &acMessage[2]);         
        }
    return(pcMessageFormatted);
}

char                   *UAnchor::loadMessage(ULONG ulMessage, ULONG ulParm0)
{
    DEBUG_L3(__THREAD_1__, "");
    char    acParm0[32];

    _itoa(ulParm0, acParm0, 10);
    return(loadMessage(ulMessage, acParm0));
}

char                   *UAnchor::loadMessageHex(ULONG ulMessage, ULONG ulParm0)
{
    DEBUG_L3(__THREAD_1__, "");
    char    acParm0[32];

    _itoa(ulParm0, acParm0, 16);
    return(loadMessage(ulMessage, acParm0));
}

char                   *UAnchor::loadMessage(ULONG ulMessage, ULONG ulParm0, ULONG ulParm1)
{
    DEBUG_L3(__THREAD_1__, "");
    char    acParm0[32];
    char    acParm1[32];

    _itoa(ulParm0, acParm0, 10);
    _itoa(ulParm1, acParm1, 10);
    return(loadMessage(ulMessage, acParm0, acParm1));
}

char                   *UAnchor::loadMessage(ULONG ulMessage, ULONG ulParm0, ULONG ulParm1, ULONG ulParm2)
{
    DEBUG_L3(__THREAD_1__, "");
    char    acParm0[32];
    char    acParm1[32];
    char    acParm2[32];

    _itoa(ulParm0, acParm0, 10);
    _itoa(ulParm1, acParm1, 10);
    _itoa(ulParm2, acParm2, 10);
    return(loadMessage(ulMessage, acParm0, acParm1, acParm2));
}

/*--------------------------------------------------------------------------------------*\
 * UAnchor: loadMessageStrip()                                                          *
 *          Loads a message from the NLS enabled message file                           *
 *          acProfile[PATH_FILE_MESSAGE_NLS] and strips trailing whitespaces.           *
\*--------------------------------------------------------------------------------------*/
char                   *UAnchor::loadMessageStrip(ULONG ulMessage, char *pcParm0, char *pcParm1, 
                                                                   char *pcParm2, char *pcParm3,
                                                                   char *pcParm4)
{
    DEBUG_L3(__THREAD_1__, "");
    char   *pcMessage=stripTrailing(loadMessage(ulMessage, pcParm0, pcParm1, pcParm2, pcParm3, pcParm4));
                                        /* Messages prefixed with 'Q' are queries where a
                                           space should be appended so that the user's
                                           response is separated from the question */
    if(pcMessage[7]=='Q')
        strcat(pcMessage, " ");
    return(pcMessage);
}

char                   *UAnchor::loadMessageStrip(ULONG ulMessage, ULONG ulParm0)
{
    DEBUG_L3(__THREAD_1__, "");
    char   *pcMessage=stripTrailing(loadMessage(ulMessage, ulParm0));
    if(pcMessage[7]=='Q')
        strcat(pcMessage, " ");
    return(pcMessage);
}

char                   *UAnchor::loadMessageStrip(ULONG ulMessage, ULONG ulParm0, ULONG ulParm1)
{
    DEBUG_L3(__THREAD_1__, "");
    char   *pcMessage=stripTrailing(loadMessage(ulMessage, ulParm0, ulParm1));
    if(pcMessage[7]=='Q')
        strcat(pcMessage, " ");
    return(pcMessage);
}

char                   *UAnchor::loadMessageStrip(ULONG ulMessage, ULONG ulParm0, ULONG ulParm1, ULONG ulParm2)
{
    DEBUG_L3(__THREAD_1__, "");
    char   *pcMessage=stripTrailing(loadMessage(ulMessage, ulParm0, ulParm1, ulParm2));
    if(pcMessage[7]=='Q')
        strcat(pcMessage, " ");
    return(pcMessage);
}

/*--------------------------------------------------------------------------------------*\
 * UAnchor::displayFormatted()                                                          *
 *          Display a message formatted for AVIO windows, that is ensure that the word  *
 *          breaks don't look too bad. The passed message is freed.                     *
\*--------------------------------------------------------------------------------------*/
UAnchor                &UAnchor::displayFormatted(char *pcMessage)
{
    ULONG   ulTemp;
    char    acLine[81];
    char   *pcCurrentLine;
    char   *pcLookup;

    logMessage(pcMessage);
    pcCurrentLine=pcMessage;
                                        /* Copy the next up to 80 characters */
    do  {
        memset(acLine, '\0', sizeof(acLine));
        ulTemp=strlen(pcCurrentLine);
        if(ulTemp>(sizeof(acLine)-1))
            ulTemp=(sizeof(acLine)-1);
        memcpy(acLine, pcCurrentLine, ulTemp);
        if(strlen(pcCurrentLine)>=(sizeof(acLine)-1))
            {
                                        /* Find any \n, as this terminates our line
                                           prematurely */
            pcLookup=strchr(acLine, '\n');
            if(pcLookup!=0)
                {
                *pcLookup='\0';
                }
            else
                {
                                        /* Find last space to replace it by \0. Then insert 
                                           a new line before if there is still something
                                           left to fill another line of up to 80 characters */
                pcLookup=strrchr(acLine, ' ');
                if(pcLookup!=0)
                    *pcLookup='\0';
                }
            DosWrite(1, acLine, strlen(acLine), &ulTemp);
                                        /* Advance to next word and skip space */
            if(pcLookup==0)
                pcCurrentLine+=(sizeof(acLine)-1);
            else
                pcCurrentLine+=((int)pcLookup-(int)acLine+1);
            if(strlen(pcCurrentLine))
                DosWrite(1, "\r\n", sizeof("\r\n")-1, &ulTemp);
            }
        else
            {
            DosWrite(1, pcCurrentLine, strlen(pcCurrentLine), &ulTemp);
            break;
            }
        } while(strlen(pcCurrentLine));
    allocFree(pcMessage);
    return(*this);
}


/*--------------------------------------------------------------------------------------*\
 * UAnchor::displayMessage()                                                            *
 *          Display a message on Logo window or with a message box. The message box     *
 *          message is posted to the Logo client window, so it can be called from       *
 *          another thread as posting does the thread context switch. The passed        *
 *          message is freed.                                                           *
\*--------------------------------------------------------------------------------------*/
UAnchor                &UAnchor::displayMessage(char *pcMessage, BOOL bLogoWindow)
{
    ULONG   ulTemp;

    DEBUG_L3(__THREAD_1__, "");
    if(pcMessage==0)
        pcMessage=allocString("Unknown error");
    if(bLogoWindow==TRUE)
        {
#ifdef  __PM__
                                        /* Display message on Logo window (which also
                                           does the cleanup) */
        WinPostMsg(puStartupParameters->hwndClient, WM_STARTUPMESSAGE, MPFROMP(pcMessage), NULL);
#endif  /* __PM__ */
#ifdef  __AVIO__
                                        /* Just output the message to stdout (via OS/2 as
                                           printf() will send the output to the PMPRINTF
                                           window not the calling commandline) and do
                                           the cleanup */
        displayFormatted(pcMessage);
        allocFree(pcMessage);
#endif  /* __AVIO__ */
        }
    else
        {
                                        /* Display a message box modal to the Logo window
                                           if the message is not just "" (which is returned
                                           when a message could not even be loaded from the US
                                           NLS message file shipped with the program) as
                                           during loadMessage() a request to show the problem
                                           with a messagebox is done anyway */
        if(strlen(pcMessage)>(sizeof("???0000I: ")-1))                                                                        
            {
#ifdef  __PM__
                                        /* Display the message in a message box (which also
                                           does the cleanup) */
            WinPostMsg(puStartupParameters->hwndClient, WM_MESSAGEBOX, MPFROMP(pcMessage), NULL);
#endif  /* __PM__ */
#ifdef  __AVIO__
                                        /* Just output the message to stdout (via OS/2 as
                                           printf() will send the output to the PMPRINTF
                                           window not the calling commandline) and do
                                           the cleanup */
            displayFormatted(pcMessage);
            allocFree(pcMessage);
#endif  /* __AVIO__ */
            }
                                        /* Switch thread context for earlier processing of
                                           message box display */
        DosSleep(0);
        }
    return(*this);
}

/*--------------------------------------------------------------------------------------*\
 * UAnchor::displayMessage()                                                            *
 *          Display a message modal to a specified window. Note that the caller and the *
 *          window should be running in the same thread. The passed message will be     *
 *          freed.                                                                      *
\*--------------------------------------------------------------------------------------*/
ULONG                   UAnchor::displayMessage(HWND hwndOwner, char *pcMessage, ULONG ulStyle, char *pcErrorModule, LONG lErrorLine)
{
    DEBUG_L3(__THREAD_1__, "");
    char   *pcAdjustedMessage=pcMessage;
    ULONG   ulRc;

                                        /* Add module and linenumber information if requested */
    if((pcErrorModule!=0) && (lErrorLine!=0))
        {   
        pcAdjustedMessage=allocMemory(strlen(pcMessage)+256);
        sprintf(pcAdjustedMessage, "%s\nModule: %s\nLinenumber: %d\n",
            pcMessage, pcErrorModule, lErrorLine);
        allocFree(pcMessage);
        }
    ulRc=displayMessage(hwndOwner, ulStyle, pcAdjustedMessage);
    allocFree(pcAdjustedMessage);
    return(ulRc);
}

ULONG                   UAnchor::displayMessage(HWND hwndOwner, ULONG ulStyle, char *pcMessage)
{
    ULONG   ulTemp;

    DEBUG_L3(__THREAD_1__, "");

    char    acMessage[128]="";
    char    acTitlebar[TITLEBARLENGTH];
    char   *pcAdjustedMessage=pcMessage;
    ULONG   ulRc;

    sprintf(acMessage, "%s0000E Internal error, Null message requested to be displayed", BLDLEVEL_PREFIX);
    if(pcMessage==0)
        pcAdjustedMessage=pcMessage=acMessage;
    memset(acTitlebar, ' ', sizeof(acTitlebar));
    acTitlebar[TITLEBARLENGTH-1]='\0';
    memcpy(acTitlebar, puStartupParameters->pcApplicationName, strlen(puStartupParameters->pcApplicationName));
#ifdef  __PM__
                                        /* Display a messagebox */
    ulRc=WinMessageBox(HWND_DESKTOP, hwndOwner, (PSZ)pcAdjustedMessage, (PSZ)acTitlebar,
        ID_DISPLAYMESSAGE, ulStyle | MB_DEFBUTTON1 | MB_MOVEABLE);
#endif  /* __PM__ */
#ifdef  __AVIO__
                                        /* Just output the message to stdout (via OS/2 as
                                           printf() will send the output to the PMPRINTF
                                           window not the calling commandline) and do
                                           the cleanup */
    logMessage(pcAdjustedMessage);
    DosWrite(1, pcAdjustedMessage, strlen(pcAdjustedMessage), &ulTemp);
    DosWrite(1, "\r\n", sizeof("\r\n")-1, &ulTemp);
#endif  /* __AVIO__ */
    allocFree(pcMessage);
    return(ulRc);
}

/*--------------------------------------------------------------------------------------*\
 * UAnchor::displayMessage()                                                            *
 *          Display a message in a message box modal to the Logo window. As this is     *
 *          done via posting to the Logo client window, the source could be another     *
 *          thread too. The passed message will be freed.                               *
\*--------------------------------------------------------------------------------------*/
UAnchor                &UAnchor::displayMessage(char *pcMessage)
{
    ULONG   ulTemp;
    char   *pcMessageCode;

    DEBUG_L3(__THREAD_1__, "");
#ifdef  __PM__
#define     LEVEL_INFORMATION           0x00000001
#define     LEVEL_WARNING               0x00000002
#define     LEVEL_ERROR                 0x00000004
                                        /* Display a message box */
    char    acTitlebar[TITLEBARLENGTH];
    ULONG   ulStyle=(MB_OK | MB_DEFBUTTON1 | MB_MOVEABLE);
    ULONG   ulLevel=0;

    memset(acTitlebar, ' ', sizeof(acTitlebar));
    acTitlebar[TITLEBARLENGTH-1]='\0';
    memcpy(acTitlebar, puStartupParameters->pcApplicationName, strlen(puStartupParameters->pcApplicationName));
                                        /* Search message for most severe error code
                                           as for PM the message may have accumulated
                                           multiple error messages */
    pcMessageCode=strstr(pcMessage, BLDLEVEL_PREFIX);
    while(pcMessageCode)
        {
        if(pcMessageCode[8]==':')
            {
            if(pcMessageCode[7]=='I')
                ulLevel|=LEVEL_INFORMATION;
            if(pcMessageCode[7]=='W')
                ulLevel|=LEVEL_WARNING;
            if(pcMessageCode[7]=='E')
                ulLevel|=LEVEL_ERROR;
            pcMessageCode=strstr(pcMessageCode+9, BLDLEVEL_PREFIX);
            }
        else
            pcMessageCode=strstr(pcMessageCode+1, BLDLEVEL_PREFIX);
        } 
    if(ulLevel & LEVEL_ERROR)
        ulStyle|=MB_ERROR;
    else if(ulLevel & LEVEL_WARNING)
        ulStyle|=MB_WARNING;
    else if(ulLevel & LEVEL_INFORMATION)
        ulStyle|=MB_INFORMATION;
    puStartupParameters->iStatus|=FEATURE_SHOW_MESSAGEBOX;
    WinMessageBox(HWND_DESKTOP, puStartupParameters->hwndFrame, (PSZ)pcMessage, (PSZ)acTitlebar,
        ID_DISPLAYMESSAGE, ulStyle);
    puStartupParameters->iStatus&=(~FEATURE_SHOW_MESSAGEBOX);
    if(puStartupParameters->iStatus & FEATURE_SHOW_CLOSEBUTTON)
        {
        WinFocusChange(HWND_DESKTOP, puStartupParameters->hwndFrame, 0);
        WinSetFocus(HWND_DESKTOP, puStartupParameters->hwndLogoPushbutton);
        }
#endif  /* __PM__ */
#ifdef  __AVIO__
                                        /* Just output the message to stdout (via OS/2 as
                                           printf() will send the output to the PMPRINTF
                                           window not the calling commandline) and do
                                           the cleanup */
    logMessage(pcMessage);
    DosWrite(1, pcMessage, strlen(pcMessage), &ulTemp);
    DosWrite(1, "\r\n", sizeof("\r\n")-1, &ulTemp);
#endif  /* __AVIO__ */
    allocFree(pcMessage);
    return(*this);
}

/*--------------------------------------------------------------------------------------*\
 * UAnchor: logMessage()                                                                *
 *          If requested, log message to logfile. During initialization pcMessage is    *
 *          0.                                                                          *
\*--------------------------------------------------------------------------------------*/
ULONG                   UAnchor::logMessage(char *pcMessage)
{
    DEBUG_L2(__THREAD_1__, "");
    ULONG   ulAction;
    DATETIME
            datetimeCurrent;
    char    pcDateTimeCurrent[32];
    ULONG   ulWritten;
    ULONG   ulStatus=NO_ERROR;
    
    if(puStartupParameters->iStatus & FEATURE_LOGFILE)
        {
        if(pcMessage==0)
            {
            ulStatus=DosOpen(puStartupParameters->acProfile[PATH_FILE_LOGFILE],
                &puStartupParameters->ahProfile[PATH_FILE_LOGFILE],
                &ulAction, 0, FILE_NORMAL,
                OPEN_ACTION_CREATE_IF_NEW | OPEN_ACTION_REPLACE_IF_EXISTS,
                OPEN_FLAGS_FAIL_ON_ERROR | OPEN_FLAGS_SEQUENTIAL | OPEN_SHARE_DENYWRITE | OPEN_ACCESS_WRITEONLY, 
                0);
            return(ulStatus);
            }
        DosGetDateTime(&datetimeCurrent);
        sprintf(pcDateTimeCurrent, "%02d.%02d.%04d %02d:%02d:%02d: ",
            (int)datetimeCurrent.day, (int)datetimeCurrent.month, (int)datetimeCurrent.year,
            (int)datetimeCurrent.hours, (int)datetimeCurrent.minutes, (int)datetimeCurrent.seconds);
        ulWritten=strlen(pcDateTimeCurrent);
        ulStatus=DosWrite(puStartupParameters->ahProfile[PATH_FILE_LOGFILE],
            pcDateTimeCurrent, ulWritten, &ulWritten);
        if(ulStatus!=NO_ERROR) 
            return(ulStatus);
        ulWritten=strlen(pcMessage);
        ulStatus=DosWrite(puStartupParameters->ahProfile[PATH_FILE_LOGFILE],
            pcMessage, ulWritten, &ulWritten);
        if(ulStatus!=NO_ERROR) 
            return(ulStatus);
        if((pcMessage[ulWritten-1]!='\r') && (pcMessage[ulWritten-1]!='\n'))
            {
            ulWritten=strlen("\r\n");
            ulStatus=DosWrite(puStartupParameters->ahProfile[PATH_FILE_LOGFILE],
                "\r\n", ulWritten, &ulWritten);
            }
        }
    return(ulStatus);
}

/*--------------------------------------------------------------------------------------*\
 * UAnchor: showWindow()                                                                *
 *          Show or hide the UStartupWindow Logo window. Note that this function must   *
 *          be invoked from the Logo window thread.                                     *
\*--------------------------------------------------------------------------------------*/
ULONG                   UAnchor::showWindow(BOOL bShowLogoWindow, BOOL bShowCloseButton, ULONG ulError)
{
    DEBUG_L2(__THREAD_1__, "");
    ULONG   ulStatus=NO_ERROR;

    if(bShowLogoWindow==TRUE) ulStatus|=LOGO_SHOWLOGOWINDOW;
    if(bShowCloseButton==TRUE) ulStatus|=LOGO_SHOWCLOSEBUTTON;
    if(ulError!=NO_ERROR) ulStatus|=LOGO_CLOSEWITHERROR;
#ifdef  __PM__
                                        /* Inform window via message to show logo
                                           window with Close button. If processing
                                           completed without an error, also quit
                                           application automatically */
    WinPostMsg(puStartupParameters->hwndClient, WM_SHOWLOGOWINDOW, MPFROMLONG(ulStatus), NULL);
    if((ulError==NO_ERROR) && !(puStartupParameters->iStatus & FEATURE_ONLINEHELP))
        WinPostMsg(puStartupParameters->hwndClient, WM_CLOSE, NULL, NULL);
#endif  /* __PM__ */
#ifdef  __AVIO__
                                        /* Nothing required here as in AVIO mode we output
                                           anything to stdout anyway */
#endif  /* __AVIO__ */
    return(NO_ERROR);
}

/*--------------------------------------------------------------------------------------*\
 * UAnchor::allocMemory()                                                               *
 *          Allocate memory outside any C++ heap, because this allows us to share data  *
 *          between the 2 heaps of UStartup and code loaded from the code DLL.          *
\*--------------------------------------------------------------------------------------*/
char                   *UAnchor::allocMemory(ULONG ulSize)
{
    DEBUG_L3(__THREAD_1__, "");
    char   *pcAllocation=0;

    if(DosAllocMem((VOID **)&pcAllocation, ulSize, PAG_COMMIT | PAG_READ | PAG_WRITE)==NO_ERROR)
        {
        memset(pcAllocation, '\0', ulSize);
        return(pcAllocation);
        }
    else
        {
        if(ulSize<sizeof(puStartupParameters->acMessage))
            {
            memset(puStartupParameters->acMessage, '\0', sizeof(puStartupParameters->acMessage));
            return(puStartupParameters->acMessage);
            }
        else
            return(0);
        }
}

/*--------------------------------------------------------------------------------------*\
 * UAnchor::allocString()                                                               *
 *          Allocate memory outside any C++ heap, because this allows us to share data  *
 *          between the 2 heaps of UStartup and code loaded from the code DLL.          *
\*--------------------------------------------------------------------------------------*/
char                   *UAnchor::allocString(char *pcString)
{
    DEBUG_L3(__THREAD_1__, "");
    char   *pcAllocation=pcString;
    ULONG   ulSize=0;

    if(pcString)
        {
        ulSize=strlen(pcString)+1;
        pcAllocation=allocMemory(ulSize);
        if(pcAllocation)
            memcpy(pcAllocation, pcString, ulSize);
        }
    return(pcAllocation);
}

/*--------------------------------------------------------------------------------------*\
 * UAnchor::concatStrings()                                                             *
 *          Allocate memory enough for concatenating 2 strings, concatenate them and    *
 *          deallocate them before returning concatenated string.                       *
\*--------------------------------------------------------------------------------------*/
char                   *UAnchor::concatStrings(char *pcString1, char *pcString2)
{
    DEBUG_L3(__THREAD_1__, "");
    char   *pcConcatString=0;
    ULONG   ulSize=0;

    if(pcString1 && pcString2)
        {
        ulSize=strlen(pcString1)+strlen(pcString2)+1;
        pcConcatString=allocMemory(ulSize);
        if(pcConcatString)
            {
            strcpy(pcConcatString, pcString1);
            strcat(pcConcatString, pcString2);
            allocFree(pcString1);
            allocFree(pcString2);
            }
        }
    return(pcConcatString);
}

/*--------------------------------------------------------------------------------------*\
 * UAnchor::allocFree()                                                                 *
 *          Free allocation outside any C++ heap.                                       *
\*--------------------------------------------------------------------------------------*/
void                    UAnchor::allocFree(char *pcAllocation)
{
    DEBUG_L3(__THREAD_1__, "");
    if(pcAllocation)
        DosFreeMem(pcAllocation);
}

/*--------------------------------------------------------------------------------------*\
 * UAnchor::getProfile()                                                                *
 *          Return the fully qualified path of the profile requested.                   *
\*--------------------------------------------------------------------------------------*/
char                   *UAnchor::getProfile(ULONG ulProfile)
{
    DEBUG_L3(__THREAD_1__, "");
    if(ulProfile>=NUM_PROFILES)
        return(0);
    return(puStartupParameters->acProfile[ulProfile]);
}

/*--------------------------------------------------------------------------------------*\
 * UAnchor::getOS2VersionMajor()                                                        *
 *          Return OS/2's major version we're running under.                            *
\*--------------------------------------------------------------------------------------*/
ULONG                   UAnchor::getOS2VersionMajor(void)
{
    DEBUG_L3(__THREAD_1__, "");
    return(puStartupParameters->ulVersionMajor);
}

/*--------------------------------------------------------------------------------------*\
 * UAnchor::getOS2VersionMinor()                                                        *
 *          Return OS/2's minor version we're running under.                            *
\*--------------------------------------------------------------------------------------*/
ULONG                   UAnchor::getOS2VersionMinor(void)
{
    DEBUG_L3(__THREAD_1__, "");
    return(puStartupParameters->ulVersionMinor);
}

/*--------------------------------------------------------------------------------------*\
 * UAnchor: stripTrailing()                                                             *
 *          Strip trailing whitespaces and CRLF from a string.                          *
\*--------------------------------------------------------------------------------------*/
char                   *UAnchor::stripTrailing(char *pcMessage)
{
    DEBUG_L3(__THREAD_1__, "");
    char   *pcTemp=strchr(pcMessage, '\0');

    while(pcTemp>=pcMessage) 
        {
        if((*pcTemp=='\r') || (*pcTemp=='\n') || (*pcTemp=='\t') || (*pcTemp==' '))
            *pcTemp='\0';
        if(*pcTemp!='\0')
            break;
        pcTemp--;
        }
    return(pcMessage);
}

/****************************************************************************************\
 * Class: UStartupWindow                                                                *
\****************************************************************************************/
/*--------------------------------------------------------------------------------------*\
 * UStartupWindow: Constructor(s)                                                       *
\*--------------------------------------------------------------------------------------*/
UStartupWindow::UStartupWindow(int argc, char **argv, int iStatus, char *pcApplicationName, char *pcPrefix, int iSignature) :
                UAnchor(argc, argv, iStatus, pcApplicationName, pcPrefix)
{
    DEBUG_L2(__THREAD_1__, "");
    ULONG   ulError=NO_ERROR;
    HAB     hab=0;
    HMQ     hmq=0;
    HWND    hwndFrame=0;
    HWND    hwndClient=0;
    char   *pcLanguage=0;

                                        /* Check prerequisites and throw execption when
                                           required */
    if((argc==0) || (argv==0) || (pcApplicationName==0) || (pcPrefix==0))
        ulError|=ERROR_NOENVIRONMENT;
    if(puStartupParameters==0)
        ulError|=ERROR_ALLOCATE_MEMORY;
                                        /* Initialize PM (required to display message boxes) */
#ifdef  __PM__
    hab=WinInitialize(0);
    hmq=WinCreateMsgQueue(hab, 0);
    if((hab==0) || (hmq==0))
        ulError|=ERROR_INITIALIZE_PM;
                                        /* Create startup Logo window. Register client class 
                                           (including 4 bytes window words) and create frame 
                                           window invisible */
    if(WinRegisterClass(hab, (PSZ)pcPrefix, (PFNWP)UStartupWindowProc,
        CS_SIZEREDRAW | CS_SAVEBITS | CS_MOVENOTIFY, 4))
        {
        ULONG   ulFCFFlags=0;           

        if(iStatus & FEATURE_ADD_WINDOWLIST)
            ulFCFFlags|=(FCF_TASKLIST | FCF_ICON);
        hwndFrame=WinCreateStdWindow(HWND_DESKTOP, 0, &ulFCFFlags,
            (PSZ)pcPrefix, (PSZ)pcApplicationName, 0, (HMODULE)0, 1, &hwndClient);
        }    
    if(hwndFrame==0)
        ulError|=ERROR_WINDOW_FAILED;
                                        /* Load our Logo object into client window window words,
                                           so that the client window procedure will be able to
                                           retrieve them */
    if(WinSetWindowULong(hwndClient, QWL_USER, (ULONG)this)!=TRUE)
        ulError|=ERROR_WINDOWWORDS_FAILED;
#endif  /* __PM__ */
                                        /* Throw error if something didn't work as expected */
    if(ulError)
        {
        static char     acStartupError[256];

        memset(acStartupError, '\0', sizeof(acStartupError));
        if(ulError & ERROR_NOENVIRONMENT)
            strcat(acStartupError, "No environment found. ");
        if(ulError & ERROR_ALLOCATE_MEMORY)
            strcat(acStartupError, "Can't allocate memory for startup processing. ");
        if(ulError & ERROR_INITIALIZE_PM)
            strcat(acStartupError, "Can't initialize PM or create message queue. ");
        if(ulError & ERROR_WINDOW_FAILED)
            strcat(acStartupError, "Can't create Logo frame window. ");
        if(ulError & ERROR_WINDOWWORDS_FAILED)
            strcat(acStartupError, "Can't set Logo window words. ");
        throw((char *)&acStartupError[0]);
        }
                                        /* Now save parameters into instance data */
    puStartupParameters->argc=argc;
    puStartupParameters->argv=argv;
    puStartupParameters->pcApplicationName=pcApplicationName;
    puStartupParameters->pcPrefix=pcPrefix;
    puStartupParameters->iSignature=iSignature;
    puStartupParameters->hab=hab;                                
#ifdef  __PM__
    puStartupParameters->hmq=hmq;                                
    puStartupParameters->hwndFrame=hwndFrame;                                
    puStartupParameters->hwndClient=hwndClient;                                
#endif  /* __PM__ */
}

/*--------------------------------------------------------------------------------------*\
 * UStartupWindow: Destructor                                                           *
\*--------------------------------------------------------------------------------------*/
UStartupWindow::~UStartupWindow(void)
{
    DEBUG_L1(__THREAD_1__, "");
    HINI               *phiniProfile=(HINI *)&(puStartupParameters->ahProfile[PATH_FILE_INI]);

    if(puStartupParameters!=0)
        {
#ifdef  __AVIO__
        if((puStartupParameters->iStatus & FEATURE_PM_PRESENT) && (*phiniProfile!=0))
            pfnPrfCloseProfile(*phiniProfile);
#endif  /* __AVIO__ */
#ifdef  __PM__
        if(*phiniProfile!=0)
            PrfCloseProfile(*phiniProfile);
        if(puStartupParameters->hwndFrame)
            WinDestroyWindow(puStartupParameters->hwndFrame);
        WinDestroyMsgQueue(puStartupParameters->hmq);
        WinTerminate(puStartupParameters->hab);
#endif  /* __PM__ */
        if(puStartupParameters->hmodCompilerRuntime)
            DosFreeModule(puStartupParameters->hmodCompilerRuntime);
        if(puStartupParameters->hmodOCLBase)
            DosFreeModule(puStartupParameters->hmodOCLBase);
        if(puStartupParameters->hmodOCLGui)
            DosFreeModule(puStartupParameters->hmodOCLGui);
        if(puStartupParameters->hmodCodeDLL)
            DosFreeModule(puStartupParameters->hmodCodeDLL);
        if(puStartupParameters->hmodResourceDLL)
            DosFreeModule(puStartupParameters->hmodResourceDLL);
        if((puStartupParameters->iStatus & FEATURE_LOGFILE) &&
            (puStartupParameters->ahProfile[PATH_FILE_LOGFILE]))
            DosClose(puStartupParameters->ahProfile[PATH_FILE_LOGFILE]);
        DosFreeMem(puStartupParameters);
        }
    puStartupParameters=0;
}

/*--------------------------------------------------------------------------------------*\
 * UStartupWindow: processWindow()                                                      *
 *                 This is execution of the message loop of the (startup) Logo window.  *
\*--------------------------------------------------------------------------------------*/
UStartupWindow         &UStartupWindow::processWindow(void)
{
    DEBUG_L2(__THREAD_1__, "");
    QMSG    qmsg;
    HAB     hab=puStartupParameters->hab;

#ifdef  __PM__
                                        /* Message loop processing for PM version */
    WinPostMsg(puStartupParameters->hwndClient, WM_INITIALIZE, NULL, NULL);
    while(WinGetMsg(hab, &qmsg, 0, 0, 0))
        WinDispatchMsg(hab, &qmsg);
#endif  /* __PM__ */
#ifdef  __AVIO__
                                        /* For AVIO version we used nested calls instead
                                           of the message loop, but finally we will have
                                           started the working thread (which continues to
                                           startup the application) we just have to wait
                                           for that thread to end */
    UStartupWindowProc((HWND)this, WM_INITIALIZE, NULL, NULL);
    DosWaitThread(&puStartupParameters->tidThread, DCWW_WAIT);
#endif  /* __AVIO__ */
    return(*this);
}

/*--------------------------------------------------------------------------------------*
 * UStartupWindow: setupEnvironment()                                                   *
 *                 Setup environment (get the process envrionment to find the path the  *
 *                 executable has been loaded from to construct the paths we load our   *
 *                 resource and code DLLs and the profiles from. Additionally change    *
 *                 into that directory to avoid problems loading the compiler runtime   *
 *                 DLLs (the problem is that loading x:\path\dll2.dll does not work     *
 *                 even when the prerequisite DLL x:\path\dll1.dll has been loaded      *
 *                 previously, because the fully qualified x:\path\dll1.dll does not    *
 *                 match with dll1.dll which is inside dll2.dll as a prerequisite or    *
 *                 something like that (I once read a explanation, but didn't           *
 *                 understand it completely))!                                          *
\*--------------------------------------------------------------------------------------*/
ULONG               UStartupWindow::setupEnvironment(void)
{
    DEBUG_L2(__THREAD_1__, "");
    char    acMessage[128]="";
    TIB    *ptib;                       /* Thread information block */
    PIB    *ppib;                       /* Process information block */
    char   *pcTemp;
    char   *pcOption;
    APIRET  apiretRc=NO_ERROR;
    
                                        /* Construct environment (message services not
                                           ready yet) */
    sprintf(acMessage, "%s0011I: Initializing %s environment ...", 
        BLDLEVEL_PREFIX, BLDLEVEL_INFO);
#ifdef  __AVIO__
    strcat(acMessage, "\r\n");
#endif  /* __AVIO__ */
    displayMessage(allocString(acMessage), TRUE);
                                        /* Query the fully qualified path the executable got
                                           loaded from */
    apiretRc=DosGetInfoBlocks(&ptib, &ppib);
    if(apiretRc!=NO_ERROR)
        return(apiretRc);
    if((ptib==0) || (ppib==0)) 
        return(apiretRc);
    DosQueryModuleName(ppib->pib_hmte, sizeof(puStartupParameters->acPathExecutable), 
        (PCHAR)puStartupParameters->acPathExecutable);
                                        /* If no drive letter is available, e.g. using
                                           SET RUNWORKPLACE=\PMAPPS\CB2.EXE, add current
                                           drive, which is the boot drive */
    if(puStartupParameters->acPathExecutable[0]=='\\')
        {
        ULONG   ulDriveNumber;          /* Current logical drive */
        ULONG   ulLogicalDriveMap;      /* Logical drive bitmapped flag */
                                        /* Current drive */
        char    acCurrentDrive[3]="C:"; 

                                        /* Get current logical drive 1...A, 2...B, 3...C, ... */
        if(!DosQueryCurrentDisk(&ulDriveNumber, &ulLogicalDriveMap))
                                        /* On no error use current drive, otherwise assume C: */
            acCurrentDrive[0]=(CHAR)(--ulDriveNumber)+'A';
        strcpy(puStartupParameters->acPath, acCurrentDrive); 
        }
    strcat(puStartupParameters->acPath, puStartupParameters->acPathExecutable);
                                        /* Remove filename, set the current directory there, 
                                           but keep the trailing \ afterwards */
    pcTemp=strrchr(puStartupParameters->acPath, '\\');
    if(pcTemp!=0)
        {
        *pcTemp='\0';
        DosSetDefaultDisk((puStartupParameters->acPath[0] | 0x20)-'a'+1);
        DosSetCurrentDir(puStartupParameters->acPath);
        *pcTemp='\\';
        ++pcTemp;
        *pcTemp='\0';
        }
    strcpy(puStartupParameters->acPathPrefix, puStartupParameters->acPath);
    strcat(puStartupParameters->acPathPrefix, puStartupParameters->pcPrefix);
                                        /* Construct fully qualified paths */
    pcTemp=puStartupParameters->acPath;
    strcpy(puStartupParameters->acProfile[PATH_FILE_CPP_RUNTIME], pcTemp);
#ifdef  DEBUG
    strcat(puStartupParameters->acProfile[PATH_FILE_CPP_RUNTIME], COMPILER_PREFIX"OM30.DLL");
#else
    strcat(puStartupParameters->acProfile[PATH_FILE_CPP_RUNTIME], BLDLEVEL_PREFIX"OM30.DLL");
#endif  /* DEBUG */
    strcpy(puStartupParameters->acProfile[PATH_FILE_OCL_BASE], pcTemp);
#ifdef  DEBUG
    strcat(puStartupParameters->acProfile[PATH_FILE_OCL_BASE], COMPILER_PREFIX"OOB3.DLL");
#else
    strcat(puStartupParameters->acProfile[PATH_FILE_OCL_BASE], BLDLEVEL_PREFIX"OOB3.DLL");
#endif  /* DEBUG */
    strcpy(puStartupParameters->acProfile[PATH_FILE_OCL_GUI], pcTemp);
#ifdef  DEBUG
    strcat(puStartupParameters->acProfile[PATH_FILE_OCL_GUI], COMPILER_PREFIX"OOU3.DLL");
#else
    strcat(puStartupParameters->acProfile[PATH_FILE_OCL_GUI], BLDLEVEL_PREFIX"OOU3.DLL");
#endif  /* DEBUG */
                                        /* Construct fully qualified paths */
    pcTemp=puStartupParameters->acPathPrefix;
    strcpy(puStartupParameters->acProfile[PATH_FILE_CODEDLL], pcTemp);
#ifdef  __PM__
    strcat(puStartupParameters->acProfile[PATH_FILE_CODEDLL], "PM.dll");
#endif  /* __PM__ */
#ifdef  __AVIO__
    strcat(puStartupParameters->acProfile[PATH_FILE_CODEDLL], ".dll");
#endif  /* __AVIO__ */
    strcpy(puStartupParameters->acProfile[PATH_FILE_INI], pcTemp);
    strcat(puStartupParameters->acProfile[PATH_FILE_INI], ".ini");
    strcpy(puStartupParameters->acProfile[PATH_FILE_PROFILE], pcTemp);
    strcat(puStartupParameters->acProfile[PATH_FILE_PROFILE], ".cfg");
    strcpy(puStartupParameters->acProfile[PATH_FILE_DEBUG], pcTemp);
    strcat(puStartupParameters->acProfile[PATH_FILE_DEBUG], ".trp");
    strcpy(puStartupParameters->acProfile[PATH_FILE_LOGFILE], pcTemp);
    strcpy(puStartupParameters->acProfile[PATH_FILE_RESERVED1], pcTemp);
    strcpy(puStartupParameters->acProfile[PATH_FILE_RESERVED2], pcTemp);
    strcpy(puStartupParameters->acProfile[PATH_FILE_RESERVED3], pcTemp);
                                        /* Check if a NLS prefix has been saved previously.
                                           Write current selection back to profile */
    pcOption=checkCommandlineOption("language", TRUE);
    if(pcOption==0)
        {
        if(accessProfile(puStartupParameters->pcApplicationName, "NLS", 
            puStartupParameters->acNLSPrefix, sizeof(puStartupParameters->acNLSPrefix), TRUE)!=0)
            puStartupParameters->acNLSPrefix[2]='\0';
        else
            if(!strcmp(puStartupParameters->acNLSPrefix, ""))
                strcpy(puStartupParameters->acNLSPrefix, "Us");
        }
    else if(pcOption!=(char *)0xFFFFFFFF)
        strcpy(puStartupParameters->acNLSPrefix, pcOption);
    accessProfile(puStartupParameters->pcApplicationName, "NLS", 
        puStartupParameters->acNLSPrefix, sizeof(puStartupParameters->acNLSPrefix));
                                        /* Let's assume that we know know the NLS version */
    pcTemp=puStartupParameters->acPathPrefix;
    strcpy(puStartupParameters->acProfile[PATH_FILE_RESOURCEDLL], pcTemp);
    strcat(puStartupParameters->acProfile[PATH_FILE_RESOURCEDLL], puStartupParameters->acNLSPrefix);
    strcat(puStartupParameters->acProfile[PATH_FILE_RESOURCEDLL], ".dll");
    strcpy(puStartupParameters->acProfile[PATH_FILE_MESSAGE_NLS], pcTemp);
    strcat(puStartupParameters->acProfile[PATH_FILE_MESSAGE_NLS], puStartupParameters->acNLSPrefix);
    strcat(puStartupParameters->acProfile[PATH_FILE_MESSAGE_NLS], ".msg");
    strcpy(puStartupParameters->acProfile[PATH_FILE_MESSAGE_US], pcTemp);
    strcat(puStartupParameters->acProfile[PATH_FILE_MESSAGE_US], "Us.msg");
    strcpy(puStartupParameters->acProfile[PATH_FILE_HELP_NLS], pcTemp);
    strcat(puStartupParameters->acProfile[PATH_FILE_HELP_NLS], puStartupParameters->acNLSPrefix);
    strcat(puStartupParameters->acProfile[PATH_FILE_HELP_NLS], ".hlp");
    strcpy(puStartupParameters->acProfile[PATH_FILE_HELP_US], pcTemp);
    strcat(puStartupParameters->acProfile[PATH_FILE_HELP_US], "Us.hlp");
#ifdef  __PM__
                                        /* Continue with checking access to the message file */
    WinPostMsg(puStartupParameters->hwndClient, WM_STARTUP, MPFROMLONG(STARTUP_LOADMESSAGEFILE), NULL);
#endif  /* __PM__ */
#ifdef  __AVIO__
                                        /* Under AVIO use a nested call instead */
    UStartupWindowProc((HWND)this, WM_STARTUP, MPFROMLONG(STARTUP_LOADMESSAGEFILE), NULL);
#endif  /* __AVIO__ */
    return(NO_ERROR);
}

/*--------------------------------------------------------------------------------------*\
 * UStartupWindow: threadStartup()                                                      *
 *                 Create a thread that executes from the main entrypoint of the code   *
 *                 DLL loaded after having finished initialization.                     *
\*--------------------------------------------------------------------------------------*/
ULONG                   UStartupWindow::threadStartup(void)
{
    DEBUG_L2(__THREAD_1__, "");
    APIRET  apiretRc;
    
    apiretRc=DosCreateThread(&puStartupParameters->tidThread, (PFNTHREAD)UStartupThread, 
        (ULONG)this, CREATE_READY | STACK_SPARSE, 131072);
    if(apiretRc!=NO_ERROR)
        {
        DEBUG_L0S(__THREAD_1__, "Error %ld creating thread\n", apiretRc);
        displayMessage(loadMessage(NLSMSG_ERRSTARTUPTHREAD, puStartupParameters->acProfile[PATH_FILE_CODEDLL]), 
            FALSE);
        UAnchor::showWindow();
        }
    return(apiretRc);
}

/*--------------------------------------------------------------------------------------*\
 * UStartupWindow: loadMessageFile()                                                    *
 *                 Try to load the NLS message file, and if not successfull the US      *
 *                 English one too. If that doesn't work too, exit.                     *
\*--------------------------------------------------------------------------------------*/
ULONG                   UStartupWindow::loadMessageFile(void)
{
    DEBUG_L2(__THREAD_1__, "");
    char   *pcMessage=0;
    char   *pcOption=0;
    APIRET  apiretRc;

                                        /* Assuming that messaging works, tell user
                                           we're loading it */
    displayMessage(loadMessage(NLSMSG_LOADMESSAGEFILE), TRUE);
                                        /* Load the test message 0 from the NLS messagefile to
                                           check if the messagefile is available, which is
                                           expected (for a complete NLS translation) */
    pcMessage=loadMessage(NLSMSG_MESSAGEFILETEST);
    if(pcMessage==0)
        {
                                        /* That error is non-critical, as the Us messagefile
                                           will be used as a fallback */
        displayMessage(loadMessage(NLSMSG_ERRLOADMESSAGEFILE, 
            puStartupParameters->acProfile[PATH_FILE_MESSAGE_NLS]), FALSE);
        }
    else
        allocFree(pcMessage);
                                        /* Try to find the help option, because that
                                           should prevent us from automatically closing
                                           the PM version even if no errors were found */
    if((pcOption=checkCommandlineOption("?", TRUE))!=0)
        puStartupParameters->iStatus|=FEATURE_ONLINEHELP;
                                        /* Try to find the logifle option, which
                                           may either specify a file or we'll use our
                                           default */
    if((pcOption=checkCommandlineOption("Log", TRUE))!=0)
        {
        puStartupParameters->iStatus|=FEATURE_LOGFILE;
                                        /* If just /Log was specified, but not filename afterwards,
                                           just use the file in our path. Otherwise use what
                                           was specified */
        if(pcOption==(char *)0xFFFFFFFF)
            strcat(puStartupParameters->acProfile[PATH_FILE_LOGFILE], ".log");
        else
            strcpy(puStartupParameters->acProfile[PATH_FILE_LOGFILE], pcOption);
                                        /* Try to open logfile, report success */
        apiretRc=logMessage(0);
        if(apiretRc!=NO_ERROR)
            {
            char    acConvert[16];
                                        /* An error is non-critical as we still log onto
                                           the console or logo textline */
            puStartupParameters->iStatus&=(~FEATURE_LOGFILE);
            _itoa(apiretRc, acConvert, 10);
#ifdef  __PM__
            displayMessage(loadMessage(NLSMSG_ERRLOGFILE, 
                acConvert, (char *)puStartupParameters->acProfile[PATH_FILE_LOGFILE]), FALSE);
#endif  /* __PM__ */
#ifdef  __AVIO__
            displayFormatted(loadMessage(NLSMSG_ERRLOGFILE, 
                acConvert, (char *)puStartupParameters->acProfile[PATH_FILE_LOGFILE]));
#endif  /* __AVIO__ */
            }
        }
#ifdef  __PM__
                                        /* Change empty Close pushbutton to NLS text */
    pcMessage=loadMessageStrip(NLSMSG_CLOSETEXT);
    if(strlen(pcMessage))
        {
        WinSetWindowText(puStartupParameters->hwndLogoPushbutton, (PSZ)pcMessage);
        allocFree(pcMessage);
        }
    else
        WinSetWindowText(puStartupParameters->hwndLogoPushbutton, (PSZ)"Close");
                                        /* Continue startup processing from the thread we
                                           create now (this prevents the PM queue to be 
                                           hogged (especially while loading the code DLL
                                           dynamically linked to the OpenClass runtime) */
    WinPostMsg(puStartupParameters->hwndClient, WM_STARTUP, MPFROMLONG(STARTUP_THREADSTARTUP), NULL);
#endif  /* __PM__ */
#ifdef  __AVIO__
                                        /* Under AVIO use a nested call instead */
    UStartupWindowProc((HWND)this, WM_STARTUP, MPFROMLONG(STARTUP_THREADSTARTUP), NULL);
#endif  /* __AVIO__ */
    return(NO_ERROR);
}

/*--------------------------------------------------------------------------------------*\
 * UStartupWindow: loadModules()                                                        *
 *                 Load the modules from the paths constructed in setupEnvironment().   *
\*--------------------------------------------------------------------------------------*/
ULONG                   UStartupWindow::loadModules(void)
{
    DEBUG_L2(__THREAD_1__, "");
    char   *pcMessage=0;
    char    acBuffer[CCHMAXPATH];
    char    acConvert[16];
    ULONG  *pulSignature=0;
    APIRET  apiretRc;

    try {
                                        /* Display the environment we're running under */
        displayMessage(loadMessage(NLSMSG_ENVIRONMENT,
            (puStartupParameters->iStatus & FEATURE_PM_PRESENT ? "PM" : "AVIO")), TRUE);
        DosSleep(INITIALIZATION_DELAY);
                                        /* Load NLS resource DLL and check magic version */
        if(puStartupParameters->iStatus & FEATURE_APP_RESOURCEDLL)
            {
            displayMessage(loadMessage(NLSMSG_LOADRESDLL), TRUE);
            DosSleep(INITIALIZATION_DELAY);
            apiretRc=DosLoadModule((PCSZ)acBuffer, sizeof(acBuffer), 
                (PCSZ)puStartupParameters->acProfile[PATH_FILE_RESOURCEDLL], &puStartupParameters->hmodResourceDLL);
            if(apiretRc!=NO_ERROR)
                {
                                        /* If NLS resource DLL can't be loaded, try loading the
                                           Us one */
                _itoa(apiretRc, acConvert, 10);
                displayMessage(loadMessage(NLSMSG_ERRLOADRESDLL, 
                    acConvert, puStartupParameters->acProfile[PATH_FILE_RESOURCEDLL], acBuffer), FALSE);
                strcpy(puStartupParameters->acProfile[PATH_FILE_RESOURCEDLL], puStartupParameters->acPathPrefix);
                strcat(puStartupParameters->acProfile[PATH_FILE_RESOURCEDLL], "Us");
                strcat(puStartupParameters->acProfile[PATH_FILE_RESOURCEDLL], ".dll");
                apiretRc=DosLoadModule((PCSZ)acBuffer, sizeof(acBuffer), 
                    (PCSZ)puStartupParameters->acProfile[PATH_FILE_RESOURCEDLL], &puStartupParameters->hmodResourceDLL);
                                            /* If Us resource DLL doesn't work it is a fatal error */
                if(apiretRc!=NO_ERROR)
                    throw((ULONG)apiretRc);
                }
            apiretRc=DosQueryProcAddr(puStartupParameters->hmodResourceDLL, DLLENTRYPOINT_VERSION, NULL, (PFN *)(&pulSignature));
            if((apiretRc!=NO_ERROR) || (*pulSignature!=puStartupParameters->iSignature))
                {
                if(apiretRc==NO_ERROR)
                    apiretRc=ERROR_INVALID_FUNCTION;
                displayMessage(loadMessage(NLSMSG_ERRVERSIONRESDLL, 
                    puStartupParameters->acProfile[PATH_FILE_RESOURCEDLL]), FALSE);
                throw((ULONG)apiretRc);
                }
            }
                                        /* Load VisualAge C++ compiler runtime and OCL dynamically
                                           linked libraries even if Code DLL is linked against them
                                           anyway. The reason is that all DLLs are expected to reside
                                           in the directory we're installed into, but we'll be not 
                                           necessarily from that directory. Even if we would load our
                                           Code DLL from the fully qualified path, that wouild not
                                           find the directory also for the compiler DLLs causing load
                                           errors. We thus explicitely load the compiler DLLs before
                                           the code DLL (of course have to free them on termination
                                           to decrement the usage count) */
        if(puStartupParameters->iStatus & FEATURE_COMPILER_RUNTIMEDLL)
            {
            displayMessage(loadMessage(NLSMSG_LOADCOMPILERRUNTIME, 
                strrchr(puStartupParameters->acProfile[PATH_FILE_CPP_RUNTIME], '\\')+1), TRUE);
            DosSleep(INITIALIZATION_DELAY>>1);
            apiretRc=DosLoadModule((PCSZ)acBuffer, sizeof(acBuffer), 
                (PCSZ)puStartupParameters->acProfile[PATH_FILE_CPP_RUNTIME], &puStartupParameters->hmodCompilerRuntime);
            if(apiretRc!=NO_ERROR)
                {
                _itoa(apiretRc, acConvert, 10);
                displayMessage(loadMessage(NLSMSG_ERRLOADCODEDLL, 
                    acConvert, puStartupParameters->acProfile[PATH_FILE_CPP_RUNTIME], acBuffer), FALSE);
                throw((ULONG)apiretRc);
                }
            displayMessage(loadMessage(NLSMSG_LOADCOMPILERRUNTIME, 
                strrchr(puStartupParameters->acProfile[PATH_FILE_OCL_BASE], '\\')+1), TRUE);
            DosSleep(INITIALIZATION_DELAY>>1);
            apiretRc=DosLoadModule((PCSZ)acBuffer, sizeof(acBuffer), 
                (PCSZ)puStartupParameters->acProfile[PATH_FILE_OCL_BASE], &puStartupParameters->hmodOCLGui);
            if(apiretRc!=NO_ERROR)
                {
                _itoa(apiretRc, acConvert, 10);
                displayMessage(loadMessage(NLSMSG_ERRLOADCODEDLL, 
                    acConvert, puStartupParameters->acProfile[PATH_FILE_OCL_BASE], acBuffer), FALSE);
                throw((ULONG)apiretRc);
                }
            }
        if(puStartupParameters->iStatus & FEATURE_COMPILER_CLASSLIBDLL)
            {
            displayMessage(loadMessage(NLSMSG_LOADCOMPILERRUNTIME, 
                strrchr(puStartupParameters->acProfile[PATH_FILE_OCL_GUI], '\\')+1), TRUE);
            DosSleep(INITIALIZATION_DELAY>>1);
            apiretRc=DosLoadModule((PCSZ)acBuffer, sizeof(acBuffer), 
                (PCSZ)puStartupParameters->acProfile[PATH_FILE_OCL_GUI], &puStartupParameters->hmodOCLGui);
            if(apiretRc!=NO_ERROR)
                {
                _itoa(apiretRc, acConvert, 10);
                displayMessage(loadMessage(NLSMSG_ERRLOADCODEDLL, 
                    acConvert, puStartupParameters->acProfile[PATH_FILE_OCL_GUI], acBuffer), FALSE);
                throw((ULONG)apiretRc);
                }
            }
                                        /* Load code DLL and check magic version */
        displayMessage(loadMessage(NLSMSG_LOADCODEDLL), TRUE);
        DosSleep(INITIALIZATION_DELAY);
        apiretRc=DosLoadModule((PCSZ)acBuffer, sizeof(acBuffer), 
            (PCSZ)puStartupParameters->acProfile[PATH_FILE_CODEDLL], &puStartupParameters->hmodCodeDLL);
        if(apiretRc!=NO_ERROR)
            {
            _itoa(apiretRc, acConvert, 10);
            displayMessage(loadMessage(NLSMSG_ERRLOADCODEDLL, 
                acConvert, puStartupParameters->acProfile[PATH_FILE_CODEDLL], acBuffer), FALSE);
            throw((ULONG)apiretRc);
            }
        apiretRc=DosQueryProcAddr(puStartupParameters->hmodCodeDLL, DLLENTRYPOINT_VERSION, NULL, (PFN *)(&pulSignature));
        if((apiretRc!=NO_ERROR) || (*pulSignature!=puStartupParameters->iSignature))
            {
            if(apiretRc==NO_ERROR)
                apiretRc=ERROR_INVALID_FUNCTION;
            displayMessage(loadMessage(NLSMSG_ERRVERSIONCODEDLL, 
                puStartupParameters->acProfile[PATH_FILE_CODEDLL]), FALSE);
            throw(apiretRc);
            }
                                        /* Load code DLL and check code entrypoint */
        apiretRc=DosQueryProcAddr(puStartupParameters->hmodCodeDLL, DLLENTRYPOINT_CODE, NULL, (PFN *)(&puStartupParameters->pfnDLLCode));
        if((apiretRc!=NO_ERROR) || (puStartupParameters->pfnDLLCode==0))
            {
            _itoa(apiretRc, acConvert, 10);
            displayMessage(loadMessage(NLSMSG_ERRENTRYPOINTCODEDLL, 
                acConvert, puStartupParameters->acProfile[PATH_FILE_CODEDLL]), FALSE);
            throw(apiretRc);
            }
        }
    catch (ULONG ulError) 
        {
        return(ulError);
        }   
    return(NO_ERROR);
}

/*--------------------------------------------------------------------------------------*\
 * UStartupWindow: showWindow()                                                         *
 *                 Show or hide the UStartupWindow Logo window.                         *
\*--------------------------------------------------------------------------------------*/
ULONG                   UStartupWindow::showWindow(ULONG ulStatus)
{
    DEBUG_L2(__THREAD_1__, "");
    SWP     swpLogo;

#ifdef  __PM__
    memset(&swpLogo, 0, sizeof(swpLogo));
    swpLogo.hwnd=puStartupParameters->hwndFrame;
    swpLogo.hwndInsertBehind=HWND_TOP;
    swpLogo.fl=(ulStatus & LOGO_SHOWLOGOWINDOW ? (SWP_ZORDER|SWP_SHOW) : (SWP_HIDE));
    if(ulStatus & LOGO_SHOWCLOSEBUTTON)
        {
        char   *pcCloseText=loadMessageStrip(NLSMSG_CLOSETEXT);
    
        if(ulStatus & LOGO_CLOSEWITHERROR)
            {
            displayMessage(loadMessage(NLSMSG_CLOSEMESSAGEERROR, pcCloseText), TRUE);
            DosSleep(INITIALIZATION_DELAY);
            }
        displayMessage(loadMessage(NLSMSG_CLOSEMESSAGENORMAL, pcCloseText), TRUE);
        if(strlen(pcCloseText))
            {
            WinSetWindowText(puStartupParameters->hwndLogoPushbutton, (PSZ)pcCloseText);
            allocFree(pcCloseText);
            }
        else
            WinSetWindowText(puStartupParameters->hwndLogoPushbutton, (PSZ)"Close");
                                        /* Show window and put focus on Close pushbutton 
                                           unless we still show a message box */
        WinShowWindow(puStartupParameters->hwndLogoPushbutton, TRUE);
                                        /* Hide status text and replace it with listbox */
        WinShowWindow(puStartupParameters->hwndLogoText, FALSE);
        WinShowWindow(puStartupParameters->hwndLogoListbox, TRUE);
        puStartupParameters->iStatus|=FEATURE_SHOW_CLOSEBUTTON;
        if(!(puStartupParameters->iStatus & FEATURE_SHOW_MESSAGEBOX))
            {
            WinFocusChange(HWND_DESKTOP, puStartupParameters->hwndFrame, 0);
            WinSetFocus(HWND_DESKTOP, puStartupParameters->hwndLogoPushbutton);
            }
        }
    WinSetMultWindowPos(puStartupParameters->hab, &swpLogo, 1);
#endif  /* __PM__ */ 
   return(NO_ERROR);
}

/*--------------------------------------------------------------------------------------*\
 * UStartupWindow: window procedure                                                     *
 *                 Due to the calling convention, the window procedure can't be a C++   *
 *                 class member, but must be a C function, so we define it as friend to *
 *                 get access.                                                          *
\*--------------------------------------------------------------------------------------*/
MRESULT EXPENTRY        UStartupWindowProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
                                        /* Our Logo object restored from window words */
UStartupWindow     *puStartupWindow=0;
UStartupParameters *puStartupParameters=0;

#ifdef  __PM__
                                        /* Access our Logo object saved into the window words
                                           during WM_INITIALIZE processing */
    puStartupWindow=(UStartupWindow *)WinQueryWindowULong(hwnd, QWL_USER);
    if(puStartupWindow==0)
        return((MRESULT)WinDefWindowProc(hwnd, msg, mp1, mp2));
#endif  /* __PM__ */
#ifdef  __AVIO__
                                        /* Under AVIO we have "reused" the window handle
                                           to transport things as we don't have window
                                           words without a window */
    puStartupWindow=(UStartupWindow *)hwnd;
#endif  /* __AVIO__ */
    puStartupParameters=puStartupWindow->puStartupParameters;
    switch(msg)
    {
    case WM_PAINT:
        {
#ifdef  __PM__
        HPS             hpsLogo=0;
        RECTL           rectlLogo;
        SWP             swpLogo;

        WinQueryWindowPos(hwnd, &swpLogo);
        hpsLogo=WinBeginPaint(hwnd, NULLHANDLE, &rectlLogo);
        rectlLogo.xLeft=rectlLogo.yBottom=0;
        rectlLogo.xRight=LOGO_CX;
        rectlLogo.yTop=swpLogo.cy;
        WinDrawBorder(hpsLogo, &rectlLogo, LOGO_BLACKBORDER+1, LOGO_BLACKBORDER+1, CLR_BLACK, CLR_BLACK, DB_AREAATTRS); 
        rectlLogo.xLeft=rectlLogo.yBottom=LOGO_BLACKBORDER;
        rectlLogo.xRight=LOGO_CX-LOGO_BLACKBORDER;
        rectlLogo.yTop=swpLogo.cy-LOGO_BLACKBORDER;
        WinDrawBorder(hpsLogo, &rectlLogo, LOGO_3DBORDER, LOGO_3DBORDER, 0, 0, DB_RAISED); 
        WinEndPaint(hpsLogo);
#endif  /* __PM__ */
        }
        break;        

    case WM_COMMAND:
        {
        USHORT  usCommand=SHORT1FROMMP(mp1);
        
#ifdef  __PM__
        if(usCommand==ID_LOGOPUSHBUTTON)
            WinPostMsg(hwnd, WM_QUIT, NULL, NULL);
#endif  /* __PM__ */
        }
        break;

/*                                                                                      *\
 * Syntax: WM_INITIALIZE, NULL, NULL                                                    *
\*                                                                                      */
    case WM_INITIALIZE:
/*                                                                                      *\
 * This message is posted before entering the message loop to allow us to do the start- *
 * up processing of the Logo window, so it's one of the first messages being processed. *
\*                                                                                      */
        {
#ifdef  __PM__
        LONG    lCXScreen=WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN);
        LONG    lCYScreen=WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN);
        POINTL  pointlClose;
        POINTL  pointlStatus;
        SWP     swpLogo;

                                        /* Get the height of 3 lines 8.Helv font */
        pointlStatus.x=LOGO_TEXT_CX;
        pointlStatus.y=LOGO_TEXT_CY;
        WinMapDlgPoints(hwnd, &pointlStatus, sizeof(pointlStatus)/sizeof(POINTL), TRUE);
                                        /* Create the infrastructure (that is the controls)
                                           of the Logo window */
        memset(&swpLogo, 0, sizeof(swpLogo));
        swpLogo.fl=SWP_ACTIVATE | SWP_SHOW | SWP_ZORDER | SWP_MOVE | SWP_SIZE;
        swpLogo.hwnd=WinQueryWindow(hwnd, QW_PARENT);
        swpLogo.cx=LOGO_CX;
        swpLogo.cy=LOGO_CY+pointlStatus.y;
        swpLogo.x=(lCXScreen-swpLogo.cx)>>1;
        swpLogo.y=(lCYScreen-swpLogo.cy)>>1;
        swpLogo.hwndInsertBehind=HWND_TOP;
        puStartupParameters->hwndLogoText=WinCreateWindow(hwnd, WC_STATIC, "", 
            DT_WORDBREAK | DT_LEFT | SS_TEXT | WS_VISIBLE, 
            LOGO_BORDER, LOGO_BORDER, LOGO_TEXT_CX, pointlStatus.y, 
            hwnd, HWND_TOP, ID_LOGOTEXT, 0, 0);
        WinSetPresParam(puStartupParameters->hwndLogoText, PP_FONTNAMESIZE,
            (sizeof("8.Helv")-1), "8.Helv");
        puStartupParameters->hwndLogoListbox=WinCreateWindow(hwnd, WC_LISTBOX, "", 
            LS_HORZSCROLL | LS_NOADJUSTPOS, 
            LOGO_BORDER, LOGO_BORDER, LOGO_TEXT_CX, pointlStatus.y, 
            hwnd, HWND_TOP, ID_LOGOLISTBOX, 0, 0);
        WinSetPresParam(puStartupParameters->hwndLogoListbox, PP_FONTNAMESIZE,
            (sizeof("8.Helv")-1), "8.Helv");
        puStartupParameters->hwndLogoBitmap=WinCreateWindow(hwnd, WC_STATIC, "#2", 
            SS_BITMAP | WS_CLIPSIBLINGS | WS_VISIBLE,
            LOGO_BORDER, LOGO_BORDER+pointlStatus.y, LOGO_GRAPHICS_CX, LOGO_GRAPHICS_CY,
            hwnd, HWND_TOP, ID_LOGOBITMAP, 0, 0);
        pointlClose.x=LOGO_BUTTON_CX;
        pointlClose.y=LOGO_BUTTON_CY;
        WinMapDlgPoints(hwnd, &pointlClose, sizeof(pointlClose)/sizeof(POINTL), TRUE);
        puStartupParameters->hwndLogoPushbutton=WinCreateWindow(hwnd, WC_BUTTON, "", 
            BS_DEFAULT,
            LOGO_CX-pointlClose.x-10, LOGO_CY+pointlStatus.y-pointlClose.y-10, pointlClose.x, pointlClose.y,
            hwnd, HWND_TOP, ID_LOGOPUSHBUTTON, 0, 0);
        WinSetMultWindowPos(WinQueryAnchorBlock(hwnd), &swpLogo, 1);        
                                        /* Now that we have created the infrastructure, and the 
                                           controls will have (hopefully) repainted, continue
                                           startup processing */
        WinPostMsg(hwnd, WM_STARTUP, MPFROMLONG(STARTUP_ENVIRONMENT), NULL);
#endif  /* __PM__ */
#ifdef  __AVIO__
                                        /* Under AVIO used nested calls instead */
        UStartupWindowProc((HWND)puStartupWindow, WM_STARTUP, MPFROMLONG(STARTUP_ENVIRONMENT), NULL);
#endif  /* __AVIO__ */
        }
        break;
     
/*                                                                                      *\
 * Syntax: WM_STARTUP, ULONG ulFunction, NULL                                           *
\*                                                                                      */
    case WM_STARTUP:
/*                                                                                      *\
 * This message is posted during WM_INITIALIZE to allow us to continue startup          *
 * processing after the Logo window has been repainted (and made visibly therefore).    *
\*                                                                                      */
        switch(LONGFROMMP(mp1))
        {
        case STARTUP_ENVIRONMENT:
            puStartupWindow->setupEnvironment();       
            break;

        case STARTUP_LOADMESSAGEFILE:
            puStartupWindow->loadMessageFile();
            break;

        case STARTUP_THREADSTARTUP:
            puStartupWindow->threadStartup();       
            break;

        }
        break;

/*                                                                                      *\
 * Syntax: WM_STARTUPMESSAGE, char *pcMessage, ULONG ulKeepMessage                      *
\*                                                                                      */
    case WM_STARTUPMESSAGE:
/*                                                                                      *\
 * This message is posted to update the message text in the logo window to indicate     *
 * progress.                                                                            *
\*                                                                                      */
#ifdef  __PM__
        {
        char   *pcMessage=puStartupWindow->stripTrailing((CHAR *)mp1);
        ULONG   ulItemCount;

        puStartupWindow->logMessage(pcMessage);
        WinSetWindowText(puStartupParameters->hwndLogoText, (PSZ)mp1);
        WinSendMsg(puStartupParameters->hwndLogoListbox, LM_INSERTITEM,
            MPFROMSHORT(LIT_END), (MPARAM)pcMessage);
        ulItemCount=(ULONG)WinSendMsg(puStartupParameters->hwndLogoListbox, LM_QUERYITEMCOUNT,
            NULL, NULL);
        WinSendMsg(puStartupParameters->hwndLogoListbox, LM_SETTOPINDEX,
            MPFROMSHORT(ulItemCount), NULL);
        }
#endif  /* __PM__ */
        if(LONGFROMMP(mp1)==FALSE)
            puStartupWindow->allocFree((char *)mp1);
        break;

/*                                                                                      *\
 * Syntax: WM_MESSAGEBOX, char *pcMessage, NULL                                         *
\*                                                                                      */
    case WM_MESSAGEBOX:
/*                                                                                      *\
 * This message is posted to the Logo window thread to display a message box modal to   *
 * the Logo window. Before displaying the message box we'll treat it as a normal logo   *
 * window message .                                                                     *
\*                                                                                      */
#ifdef  __PM__
        WinSendMsg(hwnd, WM_STARTUPMESSAGE, mp1, MPFROMLONG(TRUE));
        puStartupWindow->displayMessage((char *)PVOIDFROMMP(mp1));
#endif  /* __PM__ */
        break;

/*                                                                                      *\
 * Syntax: WM_SHOWLOGOWINDOW, ULONG ulStatus, NULL                                      *
\*                                                                                      */
    case WM_SHOWLOGOWINDOW:
/*                                                                                      *\
 * This message is posted to the Logo window thread to show or hide the UStartupWindow  *
 * Logo window (and/or Close pushbutton).                                               *
\*                                                                                      */
#ifdef  __PM__
        puStartupWindow->showWindow((ULONG)mp1);
#endif  /* __PM__ */
        break;

    case WM_USER+1000:
        printf("0x%08X, 0x%08X\n", (int)mp1, (int)mp2);
        break;

    default:                            /* Default window procedure must be called */
#ifdef  __PM__
        return((MRESULT)WinDefWindowProc(hwnd, msg, mp1, mp2));
#endif  /* __PM__ */
#ifdef  __AVIO__
        return((MRESULT)FALSE);
#endif  /* __AVIO__ */
    }
    return((MRESULT)FALSE);             /* We have handled the message */
}

/****************************************************************************************\
 * UStartupWindow: UStartupThread                                                       *
 *                 Due to the calling convention, the thread started can't be a C++     *
 *                 class member, but must be a C function, so we define it as friend to *
 *                 get access.                                                          *
\****************************************************************************************/
/*--------------------------------------------------------------------------------------*\
 * UStartup thread procedure                                                            *
\*--------------------------------------------------------------------------------------*/
ULONG _System           UStartupThread(UStartupWindow *puStartupWindow)
{
    UAnchor    *puAnchor=puStartupWindow->puAnchor;
    ULONG       ulRc=NO_ERROR;
    DEBUG_L2(__THREAD_2__, "Thread started");

                                        /* Preempt ourselves */
    DosSleep(INITIALIZATION_DELAY);
    puStartupWindow->displayMessage(puStartupWindow->loadMessage(NLSMSG_COPYRIGHT1), TRUE);
    puStartupWindow->displayMessage(puStartupWindow->loadMessage(NLSMSG_COPYRIGHT2), TRUE);
    puStartupWindow->displayMessage(puStartupWindow->loadMessage(NLSMSG_COPYRIGHT3), TRUE);
    DosSleep(INITIALIZATION_DELAY);
    if(puStartupWindow->puStartupParameters->iStatus & FEATURE_SHOW_HELP)
        {
#ifdef  __AVIO__
        puStartupWindow->displayMessage(puStartupWindow->allocString("\r\n"), TRUE);
        puStartupWindow->displayMessage(puStartupWindow->loadMessage(NLSMSG_SYNTAX_AVIO), TRUE);
        DosSleep(INITIALIZATION_DELAY);
#endif  /* __AVIO__ */
#ifdef  __PM__
        puStartupWindow->displayMessage(puStartupWindow->loadMessage(NLSMSG_SYNTAX_PM), FALSE);
        DosSleep(INITIALIZATION_DELAY);
        puStartupWindow->UAnchor::showWindow(TRUE, TRUE);
#endif  /* __PM__ */
        }
    else
        {
                                        /* Load the DLLs (which may take a while, but
                                           furtunately we're not running in a PM thread) */
        if(puStartupWindow->loadModules()==NO_ERROR)
            {
                                        /* Invoke the code DLL entrypoint to startup application */
            try
                {
                ulRc=puStartupWindow->puStartupParameters->pfnDLLCode(puStartupWindow->puStartupParameters);
                }
            catch(char *pcErrorMessage)
                {
                puStartupWindow->displayMessage(pcErrorMessage, FALSE);
                }
            catch(...)
                {
                printf("%s%04d>%s unknown exception\n",__THREAD_2__, __LINE__, __FUNCTION__);
                }
            puStartupWindow->UAnchor::showWindow(TRUE, TRUE, ulRc);
            }
        else
            {
            puStartupWindow->UAnchor::showWindow(TRUE, TRUE, TRUE);
            }
        }
                                        /* Set return code */
    puAnchor->ulReturnCode=ulRc;
                                        /* End thread */
    DEBUG_L1(__THREAD_2__, "ended");
    DosExit(EXIT_THREAD, 0);    
    return(ulRc);
}

