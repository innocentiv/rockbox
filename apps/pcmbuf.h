/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Miika Pekkarinen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef PCMBUF_H
#define PCMBUF_H

/* Commit PCM data */
void *pcmbuf_request_buffer(int *count);
void pcmbuf_write_complete(int count);

/* Init */
size_t pcmbuf_init(unsigned char *bufend);

/* Playback */
void pcmbuf_play_start(void);
void pcmbuf_play_stop(void);
void pcmbuf_pause(bool pause);
void pcmbuf_monitor_track_change(bool monitor);
bool pcmbuf_start_track_change(bool manual_skip);

/* Crossfade */
#ifdef HAVE_CROSSFADE
bool pcmbuf_is_crossfade_active(void);
void pcmbuf_request_crossfade_enable(bool on_off);
bool pcmbuf_is_same_size(void);
#else
/* Dummy functions with sensible returns */
static inline bool pcmbuf_is_crossfade_active(void)
    { return false; }
static inline void pcmbuf_request_crossfade_enable(bool on_off)
    { return; (void)on_off; }
static inline bool pcmbuf_is_same_size(void)
    { return true; }
#endif

/* Voice */
void *pcmbuf_request_voice_buffer(int *count);
void pcmbuf_write_voice_complete(int count);

/* Debug menu, other metrics */
size_t pcmbuf_free(void);
size_t pcmbuf_get_bufsize(void);
int pcmbuf_descs(void);
int pcmbuf_used_descs(void);
#ifdef ROCKBOX_HAS_LOGF
unsigned char *pcmbuf_get_meminfo(size_t *length);
#endif

/* Misc */
void pcmbuf_fade(bool fade, bool in);
void pcmbuf_soft_mode(bool shhh);
bool pcmbuf_is_lowdata(void);
void pcmbuf_set_low_latency(bool state);
unsigned long pcmbuf_get_latency(void);

#endif
