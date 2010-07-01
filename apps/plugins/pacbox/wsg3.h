/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Pacbox - a Pacman Emulator for Rockbox
 *
 * Based on PIE - Pacman Instructional Emulator
 *
 * Namco custom waveform sound generator 3 (Pacman hardware)
 *
 * Copyright (c) 2003,2004 Alessandro Scotti
 * http://www.ascotti.org/
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
#ifndef WSG3_H
#define WSG3_H

/**
    Namco 3-channel sound generator voice properties.    

    This information is only needed by applications that want to do their own
    sound rendering, as the playSound() function already plays and mixes all
    three voices.

    @see PacmanMachine::playSound
*/
struct wsg3_voice
{
    /** Volume (from 0 to 15) */
    unsigned volume;
    /** Index into the 4-bit 32-entry waveform table (0 to 7) */
    unsigned waveform; 
    /** Frequency */
    unsigned frequency;
};


struct wsg3
{
    unsigned master_clock;
    unsigned sampling_rate;
    unsigned char sound_regs[0x20];
    unsigned char sound_prom[32*8];
    unsigned resample_step;
    unsigned wave_offset[3];
    int sound_wave_data[32*8];
};

extern struct wsg3 wsg3;

/**
    Constructor.

    @param masterClock clock frequency of sound chip (in Hertz)

    @see #wsg3_play_sound
*/
void wsg3_init(unsigned master_clock);

/**
    Sets the 256 byte PROM that contains the waveform table used by the sound chip.
*/
void wsg3_set_sound_prom( const unsigned char * prom );

/**
    Sets the value of the specified register.
*/
static inline void wsg3_set_register(unsigned reg, unsigned char value)
    { wsg3.sound_regs[reg] = value; }

/**
    Returns the value of the specified register.
*/
static inline unsigned char wsg3_get_register(unsigned reg)
    { return wsg3.sound_regs[reg]; }

/**
    Reproduces the sound that is currently being generated by the sound
    chip into the specified buffer.

    The sound chip has three independent voices that generate 8-bit signed
    PCM audio. This function resamples the voices at the currently specified
    sampling rate and mixes them into the output buffer. The output buffer
    can be converted to 8-bit (signed) PCM by dividing each sample by 3 (since 
    there are three voices) or it can be expanded to 16-bit by multiplying
    each sample by 85 (i.e. 256 divided by 3). If necessary, it is possible
    to approximate these values with 4 and 64 in order to use arithmetic
    shifts that are usually faster to execute.

    Note: this function does not clear the content of the output buffer before
    mixing voices into it.

    @param buf pointer to sound buffer that receives the audio samples
    @param len length of the sound buffer
*/
void wsg3_play_sound(int * buf, int len);

/**
    Returns the sampling rate currently in use for rendering sound.
*/
static inline unsigned wsg3_get_sampling_rate(void)
    { return wsg3.sampling_rate; }

/**
    Sets the output sampling rate for playSound().

    @param samplingRate sampling rate in Hertz (samples per second)
*/
void wsg3_set_sampling_rate(unsigned sampling_rate);

#endif /* WSG3_H */
