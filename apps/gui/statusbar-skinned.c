/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Thomas Martitz
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

#include "config.h"

#include "system.h"
#include "settings.h"
#include "appevents.h"
#include "screens.h"
#include "screen_access.h"
#include "skin_engine/skin_engine.h"
#include "skin_engine/wps_internals.h"
#include "viewport.h"
#include "statusbar.h"
#include "statusbar-skinned.h"
#include "debug.h"


/* currently only one wps_state is needed */
extern struct wps_state     wps_state; /* from wps.c */
static struct gui_wps       sb_skin[NB_SCREENS]      = {{ .data = NULL }};
static struct wps_data      sb_skin_data[NB_SCREENS] = {{ .wps_loaded = 0 }};

/* initial setup of wps_data  */
static void sb_skin_update(void*);
static bool loaded_ok[NB_SCREENS] = { false };
static int skinbars = 0;
static int update_delay = DEFAULT_UPDATE_DELAY;


void sb_skin_data_load(enum screen_type screen, const char *buf, bool isfile)
{
    struct wps_data *data = sb_skin[screen].data;

    int success;
    success = buf && skin_data_load(data, buf, isfile);

    if (success)
    {  /* hide the sb's default viewport because it has nasty effect with stuff
        * not part of the statusbar,
        * hence .sbs's without any other vps are unsupported*/
        struct skin_viewport *vp = find_viewport(VP_DEFAULT_LABEL, data);
        struct skin_token_list *next_vp = data->viewports->next;

        if (!next_vp)
        {    /* no second viewport, let parsing fail */
            success = false;
        }
        /* hide this viewport, forever */
        vp->hidden_flags = VP_NEVER_VISIBLE;
    }

    if (!success)
        remove_event(GUI_EVENT_ACTIONUPDATE, sb_skin_update);

#ifdef HAVE_REMOVE_LCD
    data->remote_wps = !(screen == SCREEN_MAIN);
#endif
    loaded_ok[screen] = success;
}

struct viewport *sb_skin_get_info_vp(enum screen_type screen)
{
    return &find_viewport(VP_INFO_LABEL, sb_skin[screen].data)->vp;
}

inline bool sb_skin_get_state(enum screen_type screen)
{
    return loaded_ok[screen] && (skinbars & VP_SB_ONSCREEN(screen));
}

void sb_skin_set_state(int state, enum screen_type screen)
{
    sb_skin[screen].state->do_full_update = true;
    if (state)
    {
        skinbars |= VP_SB_ONSCREEN(screen);
    }
    else
    {
        skinbars &= ~VP_SB_ONSCREEN(screen);
    }

    if (skinbars)
        add_event(GUI_EVENT_ACTIONUPDATE, false, sb_skin_update);
    else
        remove_event(GUI_EVENT_ACTIONUPDATE, sb_skin_update);
}

static void sb_skin_update(void* param)
{
    static long next_update = 0;
    int i;
    int forced_draw = param ||  sb_skin[SCREEN_MAIN].state->do_full_update;
    if (TIME_AFTER(current_tick, next_update) || forced_draw)
    {
        FOR_NB_SCREENS(i)
        {
            if (sb_skin_get_state(i))
            {
                skin_update(&sb_skin[i], forced_draw?
                        WPS_REFRESH_ALL : WPS_REFRESH_NON_STATIC);
            }
        }
        next_update = current_tick + update_delay; /* don't update too often */
        sb_skin[SCREEN_MAIN].state->do_full_update = false;
    }
}

void sb_skin_set_update_delay(int delay)
{
    update_delay = delay;
}

void sb_skin_init(void)
{
    int i;
    FOR_NB_SCREENS(i)
    {
#ifdef HAVE_ALBUMART
        sb_skin_data[i].albumart = NULL;
        sb_skin_data[i].playback_aa_slot = -1;
#endif
#ifdef HAVE_REMOTE_LCD
        sb_skin_data[i].remote_wps = (i == SCREEN_REMOTE);
#endif
        sb_skin[i].data = &sb_skin_data[i];
        sb_skin[i].display = &screens[i];
        /* Currently no seperate wps_state needed/possible
           so use the only available ( "global" ) one */
        sb_skin[i].state = &wps_state;
        sb_skin[i].statusbars = &skinbars;
    }
}