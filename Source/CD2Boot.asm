;                              CD2Boot/2
;              Copyright (C) by Stangl Roman, 2000, 2002
;                       Roman_Stangl@at.ibm.com
;                 http://www.geocities.com/warpguru/
; This Code may be freely distributed, provided the Copyright isn't
; removed, under the conditions indicated in the documentation.
;
; CD2Boot.asm   CD2Boot assembler source file
;
;               CDBoot/CDBootPM will replace the buffer containing 512 bytes
;               of "CD2BOOT " strings with a boot sector of an bootable OS/2
;               installation diskette. It then will patch the bootsector to
;               load CD2BOOT instead of OS2BOOT and copy the patched CD2BOOT
;               file onto the installation diskette.
;               CD2BOOT will wait 10 seconds (by polling the hardware timer that
;               ticks at offset 06C in the BIOS data area) for the user to decide
;               to continue booting from the removeable media (diskette drive or
;               bootable CD-ROM diskette image) or to boot from the first harddisk
;               instead.
;
;               To allow debugging of CD2Boot build it using the Makefile, then
;               load CD2Boot.com it into a real-mode debugger, set IP to 0000
;               (instead of 0100) and add 0010 to CS (because *.COM are binary
;               images of code that DOS loads (and thus the debugger too) at
;               offset 0100 (as the first 100 bytes are DOS's PSP - Program Segment
;               Prefix, our having a ORG 0 statement causes all offset
;               calculations to be done from 0 so loading it at offset 100 will
;               cause all absolute addresses to be wrong by 100).
;
;               When debugging under DOS it might be useful to build with "DEBUG=1"
;               as this will not move the sector to be booted from CD2Boot to
;               the offset 0000:7C00 (where parts of the DOS kernel reside) but
;               in a buffer withing CD2Boot.
;
;               OS/2 is a great environment to debug such stuff, as the DOS boxes
;               are protected, and you can simulate a diskette boot (using the
;               "DOS from Drive A:" option (however that does not allow low level
;               fixed disk access to the partition table). Note however, that
;               the countdown only works in real-time if you keep the DOS box
;               busy (e.g. by using the shift keys), as OS/2 will preemt a DOS
;               box doing nothing than waiting (e.g. for a keystroke)
;
;               Note! One word about using the "INVOKE" statement, be sure to
;                     remember it uses the AX register to push the function
;                     parameters onto the stack!

;----------------------------------------------------------------------------------------
; Keep assembler quiet about variables that follow the code (I would have
; expected as the assembler is a 2-pass (IMHO) compiler to be able to resolv
; them without a forward declaration but it seems to be too silly)
;----------------------------------------------------------------------------------------
EXTERNDEF   Putstr_Color:WORD
EXTERNDEF   Cr:BYTE
EXTERNDEF   CrLf:BYTE
EXTERNDEF   ClearLine:BYTE
EXTERNDEF   SecondsOffset:WORD
EXTERNDEF   TimerTicks:WORD
EXTERNDEF   OS2BootSec:BYTE
EXTERNDEF   BootControl:BYTE
EXTERNDEF   PartitionTable:BYTE

FALSE                       EQU         0
TRUE                        EQU         1

CHAR_CR                     EQU         0Dh
CHAR_LF                     EQU         0Ah

COLOR_LITEWHITE_ON_BLUE     EQU         01Fh

                                        ; CD2BOOT checksum (XOR of all 32-bit words, thus
                                        ; CD2BOOT's size must be a multiple of 4) should 
                                        ; total to
CD2BOOT_CHECKSUM            EQU         055AA55AAh
                                        ; Size of a message line
LINESIZE                    EQU         80
                                        ; Size of password
PWDSIZE                     EQU         8

KEY_NONE                    EQU         '0'
KEY_1                       EQU         '1'
KEY_2                       EQU         '2'
KEY_ENTER1                  EQU         0Ch
KEY_ENTER2                  EQU         0Dh
KEY_SPACEBAR                EQU         ' '

;----------------------------------------------------------------------------------------
; Debugging help, just write CRLF, the char passed and CRLF again
;----------------------------------------------------------------------------------------
DBG     MACRO   cChar:REQ
        Mov     BX,0h
        Mov     CX,010h
        Mov     Ah,0Eh
        Mov     AL,0Ah
        Int     010h
        Mov     BX,0h
        Mov     CX,010h
        Mov     Ah,0Eh
        Mov     AL,0Dh
        Int     010h
        Mov     BX,0h
        Mov     CX,010h
        Mov     Ah,0Eh
        Mov     AL,cChar
        Int     010h
        Mov     BX,0h
        Mov     CX,010h
        Mov     Ah,0Eh
        Mov     AL,0Ah
        Int     010h
        Mov     BX,0h
        Mov     CX,010h
        Mov     Ah,0Eh
        Mov     AL,0Dh
        Int     010h
        ENDM

;----------------------------------------------------------------------------------------
; CD2BOOT control structure
;----------------------------------------------------------------------------------------

                                        ; Flag StatusFlag
STATUS_CID_DRIVE            EQU         00001h
STATUS_CID_TIMEOUT          EQU         00002h
STATUS_CID_CUSTOMMESSAGE    EQU         00004h
STATUS_CID_PASSWORD         EQU         00008h
STATUS_CID_HARDDISK         EQU         00010h
STATUS_CID_REMOVEABLE       EQU         00020h
STATUS_CID_CLEARSCREEN      EQU         00040h
STATUS_CID_BMBOOT           EQU         00080h

BOOTCTL                     STRUCT
                                        ; Signature to find this control
                                        ; structure so that CDBOOT/CDBOOTPM can
                                        ; find and patch it
        Signature           DB  15 DUP(" ")
                            DB  0h
        StatusFlag          DW  0h      ; Statusflag
                                        ; Number of seconds for countdown
        Seconds             DW  0h
                                        ; Optional password, ignored if first char is 0h
        Password            DB  PWDSIZE DUP(0h);
                                        ; Copyright messages color (high byte must be 0)
        Color_Copyright     DW  0h
                                        ; Custom messages color (high byte must be 0)
        Color_CustomMessage DW  0h
                                        ; CD Boot/2 Copyright messages
        Copyright_1         DB  LINESIZE DUP("2")
                            DB  0h, 0h
        Copyright_2         DB  LINESIZE DUP("3")
                            DB  0h, 0h
        Copyright_3         DB  LINESIZE DUP("4")
                            DB  0h, 0h
                                        ; Optional Custom message(s), ignored if first
                                        ; char is 0h
        CustomMessage1      DB  LINESIZE DUP(0h)
                            DB  0h, 0h
        CustomMessage2      DB  LINESIZE DUP(0h)
                            DB  0h, 0h
        CustomMessage3      DB  LINESIZE DUP(0h)
                            DB  0h, 0h
                                        ; Information about countdown
                                        ; Please press the key for the media ( is default) you want to boot from:
        SelectMessage1      DB  LINESIZE DUP("5")
                            DB  0h, 0h
                                        ;   1 ... From the first harddisk
        SelectMessage2      DB  LINESIZE DUP("6")
                            DB  0h, 0h
                                        ;   2 ... From the removeable media
        SelectMessage3      DB  LINESIZE DUP("7")
                            DB  0h, 0h
                                        ; Please select within %d seconds, or boot continues from the default media!
        SelectMessage4      DB  LINESIZE DUP("8")
                            DB  0h, 0h
                                        ; Continuing boot from removeable media
        BootRemoveable      DB  LINESIZE DUP("9")
                            DB  0h, 0h
                                        ; Continuing boot from first harddisk
        BootHarddisk        DB  LINESIZE DUP("A")
                            DB  0h, 0h
                                        ; CD2Boot SYS02025: Disk read error loading partition table from first harddisk!
        Sys02025            DB  LINESIZE DUP("B")
                            DB  0h, 0h
                                        ; CD2Boot SYS02027: Insert a system diskette and restart the system!
        Sys02027            DB  LINESIZE DUP("C")
                            DB  0h, 0h
                                        ; Please enter Password to boot removeable media: 
        PasswordEnter       DB  LINESIZE DUP("D")
                            DB  0h, 0h
                                        ; Invalid Password, please retry or press ALT+CTRL+DEL to reboot:  
        PasswordInvalid     DB  LINESIZE DUP("E")
                            DB  0h, 0h
                                        ; No active OS/2 Bootmanager Partition found in partition table.
        AdvancedMessage1    DB  LINESIZE DUP("F")
                            DB  0h, 0h
                                        ; Active OS/2 Bootmanager Partition found in partition table.
        AdvancedMessage2    DB  LINESIZE DUP("G")
                            DB  0h, 0h
BOOTCTL                     ENDS

                                        ; Flag ActiveFlag
ACTIVEFLAG_ACTIVE           EQU         080h

                                        ; Flag TypeFlag
TYPEFLAG_BOOTMANAGER        EQU         00Ah

                                        ; Entry for a partition in the partition table
PARTITION                   STRUC
        ActiveFlag          DB  0h
        StartingCHS         DB  0h, 0h, 0h
        TypeFlag            DB  0h
        EndingCHS           DB  0h, 0h, 0h
        StartingLBA         DD  0h
        EndingLBA           DD  0h
PARTITION                   ENDS

                                        ; Partition table (totals to 512 bytes)
PARTITIONTBL                STRUC
        PartitionCode       DB  446 DUP(0h)
        Partition1          PARTITION < >
        Partition2          PARTITION < >
        Partition3          PARTITION < >
        Partition4          PARTITION < >
        PartitionSignatore  DB  2 DUP(0h)
PARTITIONTBL                ENDS

;----------------------------------------------------------------------------------------
; Our single code and data segment
;----------------------------------------------------------------------------------------
CSEG    SEGMENT DWORD PUBLIC 'CODE'
        ASSUME CS:CSEG, DS:CSEG, ES:CSEG
        .386

Main    PROTO NEAR C
ClrScrn PROTO NEAR C
SetScrn PROTO NEAR C
Init    PROTO NEAR C pBootControl:PTR BYTE, pTimerTicks:PTR WORD
Putstr  PROTO NEAR C pbString:PTR BYTE
Kbhit   PROTO NEAR C
GetPwd  PROTO NEAR C pBootControl:PTR BYTE
GetTime PROTO NEAR C pbString:PTR BYTE
WaitSec PROTO NEAR C ubSeconds:BYTE
MovSec  PROTO NEAR C pbSector:PTR BYTE
GetDisk PROTO NEAR C pbSector:PTR BYTE

;*--------------------------------------------------------------------------------------*
;* START()                                                                              *
;*--------------------------------------------------------------------------------------*
        ORG     0
START   PROC    FAR
;----------------------------------------------------------------------------------------
; Here we start execution when CD2BOOT was installed onto a bootable OS/2 installation 
; diskette (or into an bootable image of that diskette on a CD-ROM drive). BIOS has 
; loaded us as 0800:0000, and the stack typically points to 0000:7B00 and grows 
; downwards.
;
; CD2BOOT must not be fragmented! The boot sector code is not able due to its space 
; constraints to load fragmented file, boot will fail and/or hang then!
; To ensure that we at least can tell the user that we're fragmented, we put our check
; for fragmentation within the first 512 bytes (because if the minimum cluster size is
; 512 bytes, the cluster size will always be contingous)!
;----------------------------------------------------------------------------------------
                                        ; Load segment registers
        Cli
        Mov     AX,CS
        Mov     DS,AX
        Mov     ES,AX
        Jmp     $+2
        Sti
        Jmp     Main
START   ENDP

;----------------------------------------------------------------------------------------
; At least the complete PutStr() and beginning of the Main() functions need to be within
; the first 512 bytes. As the Crc32 and FragmentMessage will be filled by CDBOOT/CDBOOTPM
; the offset MUST be known to them and CD2BOOT!
;----------------------------------------------------------------------------------------
        ORG     16
                                        ; Checksum of CD2BOOT file filled by CDBOOT/CDBOOTPM
                                        ; so that including all bytes of CD2BOOT in the
                                        ; checksum calculation results in an overal value
                                        ; of 0x55AA55AA
Checksum        DD  0h
                                        ; Message to be shown when fragmentation is
                                        ; detected before system is halted 
FragmentMessage DB  LINESIZE DUP("@")
                DB  0h, 0h

;*--------------------------------------------------------------------------------------*
;* Putstr(pbString)                                                                     *
;*          Write the string addressed by pbString to the current cursor position       *
;*          on the screen                                                               *
;*--------------------------------------------------------------------------------------*
                                        ; Default color used for PutStr()
Putstr_Color    DW  COLOR_LITEWHITE_ON_BLUE

Putstr  PROC NEAR C USES BX CX SI, pbString:PTR BYTE
        Mov     SI,pbString
        Mov     AL,[SI]
        .WHILE  (AL!=0)
            Push    SI
            .IF     ((AL==CHAR_CR) || (AL==CHAR_LF))
                                        ; Write character in AL onto screen
                Mov     BL,0h
                Mov     CX,01h
                Mov     AH,0Eh
                Int     010h
            .ELSE
                Push    AX
                                        ; Read cursor position into DH:DL
                Mov     BH,0h
                Mov     AH,03h
                Int     010h
                                        ; Write character in AL with attribute in BL
                                        ; onto screen
                Pop     AX
                Push    DX
                Mov     CX,01h
                Mov     BX,Putstr_Color
                Mov     AH,09h
                Int     010h
                Pop     DX
                                        ; Set cursor position form DH:DL
                Inc     DL
                Mov     BH,0h
                Mov     AH,02h
                Int     010h

            .ENDIF
                                        ; Get next character to write
            Pop     SI
            Cld
            Inc     SI
            Mov     AL,[SI]
        .ENDW
        Ret
Putstr  ENDP

;*--------------------------------------------------------------------------------------*
;* SetScrn(void)                                                                        *
;*          Change all characters on the screen into color characters (light white on   *
;*          blue being the default)                                                     *
;*--------------------------------------------------------------------------------------*
COLOR_FRAMEBUFFER           EQU         0B800h

SetScrn PROC NEAR C USES AX BX ES
        Push    COLOR_FRAMEBUFFER
        Pop     ES
        Mov     BX,0h
        .WHILE  (BX<=(80*26*2))
            Mov     AX,ES:[BX]
            Mov     AH,COLOR_LITEWHITE_ON_BLUE
            Mov     ES:[BX],AX
            Add     BX,02h
        .ENDW
        Ret
SetScrn ENDP

;*--------------------------------------------------------------------------------------*
;* Main()                                                                               *
;*--------------------------------------------------------------------------------------*
Main    PROC NEAR C
                                        ; Get end of this segment which is also the end
                                        ; of CD2BOOT
        Mov     CX,Eof                 
        Mov     BX,0h                   ; Start at offset 0
                                        ; Start with checksum 0 in DX:AX
        Mov     DX,0h                  
        Mov     AX,0h
                                        ; Calculate checksum. Note that this may not work
                                        ; when running under the debugger, because if you
                                        ; set a breakpoint the debugger patches the memory
                                        ; with a INT 3 (0xCC) instruction which would
                                        ; change the checksum calculation
        .WHILE  (BX<CX)
            Xor     AX,[BX]
            Add     BX,02h            
            Xor     DX,[BX]
            Add     BX,02h            
        .ENDW
IFDEF   DEBUG
                                        ; Under a debugger the checksum calculation
                                        ; will fail due to the patches from the debugger
        Mov         BX, CD2BOOT_CHECKSUM
        Mov         AX, CD2BOOT_CHECKSUM
ENDIF   
                                        ; Check if DX:AX now contains the checksum expected
                                        ; If the checksum is correct continue, otherwise
                                        ; halt the system
        .IF     ((DX!=CD2BOOT_CHECKSUM) || (AX!=CD2BOOT_CHECKSUM))
            Invoke  Putstr, ADDR FragmentMessage
M_FragmentationDetected:
                                        ; Halt further execution of CD2BOOT as it wasn't
                                        ; written contingously on the removeable media and
                                        ; thus will finally fail somewhere (we can't predict
                                        ; where)! 
            Jmp     M_FragmentationDetected
        .ENDIF

                                        ; Read all keys from keyboard buffer so
                                        ; it becomes empty before the user is
                                        ; requested to make a selection
        .REPEAT
            Invoke  Kbhit
        .UNTIL  (AX==0)
        Lea     BX,BootControl
                                        ; Clear screen if requested
        Test    [BX].BOOTCTL.StatusFlag,STATUS_CID_CLEARSCREEN
        .IF     (!ZERO?)
            Invoke  ClrScrn
        .ENDIF
                                        ; Set video RAM into default colors (light white on blue)
        Invoke  SetScrn
                                        ; Display copyright message
        Mov     AX, [BX].BOOTCTL.Color_Copyright
        Mov     Putstr_Color, AX
        Invoke  Putstr, ADDR [BX].BOOTCTL.Copyright_1
        Invoke  Putstr, ADDR CrLf
        Invoke  Putstr, ADDR [BX].BOOTCTL.Copyright_2
        Invoke  Putstr, ADDR CrLf
        Invoke  Putstr, ADDR [BX].BOOTCTL.Copyright_3
        Invoke  Putstr, ADDR CrLf
        Invoke  Putstr, ADDR CrLf
                                        ; If optional custom messages exist, display
                                        ; them
        Mov     AX, [BX].BOOTCTL.Color_CustomMessage
        Mov     Putstr_Color, AX
        Mov     CX,0h
        Lea     DI,[BX].BOOTCTL.CustomMessage1
        .IF     (BYTE PTR [DI]!=0h)
            Inc     CX
            Invoke  Putstr, ADDR [BX].BOOTCTL.CustomMessage1
            Invoke  Putstr, ADDR CrLf
        .ENDIF
        Lea     DI,[BX].BOOTCTL.CustomMessage2
        .IF     (BYTE PTR [DI]!=0h)
            Inc     CX
            Invoke  Putstr, ADDR [BX].BOOTCTL.CustomMessage2
            Invoke  Putstr, ADDR CrLf
        .ENDIF
        Lea     DI,[BX].BOOTCTL.CustomMessage3
        .IF     (BYTE PTR [DI]!=0h)
            Inc     CX
            Invoke  Putstr, ADDR [BX].BOOTCTL.CustomMessage3
            Invoke  Putstr, ADDR CrLf
        .ENDIF
        .IF     (CX!=0h)
            Invoke  Putstr, ADDR CrLf
        .ENDIF
                                        ; Reset color to default value now
        Mov     Putstr_Color, COLOR_LITEWHITE_ON_BLUE
                                        ; If the Advanced Harddisk boot is selected,
                                        ; try to load the first harddisk's partition
                                        ; table and check if an active OS/2 Bootmanager
                                        ; partition is found
        Test    [BX].BOOTCTL.StatusFlag,STATUS_CID_BMBOOT
        .IF     (!ZERO?)
                                        ; Read partitioon table into temporary space
                                        ; PartitonTable and check if the active
                                        ; partition is the OS/2 Bootmanager
            Invoke  GetDisk, ADDR PartitionTable
            Lea     DI,PartitionTable.Partition1
            Mov     CX,4
            .WHILE  (CX!=0h)
                Dec     CX
                .IF     ([DI].PARTITION.TypeFlag==TYPEFLAG_BOOTMANAGER)
                    Test    [DI].PARTITION.ActiveFlag,ACTIVEFLAG_ACTIVE
                    .IF     (!ZERO?)
                        Invoke  Putstr, ADDR [BX].BOOTCTL.AdvancedMessage2
                        Invoke  Putstr, ADDR CrLf
                        Invoke  Putstr, ADDR CrLf
;                        Invoke  Putstr, ADDR [BX].BOOTCTL.BootHarddisk
;                                        ; Move loaded partition table of first harddisk
;                                        ; to 0000:7C00
;                        Invoke  MovSec, ADDR PartitionTable
;                                        ; And jump to where we jump to the boot code
;                                        ; we finally want to execute
;                        Jmp     M_ExecuteBootCode

                                        ; Override user selection of which medium to boot
                                        ; by default so that always the harddisk becomes
                                        ; the default boot media
                        Lea     DI,[BX].BOOTCTL.SelectMessage2
                        Mov     Byte Ptr [DI+02h],''
                        Lea     DI,[BX].BOOTCTL.SelectMessage3
                        Mov     Byte Ptr [DI+02h],' '
                        And     [BX].BOOTCTL.StatusFlag,Not STATUS_CID_REMOVEABLE
                        Or      [BX].BOOTCTL.StatusFlag,STATUS_CID_HARDDISK
                        Jmp     M_BMFoundDefaultToHarddisk
                    .ENDIF
               .ENDIF
                Add     DI,SIZEOF PARTITION
            .ENDW
            Invoke  Putstr, ADDR [BX].BOOTCTL.AdvancedMessage1
            Invoke  Putstr, ADDR CrLf
            Invoke  Putstr, ADDR CrLf
        .ENDIF
M_BMFoundDefaultToHarddisk:
                                        ; Within the BOOTCTL.SelectMessage4 find the offset
                                        ; of "%d" which is going to be replaced by the tens
                                        ; and units of the seconds left for the user to
                                        ; decide (%d because the position depends on the NLS
                                        ; translation)
        Lea     DI,[BX].BOOTCTL.SelectMessage4
        Mov     CX,LINESIZE
        Mov     AL,'%'
        Cld
        Repne   Scasb
        .IF     (ZERO?)
            .IF     (BYTE PTR [DI]=='d')
                Lea     AX,[BX].BOOTCTL.SelectMessage4
                Dec     DI
                Sub     DI,AX
                Mov     SecondsOffset,DI
            .ENDIF
        .ENDIF
                                        ; Init control parameters patched in by
                                        ; CDBOOT/CDBOOTPM
        Invoke  Init, ADDR BootControl, ADDR TimerTicks
                                        ; Display instructions for user
        Lea     BX,BootControl
        Invoke  Putstr, ADDR [BX].BOOTCTL.SelectMessage1
        Invoke  Putstr, ADDR CrLf
        Invoke  Putstr, ADDR CrLf
        Invoke  Putstr, ADDR [BX].BOOTCTL.SelectMessage2
        Invoke  Putstr, ADDR CrLf
        Invoke  Putstr, ADDR [BX].BOOTCTL.SelectMessage3
        Invoke  Putstr, ADDR CrLf
        Invoke  Putstr, ADDR CrLf
        Invoke  Putstr, ADDR [BX].BOOTCTL.SelectMessage4
        Invoke  Putstr, Addr Cr
                                        ; Check if the user has pressed a key
                                        ; If that key was one we are looking for
                                        ; exit loop, otherwise wait for the timer
                                        ; to expire
        .REPEAT
            Invoke  Kbhit
            Push    AX
                                        ; Update timer part of instructions
            Lea     BX,BootControl
            Invoke  GetTime, ADDR [BX].BOOTCTL.SelectMessage4
                                        ; If timer expired, then get out of timer
                                        ; loop anyway
            .IF     (AX==0) || (TimerTicks==0)
                Pop     AX
                Mov     AL,KEY_NONE
                .BREAK
            .ENDIF
            Pop     AX
                                        ; If user pressed "1", get out of loop
            .IF     (AL==KEY_1)
                Mov     AL,KEY_1
                .BREAK
            .ENDIF
                                        ; If user pressed "ENTER", get out of loop
            .IF     (AL==KEY_ENTER1) || (AL==KEY_ENTER2)
                Mov     AL,KEY_ENTER1
                .BREAK
            .ENDIF
                                        ; If user pressed "2", get out of loop
            .IF     (AL==KEY_2)
                Mov     AL,KEY_2
                .BREAK
            .ENDIF
                                        ; If user pressed "SPACEBAR", get out of loop
            .IF     (AL==KEY_SPACEBAR)
                Mov     AL,KEY_SPACEBAR
                .BREAK
            .ENDIF
        .UNTIL  (0)
        Lea     BX,BootControl
                                        ; "ENTER" selects default entry, "SPACEBAR"
                                        ; selects the other one. If we by default
                                        ; boot from the first harddisk, then "ENTER"
                                        ; equals to "1" and "SPACEBAR" to "2". If
                                        ; we by default boot from the removeable
                                        ; media, then "ENTER" equals "2" and "SPACEBAR"
                                        ; equals "1"
        .IF     (AL==KEY_ENTER1) || (AL==KEY_SPACEBAR)
            .IF     (AL==KEY_ENTER1)
                Mov     AL,KEY_1
                Mov     AH,KEY_2
            .ELSE
                Mov     AL,KEY_2
                Mov     AH,KEY_1
            .ENDIF
            Test    [BX].BOOTCTL.StatusFlag,STATUS_CID_HARDDISK
            .IF     (ZERO?)
                Xchg    AL,AH
            .ENDIF
        .ENDIF
        Push    AX
                                        ; Empty the line containing the instructions
        Invoke  Putstr, ADDR Cr
        Invoke  Putstr, ADDR Clearline
        Invoke  Putstr, ADDR Cr
        Pop     AX
                                        ; If the user has made no selection, use the
                                        ; selection the user has configured in CD Boot/2
                                        ; during setup/configuration of the CD2BOOT code
        .IF     (AL==KEY_NONE)
            Test    [BX].BOOTCTL.StatusFlag,STATUS_CID_HARDDISK
            .IF     (!ZERO?)
                Mov     AL,KEY_1
            .ELSE
                Mov     AL,KEY_2
            .ENDIF
        .ENDIF
                                        ; If the user has made no selection read partition table
                                        ; of first harddisk, if he has made one continue booting
                                        ; from the removeable media
        .IF     (AL==KEY_1)
            Invoke  Putstr, ADDR [BX].BOOTCTL.BootHarddisk
            Invoke  GetDisk, ADDR OS2BootSec
        .ELSE
                                        ; For booting the removeable media, if a
                                        ; Password is required, get it from the
                                        ; user
            Lea     DI,[BX].BOOTCTL.Password
            .IF     (BYTE PTR [DI]!=0h)
                Invoke  GetPwd, ADDR BootControl
            .ENDIF
            Invoke  Putstr, ADDR [BX].BOOTCTL.BootRemoveable
        .ENDIF
                                        ; Move original OS/2 bootsector of installation
                                        ; diskette (which got patched into CD2BOOT with
                                        ; CDBOOT/CDBOOTPM) to 0000:7C00
        Invoke  MovSec, ADDR OS2BootSec
;M_ExecuteBootCode:
                                        ; Wait 5 Seconds
        Invoke  WaitSec, 3
        Invoke  Putstr, ADDR CrLf
                                        ; Clear screen if requested
        Test    [BX].BOOTCTL.StatusFlag,STATUS_CID_CLEARSCREEN
        .IF     (!ZERO?)
            Invoke  ClrScrn
        .ENDIF
        Mov     AX,0h
        Push    AX
        Mov     AX,07C00h
        Push    AX
        Retf

;----------------------------------------------------------------------------------------
; Wait for reboot. We should not really come here to really.
;----------------------------------------------------------------------------------------
M_WaitForReboot:
        Jmp     M_WaitForReboot

Main    ENDP

;----------------------------------------------------------------------------------------
; Here we have our additional little runtime implementing functions that are useful
;----------------------------------------------------------------------------------------

;*--------------------------------------------------------------------------------------*
;* ClrScrn()                                                                            *
;*          Clear screen by getting current video mode and setting it again             *
;*--------------------------------------------------------------------------------------*
ClrScrn PROC NEAR C USES BX
                                        ; Get current video mode in AL
        Mov     AH,0Fh
        Int     010h
                                        ; Set video mode in AL
        Mov     AH,0h
        Int     010h
        Ret
ClrScrn ENDP

;*--------------------------------------------------------------------------------------*
;* Init(pBootControl, pTimerTicks)                                                      *
;*          Initialize runtime parameters patched into CD2BOOT by CDBOOT/CDBOOTPM       *
;*--------------------------------------------------------------------------------------*
Init    PROC NEAR C USES BX, pBootControl:PTR BYTE, pTimerTicks:PTR WORD
                                        ; Calculate the number of hardware timer
                                        ; ticks the user has requested CD2BOOT
                                        ; to wait for the selection
        Mov     BX,pBootControl
        Mov     AX,[BX].BOOTCTL.Seconds
        XOR     AH,AH
        .IF     (AX!=0)
                                        ; If it is not 0 (the default value)
                                        ; overwrite the default value in pTimerTicks
                                        ; As we're not using floating points (but
                                        ; round down) add 1 second and then subtract
                                        ; 1 tick
            Inc     AL
            Mov     BL,TICKSPERSECOND
            Mul     BL
            Dec     AX
            Mov     BX,pTimerTicks
            Mov     [BX],AX
        .ENDIF
        Ret
Init    ENDP

;*--------------------------------------------------------------------------------------*
;* Kbhit()                                                                              *
;*          Check if a key is available from the keyboard. If none is found return 0    *
;*          in AX else return ASCII and Scan code in AX                                 *
;*--------------------------------------------------------------------------------------*
Kbhit   PROC NEAR C USES BX
                                        ; Get extended Keyboard status into ZF
        Mov     AH,011h
        Int     016h
        .IF     (ZERO?)
            Mov     AX,0h
        .ELSE
                                        ; Get keys as long there are some available
                                        ; while testing for another key save last
                                        ; key read into BX
            .REPEAT
                Mov     AH,010h
                Int     016h
                Mov     BX,AX
                Mov     AH,011h
                Int     016h
            .UNTIL  (ZERO?)
            Mov AX,BX
        .ENDIF
        Ret
KbHit   ENDP

;*--------------------------------------------------------------------------------------*
;* GetPwd(pBootControl)                                                                 *
;*          Inform the user that a password is required, get the keys typed, echo       *
;*          them when the typed key was valid, compare it with the encrypted password   *
;*          passed by.                                                                  *
;*          This call does not return until a valid password has been given             *
;*--------------------------------------------------------------------------------------*
GetPwd  PROC NEAR C USES BX CX SI DI, pBootControl:PTR BYTE
        LOCAL   sError:WORD,            ; sErrorSet to TRUE/FALSE is password mismatched/matched
                sIndex:WORD             ; Index into password entered

                                        ; Inform the user that a password is
                                        ; required
        Mov     SI,pBootControl
        Invoke  Putstr, ADDR [SI].BOOTCTL.PasswordEnter
        .REPEAT
                                        ; Initialize local variables
            Mov     sError,FALSE
            Mov     sIndex,0h
            .REPEAT
                                        ; If the password has no more chars and
                                        ; all previous chars specified were valid,
                                        ; exit loop, otherwise the user has to key
                                        ; up to the maximum number of chars in the
                                        ; password to give no hint about it
                Lea     BX,[SI].BOOTCTL.Password
                Mov     DI,sIndex
                .IF     (sError==FALSE) && (BYTE PTR [BX+DI]==0h)
                    .BREAK
                .ENDIF
                                        ; Get next key until it is valid in the range
                                        ; of allowed keys (0...9, A...Z, a...z), and echo
                                        ; it
                .REPEAT
                    Mov     AH,010h
                    Int     016h
                    .IF     (((AL>='0') && (AL<='9')) || ((AL>='A') && (AL<='Z')) || ((AL>='a') && (AL<='z')))
                    .BREAK
                    .ENDIF
                .UNTIL  (0)
                Push    AX
                Mov     AL,'*'
                Mov     BX,0h
                Mov     CX,1h
                Mov     AH,0Eh
                Int     010h
                Pop     AX
                                        ; Decrypt the key passed and move it
                                        ; into our password buffer
                Mov     CX,sIndex
                Test    CX,01h
                .IF     (ZERO?)
                    ROL     AL,CL
                    XOR     AL,03Ch
                .ELSE
                    ROR     AL,CL
                    XOR     AL,0A5h
                .ENDIF
                Lea     BX,[SI].BOOTCTL.Password
                .IF     (BYTE PTR [BX+DI]!=AL)
                    Mov     sError,TRUE
                .ENDIF
                                        ; Next character
                Add     sIndex,01h
            .UNTIL  (sIndex==PWDSIZE)
                                        ; Now that we have got a password from the
                                        ; user, see if it was a valid one to return
                                        ; to our caller, otherwise clear the current
                                        ; line to inform the user that the password
                                        ; was invalid and give it another try
            Invoke  Putstr, ADDR Cr
            Invoke  Putstr, ADDR Clearline
            Invoke  Putstr, ADDR Cr
            .IF     (sError==FALSE)
                .BREAK
            .ELSE
                Invoke  Putstr, ADDR [SI].BOOTCTL.PasswordInvalid
            .ENDIF
        .UNTIL  (0)
        Ret
GetPwd  ENDP

;*--------------------------------------------------------------------------------------*
;* GetTime(pbString)                                                                    *
;*          Check if the timer has updated the time and update the timer part of the    *
;*          instructions accordingly. In AX return the number of seconds left for the   *
;*          user to decide                                                              *
;*--------------------------------------------------------------------------------------*
GetTime PROC NEAR C USES BX DX DI, pbString:PTR BYTE
                                        ; Load current time and calculate the number of
                                        ; seconds left, quotient goes int AX, remainder
                                        ; into DX
        Xor     DX,DX
        Mov     AX,TimerTicks
        Mov     BX,TICKSPERSECOND
        Div     BX
        Mov     SecondsCurrent, AX
                                        ; Convert the seconds in AX into 2 decimal digits
                                        ; then convert them to ASCII and feed them into
                                        ; the buffer
        Mov     BX,0Ah
        Div     BL
        Xor     AX,'00'
        Mov     DI,pbString
        Add     DI,SecondsOffset
        Mov     [DI],AL
        Mov     [DI+1],AH
                                        ; If the number of seconds left has changed, update
                                        ; instructions for user
        Mov     AX,SecondsCurrent
        Mov     BX,SecondsPrev
        .IF     (AX!=BX)
            Mov     SecondsPrev,AX
            Invoke  Putstr, ADDR Cr
            Invoke  Putstr, pbString
        .ENDIF
                                        ; Check if the free running hardware timer has updated
                                        ; its ticks in the BIOS data area. As we only look for
                                        ; the changes (and as we're polling without doing
                                        ; anything else we're not likely missing a tick at the
                                        ; frequency of 18.2 ticks/s) we don't need to take the
                                        ; upper word and the timer overflow byte into account
        Cli
        Push    ES
        Mov     AX,040h
        Mov     ES,AX
        Mov     AX,ES:[TICKSBIOS]
        Mov     BX,BIOSTimerPrev
        Pop     ES
        Sti
        .IF     (AX!=BX)
            Dec     TimerTicks
            Mov     BIOSTimerPrev,AX
        .ENDIF
                                        ; Return seconds left
        Mov     AX,SecondsCurrent
        Ret
GetTime ENDP

;*--------------------------------------------------------------------------------------*
;* WaitSec(ubSeconds)                                                                   *
;*          Wait a specified number of seconds                                          *
;*                                                                                      *
;*                                                                                      *
;*--------------------------------------------------------------------------------------*
WaitSec PROC NEAR C USES BX CX, ubSeconds:BYTE
                                        ; Calculate the number of ticks equivalent
                                        ; to the seconds count passed by
        XOR     AX,AX
        Mov     AL,ubSeconds;
        Mov     BL,TICKSPERSECOND
        Mul     BL
        Mov     CX,AX
                                        ; Address the free running hardware timer
                                        ; and wait for the passed number of seconds to
                                        ; exceed
        Cli
        Push    ES
        Mov     AX,040h
        Mov     ES,AX
        Sti
                                        ; Loop for the ticks to pass by
        Mov     BX,ES:[TICKSBIOS]
        .REPEAT
            Mov     AX,ES:[TICKSBIOS]
            .IF     (AX!=BX)
                .IF     (CX==0)
                    .BREAK
                .ENDIF
                Dec     CX
                Mov     BX,AX
            .ENDIF
        .UNTIL  (0)
        Cli
        Pop     ES
        Sti
        Ret
WaitSec ENDP

;*--------------------------------------------------------------------------------------*
;* MoveSec(pbSector)                                                                    *
;*          Move original boot sector at pbSector to 0000:7C00                          *
;*--------------------------------------------------------------------------------------*
BOOT_SEGMENT                EQU         00000h
BOOT_OFFSET                 EQU         07C00h

MovSec  PROC NEAR C USES AX CX ES SI DI, pbSector:PTR BYTE
        Mov     SI,pbSector
IFDEF   DEBUG
        Mov     DI,offset TestMove
ELSE
        Mov     DI,BOOT_OFFSET
        Mov     AX,BOOT_SEGMENT
        Cli
        Mov     ES,AX
        Sti
ENDIF
        Mov     CX,OS2BootSecSize
        Shr     CX,01h
        Cld
        Rep     MOVSW
        Ret
MovSec  ENDP

;*--------------------------------------------------------------------------------------*
;* GetDisk(pbSector)                                                                    *
;*          Load partition table of first harddrive at pbSector                         *
;*--------------------------------------------------------------------------------------*
HARDDISK_1                  EQU         080h

GetDisk PROC NEAR C USES BX CX DX ES, pbSector:PTR BYTE
                                        ; Reset first harddisk
        Mov     AH,0h
        Mov     DL,HARDDISK_1
        Int     013h
        .IF     !CARRY?
                                        ; If reset worked, read in the partition
                                        ; table
            Mov     AX,0201h
            Mov     DX,HARDDISK_1
            Mov     CX,01h
            Mov     BX,pbSector
            Int     013h
        .ENDIF
        .IF     CARRY?
                                        ; If reset or reading failed display error message
            Lea     BX,BootControl
            Invoke  Putstr, ADDR [BX].BOOTCTL.SYS02025
            Invoke  Putstr, ADDR CrLf
            Invoke  Putstr, ADDR [BX].BOOTCTL.SYS02027
            Invoke  Putstr, ADDR CrLf
                                        ; On error return 1
            Mov     AX,01h
        .ELSE
                                        ; If no error, return 0
            Mov     AX,0h
        .ENDIF
        Ret
GetDisk ENDP

;----------------------------------------------------------------------------------------
; Here we replace a 512 Byte buffer with the OS/2 boot sector loaded
; from a bootable diskette
; For debugging purposes move boot sector into a test buffer instead at
; 0000:7C00 to prevent overwriting the DOS kernel itself (and crash it)
;----------------------------------------------------------------------------------------
        ORG     2048
                                        ; Buffer to load first sector
OS2BootSec      DB 64 DUP("CD2BOOT ")
OS2BootSecSize  EQU $-OS2BootSec

IFDEF   DEBUG
TestMove        DB  512 DUP("?")
ENDIF   ; DEBUG

;----------------------------------------------------------------------------------------
; Here we define additional messages
;----------------------------------------------------------------------------------------
Cr              DB  0Dh, 0h
CrLf            DB  0Dh, 0Ah, 0h
                DB  0h
ClearLine       DB  79 DUP (" "), 0h
                                        ; BLDLEVEL string
Description     DB  "$@#(C) Roman_Stangl@at.ibm.com:2.00 (05, 2002) (CD2BOOT)#@CDBoot/2", 0h
                DB  0h

;----------------------------------------------------------------------------------------
; Here we define our global variables
;----------------------------------------------------------------------------------------
TICKSPERSECOND  EQU 18                  ; BIOS initilized timer hardware ticks per second (18.2/s)
TICKSBIOS       EQU 06Ch                ; Offset of DWord of timer ticks of free running hardware
                                        ; timer in BIOS data area ticking TICKSPERSECOND
TimerTicks      DW  (18*11-1)           ; Our timer ticking at TICKSPERSECOND counting down from n
                                        ; seconds (n+1 seconds minus 1 ticks as we don't do floating
                                        ; point calculations) to 0 getting decremented in GetTime()
                                        ; whenever the free running hardware timer in the BIOS data
                                        ; area has changed (due to our fast polling we'll lose next to
                                        ; 0 updates)
BIOSTimerPrev   DW  0h                  ; Previous hardware timer ticks in BIOS data area
SecondsCurrent  DW  0h                  ; Number of seconds (no fraction) of current ticks count
SecondsPrev     DW  0h                  ; Number of seconds (no fraction) of previous ticks count
                                        ; Offset within BOOTCTL.SelectMessage of %d which is going
                                        ; to be replaced by the tens and units of the seconds left
SecondsOffset   DW  0h
                                        ; CD2BOOT controls structure (containing some values only,
                                        ; as the assembler doesn't seem to like other initializers),
                                        ; patched by CDBOOT/CDBOOTPM according to the user preferences
BootControl     BOOTCTL < "@CD2BOOT V2.0c@", 0h, 0h, 0Ah >

;----------------------------------------------------------------------------------------
; Here we define our local heap (that is we use the storage after where OS2BOOT
; is loaded as a temporary storage work area)
;----------------------------------------------------------------------------------------
                                        ; Load partition table hereafter
PartitionTable  PARTITIONTBL 0 DUP (< >)
CSEG    ENDS

;----------------------------------------------------------------------------------------
; For some reasons ALIGN does not work with MASM 6.0 (that is, the data following that 
; statement is not ; aligned as it should be), we add a dummy segment ; which is aligned 
; due to the SEGMENT statement using ; a DWORD alignment.
;----------------------------------------------------------------------------------------
DUMMY   SEGMENT DWORD PUBLIC 'CODE'
;----------------------------------------------------------------------------------------
; Signature, for the checksum calculation the file size needs to be a multiple of 4 
; bytes. Unfortunately as ALIGN doesn't work, I have to use a dummy segment.
;----------------------------------------------------------------------------------------
                                        ; Signature for end of the file
EofSignature    DB  055h, 0AAh, 055h, 0AAh
                                        ; Label of end of file
Eof:
DUMMY   ENDS

        END     START
