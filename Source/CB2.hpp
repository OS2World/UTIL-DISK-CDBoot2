#ifndef _CB2_HPP_
#define _CB2_HPP_

/***********************************************************************\
 *                              CDBoot/2                               *
 *              Copyright (C) by Stangl Roman, 2000, 2002              *
 * This Code may be freely distributed, provided the Copyright isn't   *
 * removed, under the conditions indicated in the documentation.       *
 *                                                                     *
 * CD2.hpp      Code DLL main module header                            *
 *                                                                     *
\***********************************************************************/

#pragma pack(1)

                                        /* The checksum of CD2BOOT (XOR of all 32-bit words,
                                           thus the size of CD2BOOT must be a multiple of 4)
                                           should total to that */
#define CD2BOOT_CHECKSUM                0x55AA55AA
                                        /* Maximum size of a single message line in CD2BOOT */
#define LINESIZE                        80

                                        /* Offset of ERRORCTL structure in CD2BOOT. As this
                                           offset is hardcoded here and in CD2BOOT they have
                                           to be coordinated. The reason behind hardcoding is
                                           to ensure that this data structure and the code
                                           in CD2BOOT checking for fragmentation are located
                                           within the first 512 bytes of CD2BOOT, because the
                                           minimum cluster size is 512 bytes at least those
                                           512 bytes are contingous */
#define ERRORCTL_OFFSET                 16

typedef     struct _ERRORCTL            ERRORCTL;
struct  _ERRORCTL
{
    ULONG       ulChecksum;             /* Checksum  */
                                        /* Fragmentation message (which CD2BOOT will display
                                           if it during execution detects that it was saved
                                           fragmented onto the removeable media, and then
                                           halt the system) */
    UCHAR       aucFragmentMessage[LINESIZE];      
    BYTE        bReserved1[2];          /* Padding */
};

                                        /* CD2BOOT control structure within CD2BOOT file that
                                           gets patched according to the user preferences */
#define PWDSIZE                         8
typedef     struct _BOOTCTL             BOOTCTL;
struct  _BOOTCTL
{
    UCHAR       aucSignature[15];       /* Signature */
    BYTE        bReserved1;             /* Padding */
    USHORT      usStatusFlag;           /* Status corresponding to CDBOOT.ulStatusFlag */ 
    USHORT      usSeconds;              /* Seconds for CD2BOOT countdown */
                                        /* Password */
    UCHAR       aucPassword[PWDSIZE];
                                        /* Copyright messages color (high byte must be 0) */
    USHORT      usColorCopyright;
                                        /* Custom messages color (high byte must be 0) */
    USHORT      usColorCustomMessage;
                                        /* NLSMSG_CD2BOOT_COPYRIGHT1 */ 
    UCHAR       aucCopyright1[LINESIZE];      
    BYTE        bReserved2[2];          /* Padding */
                                        /* NLSMSG_CD2BOOT_COPYRIGHT2 */ 
    UCHAR       aucCopyright2[LINESIZE];      
    BYTE        bReserved3[2];          /* Padding */
                                        /* NLSMSG_CD2BOOT_COPYRIGHT3 */
    UCHAR       aucCopyright3[LINESIZE];      
    BYTE        bReserved4[2];          /* Padding */
                                        /* CUSTOMMESSAGE1 */
    UCHAR       aucCustomMessage1[LINESIZE];  
    BYTE        bReservedC1[2];         /* Padding */
                                        /* CUSTOMMESSAGE2 */
    UCHAR       aucCustomMessage2[LINESIZE];  
    BYTE        bReservedC2[2];          /* Padding */
                                         /* CUSTOMMESSAGE3 */
    UCHAR       aucCustomMessage3[LINESIZE];  
    BYTE        bReservedC3[2];         /* Padding */
                                        /* NLSMSG_CD2BOOT_SELECTMESSAGE1 */
    UCHAR       aucSelectMessage1[LINESIZE];  
    BYTE        bReserved5[2];          /* Padding */
                                        /* NLSMSG_CD2BOOT_SELECTMESSAGE2 */
    UCHAR       aucSelectMessage2[LINESIZE];  
    BYTE        bReserved6[2];          /* Padding */
                                        /* NLSMSG_CD2BOOT_SELECTMESSAGE3 */
    UCHAR       aucSelectMessage3[LINESIZE];  
    BYTE        bReserved7[2];          /* Padding */
                                        /* NLSMSG_CD2BOOT_SELECTMESSAGE4 */
    UCHAR       aucSelectMessage4[LINESIZE];
    BYTE        bReserved8[2];          /* Padding */
                                        /* NLSMSG_CD2BOOT_BOOTREMOVEABLE */
    UCHAR       aucBootRemoveable[LINESIZE];
    BYTE        bReserved9[2];          /* Padding */
                                        /* NLSMSG_CD2BOOT_BOOTHARDDISK */
    UCHAR       aucBootHarddisk[LINESIZE];
    BYTE        bReservedA[2];          /* Padding */
                                        /* NLSMSG_CD2BOOT_SYS02025 */   
    UCHAR       aucSys02025[LINESIZE];  
    BYTE        bReservedB[2];          /* Padding */
                                        /* NLSMSG_CD2BOOT_SYS02027 */   
    UCHAR       aucSys02027[LINESIZE];  
    BYTE        bReservedC[2];          /* Padding */
                                        /* NLSMSG_CD2BOOT_PASSWORDENTER */   
    UCHAR       aucPasswordEnter[LINESIZE];  
    BYTE        bReservedD[2];          /* Padding */
                                        /* NLSMSG_CD2BOOT_PASSWORDINVALID */   
    UCHAR       aucPasswordInvalid[LINESIZE];  
    BYTE        bReservedE[2];          /* Padding */
                                        /* NLSMSG_CD2BOOT_ADVANCEDMESSAGE1 */   
    UCHAR       aucAdvancedMessage1[LINESIZE];  
    BYTE        bReservedF[2];          /* Padding */
                                        /* NLSMSG_CD2BOOT_ADVANCEDMESSAGE2 */   
    UCHAR       aucAdvancedMessage2[LINESIZE];  
    BYTE        bReservedG[2];          /* Padding */
};

typedef     struct _EXT_BPB             EXT_BPB;
struct  _EXT_BPB
{
    BYTE        abOEMName[8];
    USHORT      usBytesPerSector;
    BYTE        bSectorsPerCluster;
    USHORT      usReservedSectors;
    BYTE        bCountFATs;
    USHORT      usCountRootEntries;
    USHORT      usCountSectors;
    BYTE        bMedia;
    USHORT      usSectorsPerFAT;
    USHORT      usSectorsPerTrack;
    USHORT      usCountHeads;
    ULONG       ulCountHiddenSectors;
    ULONG       ulCountLargeSectors;
    BYTE        bDriveNum;
    BYTE        bReserved1;
    BYTE        bSignature;
    ULONG       ulVolumeID;
    BYTE        abVolumeLabel[11];
    BYTE        abFileSysType[8];
};
                                        /* Layout of a bootsector of a normal
                                           FAT formatted removeable media */
typedef     struct _BOOTSECTOR          BOOTSECTOR;
struct  _BOOTSECTOR
{
    BYTE        abJMP[2];               /* Jump over BPB (Bios Parameter Block) */
    BYTE        bNOP;                   /* NOP opcode */
    EXT_BPB     extbpbMedia;            /* Extended Bios Parameter Block */
    BYTE        bBootCode[448];         /* Boot sector code */
    BYTE        bSignature[2];          /* Signature 0x55 0xAA */
};

#pragma pack()

typedef     struct _GETDEVICEPARAMS_PPF GETDEVICEPARAMS_PPF;
typedef     struct _GETDEVICEPARAMS_DPF GETDEVICEPARAMS_DPF;

struct  _GETDEVICEPARAMS_PPF
{
    UCHAR       ucInfoType;
    UCHAR       ucDriveUnit;
};

#define ATTRIBUTE_NONREMOVEABLE         0x00000001
#define ATTRIBUTE_CHANGELINEFLAG        0x00000002
#define ATTRIBUTE_GREATER16MB           0x00000004

#define DEVTYPE_OPTICAL                 0x0008
#define DEVTYPE_288D                    0x0009

#pragma pack(1)
struct  _GETDEVICEPARAMS_DPF
{
    BIOSPARAMETERBLOCK
                biosparameterblock;
    USHORT      usNumCylinder;
    UCHAR       ucDeviceType;
    USHORT      usDeviceAttributes;    
};
#pragma pack()

typedef     struct _REMOVEABLE_PPF      REMOVEABLE_PPF;
typedef     struct _REMOVEABLE_DPF      REMOVEABLE_DPF;

struct _REMOVEABLE_PPF                  /* Parameter Packet Format for DSK_BLOCKREMOVEABLE */
{
    BYTE        bCommandInformation;
    BYTE        bDriveUnit;
};

struct _REMOVEABLE_DPF                  /* Data Packet Format for DSK_BLOCKREMOVEABLE */
{
    BYTE        bData;
};

/****************************************************************************************\
 * Class: CD2Boot                                                                       *
\****************************************************************************************/

                                        /* Flag ulStatusFlag */
#define     STATUS_CID_DRIVE            0x00000001
#define     STATUS_CID_TIMEOUT          0x00000002
#define     STATUS_CID_CUSTOMMESSAGE    0x00000004
#define     STATUS_CID_PASSWORD         0x00000008
#define     STATUS_CID_HARDDISK         0x00000010
#define     STATUS_CID_REMOVEABLE       0x00000020
#define     STATUS_CID_CLEARSCREEN      0x00000040
#define     STATUS_CID_BMBOOT           0x00000080
#define     STATUS_CID_MODEIBM          0x00000100
#define     STATUS_CID_MODECID          0x00000200
#define     STATUS_CID_COLORCOPYRIGHT   0x00000400
#define     STATUS_CID_COLORCUSTOM      0x00000800

                                        /* Signature to look into CD2BOOT for to prevent
                                           a version mismatch. The asterisks is our wildcard
                                           character, that is we accept minor changes at the
                                           one-digit level, but all major changes and ten-digit
                                           minor changes require a new pair of CD2BOOT
                                           and CDBOOT/CDBOOTPM */
#define     SIGNATURE                   "@CD2BOOT V2.0*@"
                                        /* FAT (8.3) conforming strings to look for in an
                                           OS/2 boot sector on a removeable media */
#define     SIGNATURE_OS2BOOT           "OS2BOOT    "
#define     SIGNATURE_CD2BOOT           "CD2BOOT    "
                                        /* FAT removeable media bootsector signature */
#define     SIGNATURE_FAT               "FAT"
                                        /* Define minimum and maximum delay */
#define     DELAY_MINIMUM               1
#define     DELAY_DEFAULT               10
#define     DELAY_MAXIMUM               60

                                        /* OS/2 bootsector on removeable media */
#define     SECTORSIZE                  512

class   CD2Boot
{
                                        /* That allows us to get back into the context of
                                           the object from the function window procedure */
friend MRESULT EXPENTRY UStartupWindowProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

public:
                                        /* Constructor */
                    CD2Boot(UAnchor *puAnchor);
                                        /* Desctructor */
                    ~CD2Boot(void);
                                        /* Install the removeable media boot modification */
    ULONG           InstallCD2Boot(void);
                                        /* CDBootPM window procedure (PM) */
    MRESULT         CB2_DialogProc(HWND hwndDlg, ULONG msg, MPARAM mp1, MPARAM mp2);
                                        /* CDBoot interactive dialog (AVIO) */
    MRESULT         CB2_Interactive(void);
                                        /* Set status */
    MRESULT         SetStatus(ULONG ulStatusFlagMask, UCHAR *pucParameter, ULONG ulParameter);
                                        /* Get status */
    ULONG           GetStatus(ULONG ulStatusFlagMask=0);
private:
    UAnchor        *puAnchorCopy;       /* Reference to UAnchor copy */
                                        /* Load NLS messages for CD2BOOT into pBootControl */
    ULONG           LoadBootControlMessages(void);  
                                        /* Load checksum for CD2BOOT into pErrorControl */
    ULONG           LoadErrorControlChecksum(void);  
                                        /* Read or write the bootsector of the drive specified */
    UCHAR          *BootSectorIO(UCHAR *pucDrive, BOOL bWrite=FALSE);
                                        /* Read or write the file "CD2BOOT" into/from storage */
    ULONG           CD2BootIO(UCHAR *pucCD2Boot, BOOL bWrite=FALSE);
                                        /* Search a string (optinally containing * wildcards) 
                                           within a given buffer */
    ULONG           SearchBuffer(UCHAR *pucBuffer, ULONG ulBufferSize, UCHAR *pucString);
                                        /* Encrypt the password in place */ 
    ULONG           EncryptPassword(void);
    ULONG           ulStatusFlag;       /* Status flag */
                                        /* Bootsector loaded from removeable media */
    BOOTSECTOR      bootsectorRemoveable;
                                        /* Bootsector of a removeable OS/2 media */
    BOOTSECTOR      bootsectorRemoveableOS2;
                                        /* Offset of "OS2BOOT" within Bootsector which
                                           then gets replaced by the string "CD2BOOT" */
    ULONG           ulOS2BOOTOffset;                  
    BYTE           *pbCD2BOOT;          /* CD2BOOT loaded into dynamically allocated memory */
    ULONG           ulCD2BOOTSize;      /* Size of CD2BOOT file */
    ULONG           ulBootSectorOffset; /* OS/2 boot sector within CD2BOOT file */
                                        /* CD2BOOT control structure within CD2BOOT file */
    ULONG           ulBootControlOffset;
    BOOTCTL        *pBootControl;       /* Cache for pointer to CD2BOOT BOOTCTL control structure */
    ERRORCTL       *pErrorControl;      /* Cache for pointer to CD2BOOT ERRORCTL control structure */
                                        /* Path to the CD2BOOT boot loader */
    UCHAR           aucCD2BootPath[CCHMAXPATH];
    UCHAR           ucDrive;            /* Drive to install CD2BOOT into */
    USHORT          usSeconds;          /* Time period for decission to be patched into CD2BOOT */
                                        /* Copyright messages */
    USHORT          usCopyrightColor;
    UCHAR           aucCopyright1[LINESIZE];
    UCHAR           aucCopyright2[LINESIZE];
    UCHAR           aucCopyright3[LINESIZE];
                                        /* Optional custom messages */
    USHORT          usCustomMessageColor;
    UCHAR           aucCustomMessage1[LINESIZE];
    UCHAR           aucCustomMessage2[LINESIZE];
    UCHAR           aucCustomMessage3[LINESIZE];
                                        /* Optional password */
    UCHAR           aucPassword[PWDSIZE];
                                        /* Drive map for all drives that tell to be removeable
                                           (Bit 0=A, Bit 1=B, Bit 2=C, ...) */
    ULONG           ulRemoveableMap;
                                        /* Optional user specified a CID file to read parameters
                                           from */
    UCHAR           aucCIDFile[CCHMAXPATH];
};

#endif  /* _CB2_HPP_ */
