/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * Module: rbutil
 * File: irivertools.h
 *
 * Copyright (C) 2007 Dominik Wenger
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/


#ifndef IRIVERTOOLS_H_INCLUDED
#define IRIVERTOOLS_H_INCLUDED

#include <QtCore>

#include "md5sum.h"

#define ESTF_SIZE 32

struct sumpairs {
    const char *unpatched;
    const char *patched;
};


enum striptype
{
    STRIP_NONE,
    STRIP_HEADER_CHECKSUM,
    STRIP_HEADER_CHECKSUM_ESTF
};

/* protos for iriver.c */

int intable(char *md5, struct sumpairs *table, int len);

int mkboot(QString infile, QString outfile,QString bootloader,int origin);
int iriver_decode(QString infile_name, QString outfile_name, unsigned int modify,
                  enum striptype stripmode);
int iriver_encode(QString infile_name, QString outfile_name, unsigned int modify);

#endif // IRIVERTOOLS_H_INCLUDED
