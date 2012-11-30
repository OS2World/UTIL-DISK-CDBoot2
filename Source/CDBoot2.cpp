/***********************************************************************\
 *                              CDBoot/2                               *
 *              Copyright (C) by Stangl Roman, 2000, 2002              *
 * This Code may be freely distributed, provided the Copyright isn't   *
 * removed, under the conditions indicated in the documentation.       *
 *                                                                     *
 * CDBoot2.cpp  Entrypoint module                                      *
 *                                                                     *
\***********************************************************************/

#ifndef __PM__
#ifndef __AVIO__
#error(Either "__PM__" or "__AVIO__" must be defined)
#endif  /* __AVIO__ */
#endif  /* __PM__ */

#define     INCL_DOSERRORS
#define     INCL_WINDIALOGS
#include    <os2.h>

#include    "UStartup.hpp"
#include    "CDBoot2.hpp"

int     main(int argc, char *argv[])
{
    ULONG   ulReturnCode=NO_ERROR;
    ULONG   ulTemp;

#ifdef  DEBUG
#ifdef  __PM__
    printf("%s (PM): %s Build %s %s\n", BLDLEVEL_INFO, BLDLEVEL_PRODUCT, __DATE__, __TIME__);
#endif  /* __PM__ */
#ifdef  __AVIO__
    printf("%s (AVIO): %s Build %s %s\n", BLDLEVEL_INFO, BLDLEVEL_PRODUCT, __DATE__, __TIME__);
#endif  /* __AVIO__ */
#endif  /* DEBUG */
    UStartupWindow *puStartupWindow=0;

    try {
#ifdef  __PM__
        puStartupWindow=new UStartupWindow(argc, argv, FEATURE_ENVIRONMENT_PM | FEATURE_ADD_WINDOWLIST, 
            BLDLEVEL_INFO, BLDLEVEL_PREFIX, BLDLEVEL_SIGNATURE);
#endif  /* __PM__ */
#ifdef  __AVIO__
        puStartupWindow=new UStartupWindow(argc, argv, FEATURE_ENVIRONMENT_AVIO, 
            BLDLEVEL_INFO, BLDLEVEL_PREFIX, BLDLEVEL_SIGNATURE);
#endif  /* __AVIO__ */
        }
    catch(char *pcException)
        {
        ULONG   ulRc=0;

#ifdef  __PM__
        ulRc=WinMessageBox(HWND_DESKTOP, HWND_DESKTOP, (PSZ)pcException, BLDLEVEL_INFO,
            ID_MAINWINDOW, MB_OK | MB_ERROR | MB_DEFBUTTON1 | MB_MOVEABLE);
        if(ulRc==MBID_ERROR)
            exit(2);
#endif  /* __PM__ */
#ifdef  __AVIO__
        DosWrite(1, pcException, strlen(pcException), &ulTemp);
#endif  /* __AVIO__ */
        exit(CB2ERR_FATAL);
        }
                                        /* Process object */
    puStartupWindow->processWindow();
                                        /* Save return code */
    ulReturnCode=puStartupWindow->ulReturnCode;
                                        /* Destroy object */
    delete puStartupWindow;
    exit(ulReturnCode);
}

extern  "C"
{
long    _Optlink time(long *plTimer);
}

long    time(long *plTimer)
{
    return(*plTimer);
}

