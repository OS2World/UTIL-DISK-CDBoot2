#ifndef _CDBOOT2_HPP_
#define _CDBOOT2_HPP_

/***********************************************************************\
 *                              CDBoot/2                               *
 *              Copyright (C) by Stangl Roman, 2000, 2002              *
 * This Code may be freely distributed, provided the Copyright isn't   *
 * removed, under the conditions indicated in the documentation.       *
 *                                                                     *
 * CDBoot2.hpp  CDBootPM header file                                   *
 *                                                                     *
\***********************************************************************/

                                        /* Don't include if we compile HLP files */
#ifndef IPFC
                                        /* Environment include files */
#define         INCL_WIN
#define         INCL_GPI
#define         INCL_DOS
#define         INCL_BASE
#define         INCL_DOSERRORS
#define         INCL_GPIERRORS
#define         INCL_DOSPROCESS
#define         INCL_DOSDEVIOCTL
#include        <os2.h>
                                        /* C Set++ include files */
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <ctype.h>
#include        <memory.h>
#include        <time.h>
#include        <builtin.h>
#endif /* IPFC */

                                        /* BLDLEVEL information (in C source modules added via macro
                                           concatenation) for BuildLevel.cmd to generate BLDLEVEL information.
                                           Version and Signature should correspond (at least for a GA build) */
#define BLDLEVEL_VENDOR         "(C) Roman_Stangl@at.ibm.com"
#define BLDLEVEL_VERSION        "2.00 (05, 2002)"
#define BLDLEVEL_SIGNATURE      0x00020000
#define BLDLEVEL_RELEASE        ((((BLDLEVEL_SIGNATURE & 0xFFFF0000)>>16)*100)+(BLDLEVEL_SIGNATURE & 0x000000FF))
#define BLDLEVEL_INFO           "CD Boot/2"
#define BLDLEVEL_PRODUCT        "CB/2"
#define BLDLEVEL_PREFIX         "CB2"

                                        /* Return codes */
#define CB2ERR_NO_ERROR                 (0)
#define CB2ERR_WARNING                  (1)
#define CB2ERR_FATAL                    (2)

                                        /* Messages */
#define NLSMSG_SYNTAX_AVIO              (50)
#define NLSMSG_SYNTAX_PM                (51)
#define NLSMSG_ERR_SYNTAX_DRIVE         (52)
#define NLSMSG_ERR_SYNTAX_TIMEOUT       (53)
#define NLSMSG_ERR_SYNTAX_PARAMETERS1   (54)
#define NLSMSG_ERR_SYNTAX_PARAMETERS2   (55)
#define NLSMSG_CID_COMMANDLINE_OK       (56)
#define NLSMSG_ERR_SYNTAX_CUSTOMMESSAGE (57)
#define NLSMSG_ERR_SYNTAX_PASSWORD      (58)
#define NLSMSG_CID_UNATTENDED           (59)

#define NLSMSG_ERR_HABDIALOG            (70)
#define NLSMSG_ERR_HMQDIALOG            (71)
#define NLSMSG_ERR_HWNDDIALOG           (72)
#define NLSMSG_ERR_DOSOPEN              (73)
#define NLSMSG_ERR_LOCKING              (74)
#define NLSMSG_ERR_SECTORIO             (75)
#define NLSMSG_ERR_DOSCLOSE             (76)
#define NLSMSG_ERR_CD2BOOTIO            (77)
#define NLSMSG_ERR_CD2BOOTSIGNATURE     (78)
#define NLSMSG_ERR_OS2BOOTMISSING       (79)

#define NLSMSG_INFORMDRIVE              (80)
#define NLSMSG_QUERYDRIVE               (81)
#define NLSMSG_INFORMSECONDS            (82)
#define NLSMSG_QUERYSECONDS             (83)
#define NLSMSG_READBOOTSECTOR           (84)
#define NLSMSG_READCD2BOOT              (85)
#define NLSMSG_CD2BOOTALREADYRUN        (86)
#define NLSMSG_FOUNDFATBOOTSECTOR       (87)
#define NLSMSG_FOUNDUNKNOWNBOOTSECTOR   (88)
#define NLSMSG_WRITECD2BOOT             (89)
#define NLSMSG_ADJUSTBOOTSECTOR         (90)
#define NLSMSG_WRITEBOOTSECTOR          (91)

#define NLSMSG_INITIALIZECB2            (98)
#define NLSMSG_TERMINATECB2             (99)

                                        /* The following messages are limited to NLSMSG_CD2BOOT_LENGTH 
                                           characters (to prevent linefeeds during display in 
                                           CD2BOOT), but this is not checked! */
#define NLSMSG_CD2BOOT_LENGTH           LINESIZE-1

#define NLSMSG_CD2BOOT_COPYRIGHT1       (100)
#define NLSMSG_CD2BOOT_COPYRIGHT2       (101)
#define NLSMSG_CD2BOOT_COPYRIGHT3       (102)
#define NLSMSG_CD2BOOT_SELECTMESSAGE1   (103)
#define NLSMSG_CD2BOOT_SELECTMESSAGE2   (104)
#define NLSMSG_CD2BOOT_SELECTMESSAGE3   (105)
#define NLSMSG_CD2BOOT_SELECTMESSAGE4   (106)
#define NLSMSG_CD2BOOT_BOOTREMOVEABLE   (107)
#define NLSMSG_CD2BOOT_BOOTHARDDISK     (108)
#define NLSMSG_CD2BOOT_SYS02025         (109)
#define NLSMSG_CD2BOOT_SYS02027         (110)
#define NLSMSG_CD2BOOT_PASSWORDENTER    (111)
#define NLSMSG_CD2BOOT_PASSWORDINVALID  (112)
#define NLSMSG_CD2BOOT_ADVANCEDMESSAGE1 (113)
#define NLSMSG_CD2BOOT_ADVANCEDMESSAGE2 (114)
#define NLSMSG_CD2BOOT_FRAGMENTMESSAGE  (115)

                                        /* The following are just dialog control NLS strings */
#define NLSMSG_GRP_CDBOOT2              (150)
#define NLSMSG_GRP_SETTINGS             (151)
#define NLSMSG_ST_DRIVE                 (152)
#define NLSMSG_ST_CD2BOOT               (153)
#define NLSMSG_ST_SECONDS               (154)
#define NLSMSG_RB_DEFAULTHARDDISK       (155)
#define NLSMSG_RB_DEFAULTREMOVEABLE     (156)
#define NLSMSG_CB_CLEARSCREEN           (157)
#define NLSMSG_GRP_PASSWORD             (158)
#define NLSMSG_CB_PASSWORD              (159)
#define NLSMSG_GRP_ADVANCED             (160)
#define NLSMSG_CB_BMBOOT                (161)
#define NLSMSG_GRP_COPYRIGHT            (162)
#define NLSMSG_GRP_CUSTOMMESSAGE        (163)
#define NLSMSG_ST_DID_OK                (164)
#define NLSMSG_ST_DID_CANCEL            (165)
                                        /* Foreground colors (used for copyright and custom
                                           messages */
#define NLSMSG_ST_COLOR                 (166)
#define NLSMSG_COLOR0                   (167)
#define NLSMSG_COLOR1                   (168)
#define NLSMSG_COLOR2                   (169)
#define NLSMSG_COLOR3                   (170)
#define NLSMSG_COLOR4                   (171)
#define NLSMSG_COLOR5                   (172)
#define NLSMSG_COLOR6                   (173)
#define NLSMSG_COLOR7                   (174)
#define NLSMSG_COLOR8                   (174)
#define NLSMSG_COLOR9                   (176)
#define NLSMSG_COLOR10                  (177)
#define NLSMSG_COLOR11                  (178)
#define NLSMSG_COLOR12                  (179)
#define NLSMSG_COLOR13                  (180)
#define NLSMSG_COLOR14                  (181)
#define NLSMSG_COLOR15                  (182)


#endif  /* _CDBOOT2_HPP_ */
