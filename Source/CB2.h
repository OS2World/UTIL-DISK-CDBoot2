#ifndef _CB2_H_
#define _CB2_H_

/***********************************************************************\
 *                              CDBoot/2                               *
 *              Copyright (C) by Stangl Roman, 2000, 2002              *
 * This Code may be freely distributed, provided the Copyright isn't   *
 * removed, under the conditions indicated in the documentation.       *
 *                                                                     *
 * CD2.h        Dialog control definitions                             *
 *                                                                     *
\***********************************************************************/

                                        /* As the dialog editor does not want to include
                                           *.hpp files (only *.h files) we have to outsource
                                           them here */
#define     CB2_DIALOG                  100
#define     CDGRP_CDBOOT2               101

#define     CDGRP_SETTINGS              120
#define     CDST_DRIVE                  121
#define     CDCBX_DRIVE                 122
#define     CDST_CD2BOOT                123
#define     CDEF_CD2BOOT                124
#define     CDST_SECONDS                125
#define     CDEF_SECONDS                126
#define     CDHSB_SECONDS               127
#define     CDRB_DEFAULTHARDDISK        128
#define     CDRB_DEFAULTREMOVEABLE      129
#define     CDCB_CLEARSCREEN            130

#define     CDGRP_PASSWORD              140
#define     CDCB_PASSWORD               141
#define     CDEF_PASSWORD               142

#define     CDGRP_ADVANCED              150
#define     CDCB_BMBOOT                 151

#define     CDGRP_COPYRIGHT             160
#define     CDST_COPYRIGHT_COLOR        161
#define     CDCBX_COPYRIGHT_COLOR       162
#define     CDEF_COPYRIGHT1             163
#define     CDEF_COPYRIGHT2             164
#define     CDEF_COPYRIGHT3             165

#define     CDGRP_CUSTOMMESSAGE         170
#define     CDST_CUSTOMMESSAGE_COLOR    171
#define     CDCBX_CUSTOMMESSAGE_COLOR   172
#define     CDEF_CUSTOMMESSAGE1         173
#define     CDEF_CUSTOMMESSAGE2         174
#define     CDEF_CUSTOMMESSAGE3         175

#endif  /* _CB2_H_ */
