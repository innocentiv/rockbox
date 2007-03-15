/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Dave Chapman
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

static off_t filesize(int fd)
{
    struct stat buf;

    fstat(fd,&buf);
    return buf.st_size;
}

static int write_cfile(unsigned char* buf, off_t len, char* cname)
{
    char filename[256];
    FILE* fp;
    int i;

    snprintf(filename,256,"%s.c",cname);

    fp = fopen(filename,"w+");
    if (fp == NULL) {
        fprintf(stderr,"Couldn't open %s\n",filename);
        return -1;
    }

    fprintf(fp,"/* Generated by ipod2c */\n\n");
    fprintf(fp,"unsigned char %s[] = {",cname);

    for (i=0;i<len;i++) {
        if ((i % 16) == 0) {
           fprintf(fp,"\n    ");
        }
        if (i == (len-1)) {
            fprintf(fp,"0x%02x",buf[i]);
        } else {
            fprintf(fp,"0x%02x, ",buf[i]);
        }
    }
    fprintf(fp,"\n};\n");

    fclose(fp);
    return 0;
}

static int write_hfile(unsigned char* buf, off_t len, char* cname)
{
    char filename[256];
    FILE* fp;

    snprintf(filename,256,"%s.h",cname);
    fp = fopen(filename,"w+");
    if (fp == NULL) {
        fprintf(stderr,"Couldn't open %s\n",filename);
        return -1;
    }

    fprintf(fp,"/* Generated by ipod2c */\n\n");
    fprintf(fp,"#define LEN_%s %d\n",cname,(int)len);
    fprintf(fp,"extern unsigned char %s[];\n",cname);
    fclose(fp);
    return 0;
}

int main (int argc, char* argv[])
{
    char* infile;
    char* cname;
    int fd;
    unsigned char* buf;
    int len;
    int n;

    if (argc != 3) {
        fprintf(stderr,"Usage: bin2c file cname\n");
        return 0;
    }

    infile=argv[1];
    cname=argv[2];

    fd = open(infile,O_RDONLY|O_BINARY);
    if (fd < 0) {
        fprintf(stderr,"Can not open %s\n",infile);
        return 0;
    }

    len = filesize(fd);

    buf = malloc(len);
    n = read(fd,buf,len);
    if (n < len) {
        fprintf(stderr,"Short read, aborting\n");
        return 0;
    }
    close(fd);

    if (write_cfile(buf,len,cname) < 0) {
        return -1;
    }
    if (write_hfile(buf,len,cname) < 0) {
        return -1;
    }

    return 0;
}
