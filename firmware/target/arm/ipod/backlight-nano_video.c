/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include <stdlib.h>
#include "cpu.h"
#include "kernel.h"
#include "thread.h"
#include "i2c.h"
#include "debug.h"
#include "rtc.h"
#include "usb.h"
#include "power.h"
#include "system.h"
#include "button.h"
#include "timer.h"
#include "backlight.h"

inline void __backlight_on(void)
{
    /* set port B03 on */
    outl(((0x100 | 1) << 3), 0x6000d824);

    /* set port L07 on */
    GPIOL_OUTPUT_VAL = ((0x100 | 1) << 7);
}

inline void __backlight_off(void)
{
    /* set port B03 off */
    outl(((0x100 | 0) << 3), 0x6000d824);

    /* set port L07 off */
    GPIOL_OUTPUT_VAL = ((0x100 | 0) << 7);
}
