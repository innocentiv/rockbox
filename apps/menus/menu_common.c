/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Dan Everton
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stddef.h>
#include <limits.h>
#include "config.h"
#include "action.h"
#include "menu.h"
#include "menu_common.h"
#if CONFIG_CODEC == SWCODEC
#include "pcmbuf.h"
#endif

#if CONFIG_CODEC == SWCODEC
/* Use this callback if your menu adjusts DSP settings. */
int lowlatency_callback(int action, const struct menu_item_ex *this_item)
{
    (void)this_item;
    switch (action)
    {
        case ACTION_ENTER_MENUITEM: /* on entering an item */
            pcmbuf_set_low_latency(true);
            break;
        case ACTION_EXIT_MENUITEM: /* on exit */
            pcmbuf_set_low_latency(false);
            break;
    }
    return action;
}
#endif

