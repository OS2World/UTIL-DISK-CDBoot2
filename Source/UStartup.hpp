#ifndef _USTARTUP_HPP_
#define _USTARTUP_HPP_

/***********************************************************************\
 *                              UStartup                               *
 *              Copyright (C) by Stangl Roman, 1999, 2002              *
 * This Code may be freely distributed, provided the Copyright isn't   *
 * removed, under the conditions indicated in the documentation.       *
 *                                                                     *
 * UStartup.hpp General purpose application entry code that displays a *
 *              Logo window, loads resources and starts the real part  *
 *              of the application from those resources.               *
 *                                                                     *
\***********************************************************************/

                                        /* Thanks to Holger Veit for documenting this */ 
#ifndef DOSSMPMPRESENT
                                        /* Return if we're running under PM */
typedef APIRET16    (APIENTRY16 DOSSMPMPRESENT)(SHORT *psFlag);
#define ORD_DOSSMPMPRESENT              712
#endif  /* DOSSMPMPRESENT */

                                        /* 32-Bit Prf* API function prototypes as we 
                                           load them dynamically (only if we're running
                                           under PM) */
typedef HINI        (APIENTRY PRFOPENPROFILE)(HAB hab, PCSZ pszFileName);
typedef BOOL        (APIENTRY PRFWRITEPROFILEDATA)(HINI hini, PCSZ pszApp, PCSZ pszKey, PVOID pData, ULONG cchDataLen);
typedef BOOL        (APIENTRY PRFQUERYPROFILEDATA)(HINI hini, PCSZ pszApp, PCSZ pszKey, PVOID pBuffer, PULONG pulBuffLen);
typedef BOOL        (APIENTRY PRFCLOSEPROFILE)(HINI hini);

#ifndef __PM__
#ifndef __AVIO__
#error(Either "__PM__" or "__AVIO__" must be defined)
#endif  /* __AVIO__ */
#endif  /* __PM__ */

struct  UStartupParameters;
class   UAnchor;
class   UStartupWindow;

                                        /* Check IBM C/C++ Compiler version */
#ifndef DEBUG
#define COMPILER_PREFIX         "OS2"
#else
#if     __IBMCPP__==300
#define COMPILER_PREFIX         "CPP"
#else
#error Without modifications only IBM VisualAge C++ 3.0 compiler is supported!
#endif  /* __IBMCPP__ */
#endif  /* DEBUG */


#if     __cplusplus
extern  "C"
{
    MRESULT EXPENTRY    UStartupWindowProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
    ULONG _System       UStartupThread(UStartupWindow *puStartupWindow);
}
#endif  /* __cplusplus */

                                        /* UAnchor flags for ulDebug */
#define DEBUG_LEVEL_0                   0                            
#define DEBUG_LEVEL_1                   1                            
#define DEBUG_LEVEL_2                   2                            
#define DEBUG_LEVEL_3                   3                            

                                        /* Debug definitions */
#ifdef  NODEBUG
#define     DEBUG_L3(pcThread, pcDebugText)   
#define     DEBUG_L3S(pcThread, pcFormatString, statement)
#define     DEBUG_L2(pcThread, pcDebugText)   
#define     DEBUG_L3S(pcThread, pcFormatString, statement)
#define     DEBUG_L1(pcThread, pcDebugText)   
#define     DEBUG_L3S(pcThread, pcFormatString, statement)
#define     DEBUG_L0(pcThread, pcDebugText)   
#define     DEBUG_L3S(pcThread, pcFormatString, statement)
#else
                                        /* To debug we print debugging information via stdout into a
                                           PM window (using the printf() function from the PMPRINTF
                                           package) */
#define     DEBUG_L3(pcThread, pcDebugText) { \
                                            if(puAnchor->ulDebug>=DEBUG_LEVEL_3) \
                                                printf("%s:%04d>%s %s\n", pcThread, __LINE__, __FUNCTION__, ""##pcDebugText); \
                                            }
#define     DEBUG_L3S(pcThread, pcFormatString, statement) { \
                                            if(puAnchor->ulDebug>=DEBUG_LEVEL_3) \
                                                printf("%s:%04d>%s "##pcFormatString, pcThread, __LINE__, __FUNCTION__, pStatement); \
                                            }
#define     DEBUG_L2(pcThread, pcDebugText) { \
                                            if(puAnchor->ulDebug>=DEBUG_LEVEL_2) \
                                                printf("%s:%04d>%s %s\n", pcThread, __LINE__, __FUNCTION__, ""##pcDebugText); \
                                            }
#define     DEBUG_L2S(pcThread, pcFormatString, statement) { \
                                            if(puAnchor->ulDebug>=DEBUG_LEVEL_2) \
                                                printf("%s:%04d>%s "##pcFormatString, pcThread, __LINE__, __FUNCTION__, statement); \
                                            }
#define     DEBUG_L1(pcThread, pcDebugText) { \
                                            if(puAnchor->ulDebug>=DEBUG_LEVEL_1) \
                                                printf("%s:%04d>%s %s\n", pcThread, __LINE__, __FUNCTION__, ""##pcDebugText); \
                                            }
#define     DEBUG_L1S(pcThread, pcFormatString, statement) { \
                                            if(puAnchor->ulDebug>=DEBUG_LEVEL_1) \
                                                printf("%s:%04d>%s "##pcFormatString, pcThread, __LINE__, __FUNCTION__, statement); \
                                            }
#define     DEBUG_L0(pcThread, pcDebugText) { \
                                                printf("%s:%04d>%s %s\n", pcThread, __LINE__, __FUNCTION__, ""##pcDebugText); \
                                            }
#define     DEBUG_L0S(pcThread, pcFormatString, statement) { \
                                                printf("%s:%04d>%s "##pcFormatString, pcThread, __LINE__, __FUNCTION__, statement); \
                                            }
#endif  /* NODEBUG */

                                        /* Memory management checking */
#ifdef  DEBUG
#ifdef  __DEBUG_ALLOC__
#define     CHECK_MEMORY(pcThread) { \
                                   DEBUG_L0(pcThread, "Checking memory management..."); \
                                   _dump_allocated(64); \
                                   }
#else
#define     CHECK_MEMORY(pcThread)
#endif  /* __DEBUG_ALLOC__ */
#endif  /* DEBUG */

                                        /* Messages loaded from NLS message file */
#define NLSMSG_MESSAGEFILETEST          (0)
#define NLSMSG_ERRLOGFILE               (4) 
#define NLSMSG_CLOSEMESSAGEERROR        (5) 
#define NLSMSG_CLOSEMESSAGENORMAL       (6) 
#define NLSMSG_COPYRIGHT1               (7) 
#define NLSMSG_COPYRIGHT2               (8) 
#define NLSMSG_COPYRIGHT3               (9) 
#define NLSMSG_CLOSETEXT                (10)
#define NLSMSG_INITIALIZING             (11)
#define NLSMSG_LOADMESSAGEFILE          (12)
#define NLSMSG_ENVIRONMENT              (13)
#define NLSMSG_LOADMODULES              (14)
#define NLSMSG_LOADRESDLL               (15)
#define NLSMSG_ERRLOADMESSAGEFILE       (16)
#define NLSMSG_ERRLOADRESDLL            (17)
#define NLSMSG_ERRVERSIONRESDLL         (18)
#define NLSMSG_LOADCOMPILERRUNTIME      (19)
#define NLSMSG_ERRLOADCOMPILERRUNTIME   (20)
#define NLSMSG_LOADCODEDLL              (21)
#define NLSMSG_ERRLOADCODEDLL           (22)
#define NLSMSG_ERRVERSIONCODEDLL        (23)
#define NLSMSG_ERRENTRYPOINTCODEDLL     (24)
#define NLSMSG_ERRSTARTUPTHREAD         (25)

#define MSG(n)                          BLDLEVEL_PREFIX##n

                                        /* Logo window controls */       
#define ID_LOGOBITMAP                   2
#define ID_LOGOTEXT                     3
#define ID_LOGOLISTBOX                  4
#define ID_LOGOPUSHBUTTON               5

                                        /* The first entrypoint of all DLLs must be the version */
#define DLLENTRYPOINT_VERSION           1
                                        /* The second entrypoint of a code DLL must be the main one */
#define DLLENTRYPOINT_CODE              2
                                        /* The second entrypoint of a code DLL must have the following
                                           signature */ 
typedef ULONG           (_System PFNDLLCODE)(UStartupParameters *puStartupParameters);

                                        /* Define OS/2 major and minor versions */
#define OS2_MAJOR                       0x00000014
#define OS2_MINOR_200                   0x00000000
#define OS2_MINOR_210                   0x0000000A
#define OS2_MINOR_211                   0x0000000B
#define OS2_MINOR_300                   0x0000001E
#define OS2_MINOR_400                   0x00000028

                                        /* Logo window definitions */
#define     LOGO_GRAPHICS_CX            320
#define     LOGO_GRAPHICS_CY            200
#define     LOGO_TEXT_CX                LOGO_GRAPHICS_CX
#define     LOGO_TEXT_CY                (8*3)
#define     LOGO_BLACKBORDER            1
#define     LOGO_3DBORDER               3
#define     LOGO_BORDER                 (LOGO_BLACKBORDER+LOGO_3DBORDER)
#define     LOGO_CX                     ((LOGO_BORDER<<1)+LOGO_GRAPHICS_CX)
#define     LOGO_CY                     ((LOGO_BORDER<<1)+LOGO_GRAPHICS_CY)
#define     LOGO_BUTTON_CX              60
#define     LOGO_BUTTON_CY              20

#define     INITIALIZATION_DELAY        1000

                                        /* Country to NLS prefix translation table */
struct  UCountryTranslate
{
    ULONG   ulCountry;
    char    acCountry[3];
};

struct  UStartupParameters
{
                                        /* Number of profiles */
#define NUM_PROFILES                    16
                                        /* Index of files in acProfile and ahProfile */
                                        /* Code DLL to load */
#define PATH_FILE_CODEDLL               0
                                        /* VisualAge compiler and OCL runtime */
#define PATH_FILE_CPP_RUNTIME           1
#define PATH_FILE_OCL_BASE              2
#define PATH_FILE_OCL_GUI               3
                                        /* Resource DLL */
#define PATH_FILE_RESOURCEDLL           4
                                        /* English and NLS message file */
#define PATH_FILE_MESSAGE_US            5
#define PATH_FILE_MESSAGE_NLS           6
                                        /* English and NLS online help */
#define PATH_FILE_HELP_US               7 
#define PATH_FILE_HELP_NLS              8 
                                        /* INI file */
#define PATH_FILE_INI                   9 
                                        /* Configuration file */
#define PATH_FILE_PROFILE               10
                                        /* Exception log file */
#define PATH_FILE_DEBUG                 11
                                        /* Message Log file */
#define PATH_FILE_LOGFILE               12
#define PATH_FILE_RESERVED1             13
#define PATH_FILE_RESERVED2             14
#define PATH_FILE_RESERVED3             15

                                        /* Number of commandline arguments */
    int                 argc;  
                                        /* Pointer to array of commandline arguments */ 
    char              **argv;
                                        /* Status flags */
    int                 iStatus;
                                        /* Version check */
    int                 iSignature;
                                        /* Communication area for emergency cases */
    char                acMessage[1024];
                                        /* Application name (e.g. in Window List,...) */
    char               *pcApplicationName;
                                        /* Prefix (3 characters to conform to the DLL 8.3 limit) */
    char               *pcPrefix;
                                        /* Fully qualified path executable got loaded from */
    char                acPathExecutable[CCHMAXPATH];
                                        /* Fully qualified path of directory application started from */
    char                acPath[CCHMAXPATH];
                                        /* Fully qualified path of directory and prefix to construct
                                           other filesnames from */
    char                acPathPrefix[CCHMAXPATH];
                                        /* 2 character prefix of NLS filenames */
    char                acNLSPrefix[3];
                                        /* Array of profile filenames */
    char                acProfile[NUM_PROFILES][CCHMAXPATH];
                                        /* Array of profile handles */
    LHANDLE             ahProfile[NUM_PROFILES];
                                        /* Anchor block */
    HAB                 hab;
                                        /* Message queue */
    HMQ                 hmq;
                                        /* Logo window frame handle */
    HWND                hwndFrame;
                                        /* Logo window client handle */
    HWND                hwndClient;
                                        /* Logo window bitmap (SS_BITMAP) handle */
    HWND                hwndLogoBitmap;
                                        /* Logo window static text (SS_TEXT) handle */
    HWND                hwndLogoText;
                                        /* Logo window listbox handle */
    HWND                hwndLogoListbox;
                                        /* Logo window close button handle */
    HWND                hwndLogoPushbutton;
                                        /* OS/2 Version */
    ULONG               ulVersionMajor;
    ULONG               ulVersionMinor;
                                        /* Compiler runtime DLLs module handles */
    HMODULE             hmodCompilerRuntime;
    HMODULE             hmodOCLBase;
    HMODULE             hmodOCLGui;
                                        /* Code DLL module handle */
    HMODULE             hmodCodeDLL;
                                        /* Resource DLL module handle */
    HMODULE             hmodResourceDLL;
                                        /* Entrypoint to thread code loaded from code DLL */
    PFNDLLCODE         *pfnDLLCode;
                                        /* Thread */
    TID                 tidThread;
                                        /* Reference */
    ULONG               ulDebug;
    UAnchor            *puAnchor;
};


/****************************************************************************************\
 * Class: UAnchor                                                                       *
\****************************************************************************************/

                                        /* Message box ID */
#define ID_DISPLAYMESSAGE               1
                                        /* Message box titlebar size (padded with spaces
                                           to show dialog wider) */
#define TITLEBARLENGTH                  255

class   UAnchor
{
public:
                                        /* Public constructor that requires parameter structures
                                           allready allocated */
                        UAnchor(UStartupParameters *puStartupParameters);
                                        /* Destructor */
                        ~UAnchor(void);
                                        /* Set user arguments argc and argv[], so that e.g.
                                           checkCommandlineOption() will also use those */
    ULONG               setUserCommandline(char *pcCIDFile);
                                        /* Check for commandline option */
    char               *checkCommandlineOption(char *pcOption, BOOL bCompositeOption=FALSE, BOOL bCaseSensitive=FALSE);
                                        /* Access the INI file */
    int                 accessProfile(char *pcApplication, char *pcKey, char *pcValue, ULONG ulValueSize, BOOL bRead=FALSE);
    int                 accessProfile(char *pcApplication, char *pcKey, LONG *plValue, BOOL bRead=FALSE);
                                        /* Load a message from NLS message file */
    char               *loadMessage(ULONG ulMessage, char *pcParm0=0, char *pcParm1=0, 
                                                     char *pcParm2=0, char *pcParm3=0,
                                                     char *pcParm4=0);
    char               *loadMessage(ULONG ulMessage, ULONG ulParm0);
    char               *loadMessageHex(ULONG ulMessage, ULONG ulParm0);
    char               *loadMessage(ULONG ulMessage, ULONG ulParm0, ULONG ulParm1);
    char               *loadMessage(ULONG ulMessage, ULONG ulParm0, ULONG ulParm1, ULONG ulParm2);
                                        /* Load a message fron NLS message file and strip trailing whitespaces */
    char               *loadMessageStrip(ULONG ulMessage, char *pcParm0=0, char *pcParm1=0, 
                                                          char *pcParm2=0, char *pcParm3=0,
                                                          char *pcParm4=0);
    char               *loadMessageStrip(ULONG ulMessage, ULONG ulParm0);
    char               *loadMessageStrip(ULONG ulMessage, ULONG ulParm0, ULONG ulParm1);
    char               *loadMessageStrip(ULONG ulMessage, ULONG ulParm0, ULONG ulParm1, ULONG ulParm2);
                                        /* Display a message formatted for AVIO windows */
    UAnchor            &displayFormatted(char *pcMessage);
                                        /* Display a message on Logo window or message box and 
                                           allocFree() message*/
    UAnchor            &displayMessage(char *pcMessage, BOOL bLogoWindow);
                                        /* Display a message on modal to a window and allocFree() 
                                           message */
    ULONG               displayMessage(HWND hwndOwner, char *pcMessage, ULONG ulStyle=MB_INFORMATION,
                                       char *pcErrorModule=0, LONG lErrorLine=0);
                                        /* Display a message on modal to a window and don't 
                                           allocFree() message */
    ULONG               displayMessage(HWND hwndOwner, ULONG ulStyle, char *pcMessage);
                                        /* Log message to logfile if requested */
    ULONG               logMessage(char *pcMessage);
                                        /* Show or hide UStartupWindow Logo window */
    ULONG               showWindow(BOOL bShowLogoWindow=TRUE, BOOL bShowCloseButton=TRUE, ULONG ulError=NO_ERROR);
                                        /* Allocate memory outside any C++ heap */
    char               *allocMemory(ULONG ulSize);
    char               *allocString(char *pcString);
    char               *concatStrings(char *pcString1, char *pcString2);
    void                allocFree(char *pcAllocation);                                    
                                        /* Return selected profile */
    char               *getProfile(ULONG ulProfile);
                                        /* Retrieve OS/2 version */
    ULONG               getOS2VersionMajor(void);
    ULONG               getOS2VersionMinor(void);
protected:
                                        /* Protected constructor to allow initialization only
                                           by a derived class */
                        UAnchor(int argc, char **argv, int iStatus, char *pcApplicationName, char *pcPrefix);
                                        /* Display a message in a message box */
    UAnchor            &displayMessage(char *pcMessage);
                                        /* Strip trailing whitespaces and CRLF */
    char               *stripTrailing(char *pcMessage);
private:
                                        /* Copy of command line as we modify it when searching
                                           for a parameter */
    char               *pcCommandlineCopy;
                                        /* Commandline read from the CID file */
    char               *pcUserCommandline;
protected:
                                        /* Prf* APIs dynamically loaded */
    PRFOPENPROFILE     *pfnPrfOpenProfile;
    PRFWRITEPROFILEDATA
                       *pfnPrfWriteProfileData;
    PRFQUERYPROFILEDATA
                       *pfnPrfQueryProfileData;
    PRFCLOSEPROFILE    *pfnPrfCloseProfile;
public:
                                        /* Pointer to parameter structure that is allocated
                                           at runtime to be shared with all code DLLs */
    UStartupParameters *puStartupParameters;
                                        /* Debug status cache */
    ULONG               ulDebug;
                                        /* Return code to operating system */
    ULONG               ulReturnCode;
                                        /* Anchor cache */
    UAnchor            *puAnchor;
};

/****************************************************************************************\
 * Class: UStartupWindow                                                                *
\****************************************************************************************/

                                        /* UStartupWindow frame window */
#define ID_MAINWINDOW                   1
#define ID_LOGOGRAPHICS                 2
#define ID_LOGOTEST                     3

                                        /* Feature request flags that will be used */
                                        /* Environment options */
#define FEATURE_ENVIRONMENT_PM          0x00000001
#define FEATURE_ENVIRONMENT_AVIO        0x00000002
#define FEATURE_ADD_WINDOWLIST          0x00000004
#define FEATURE_PM_PRESENT              0x00000008
                                        /* Runtime options */
#define FEATURE_COMPILER_RUNTIMEDLL     0x00000010
#define FEATURE_COMPILER_CLASSLIBDLL    0x00000020
#define FEATURE_APP_RESOURCEDLL         0x00000040
#define FEATURE_APP_HELPFILE            0x00000080
                                        /* Logging options */
#define FEATURE_LOGFILE                 0x00000100
                                        /* Help options */
#define FEATURE_ONLINEHELP              0x00000200
                                        /* GUI options */
#define FEATURE_SHOW_HELP               0x00001000
#define FEATURE_SHOW_MESSAGEBOX         0x00002000
#define FEATURE_SHOW_CLOSEBUTTON        0x00004000


                                        /* Error codes UStartupWindow may throw during construction */
#define ERROR_NOENVIRONMENT             0x00000001
#define ERROR_ALLOCATE_MEMORY           0x00000002
#define ERROR_INITIALIZE_PM             0x00000004
#define ERROR_WINDOW_FAILED             0x00000008
#define ERROR_WINDOWWORDS_FAILED        0x00000010

class   UStartupWindow : public UAnchor
{
#define STARTUP_ENVIRONMENT             0x00000001
#define STARTUP_LOADMESSAGEFILE         0x00000002
#define STARTUP_THREADSTARTUP           0x00000003
#define STARTUP_LOADMODULES             0x00000004

#define LOGO_SHOWLOGOWINDOW             0x00000001
#define LOGO_SHOWCLOSEBUTTON            0x00000002
#define LOGO_CLOSEWITHERROR             0x00000004

                                        /* For PM UStartupWindowProc() really is a window
                                           procedure (called by using messages), while under
                                           AVIO we call it directly (which may even lead to
                                           nested calls) and "reuse" hwnd to replace the not
                                           available window words */
friend MRESULT EXPENTRY UStartupWindowProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
                                        /* Our woring thread */
friend ULONG _System    UStartupThread(UStartupWindow *puStartupWindow);

public:

                                        /* Public constructor that initializes parameter structure
                                           through UAnchor protected constructor */
                        UStartupWindow(int argc, char **argv, int iStatus, char *pcApplicationName, char *pcPrefix, int iMagicVersion);
                                        /* Destructor */
                        ~UStartupWindow(void);
                                        /* Message loop */
    UStartupWindow     &processWindow(void); 
protected:
                                        /* Setup environment */
    ULONG               setupEnvironment(void);
                                        /* Start thread */     
    ULONG               threadStartup(void);
                                        /* Load message file */
    ULONG               loadMessageFile(void);
                                        /* Load modules (files) */
    ULONG               loadModules(void);
                                        /* Show or hide Logo window */
    ULONG               showWindow(ULONG ulStatus);
private:
                                        /* Version check */
    int                 iMagicVersion;
};

#endif  /* _USTARTUP_HPP_ */
