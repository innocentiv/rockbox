/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Robert E. Hak
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdbool.h>

#include "lcd.h"
#include "font.h"
#include "backlight.h"
#include "menu.h"
#include "button.h"
#include "kernel.h"
#include "debug.h"
#include "usb.h"
#include "panic.h"
#include "settings.h"
#include "status.h"
#include "screens.h"

#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#include "widgets.h"
#endif

struct menu {
    int top;
    int cursor;
    struct menu_items* items;
    int itemcount;
};

#define MAX_MENUS 4

#ifdef HAVE_LCD_BITMAP

/* pixel margins */
#define MARGIN_X (global_settings.scrollbar && \
                  menu_lines < menus[m].itemcount ? SCROLLBAR_WIDTH : 0) +\
                  CURSOR_WIDTH
#define MARGIN_Y (global_settings.statusbar ? STATUSBAR_HEIGHT : 0)

/* position the entry-list starts at */
#define LINE_X   0
#define LINE_Y   (global_settings.statusbar ? 1 : 0)

#define CURSOR_X (global_settings.scrollbar && \
                  menu_lines < menus[m].itemcount ? 1 : 0)
#define CURSOR_Y 0 /* the cursor is not positioned in regard to
                      the margins, so this is the amount of lines
                      we add to the cursor Y position to position
                      it on a line */
#define CURSOR_WIDTH  4

#define SCROLLBAR_X      0
#define SCROLLBAR_Y      lcd_getymargin()
#define SCROLLBAR_WIDTH  6

#else /* HAVE_LCD_BITMAP */

#define LINE_X      1 /* X position the entry-list starts at */

#define MENU_LINES 2

#define CURSOR_X    0
#define CURSOR_Y    0 /* not really used for players */

#endif /* HAVE_LCD_BITMAP */

#ifdef HAVE_NEW_CHARCELL_LCD
#define CURSOR_CHAR 0x7e
#else
#define CURSOR_CHAR 0x89
#endif

static struct menu menus[MAX_MENUS];
static bool inuse[MAX_MENUS] = { false };

/* count in letter positions, NOT pixels */
void put_cursorxy(int x, int y, bool on)
{
#ifdef HAVE_LCD_BITMAP
    int fh, fw;
    int xpos, ypos;
    lcd_getstringsize("A", &fw, &fh);
    xpos = x*6;
    ypos = y*fh + lcd_getymargin();
    if ( fh > 8 )
        ypos += (fh - 8) / 2;
#endif

    /* place the cursor */
    if(on) {
#ifdef HAVE_LCD_BITMAP
        lcd_bitmap ( bitmap_icons_6x8[Cursor], 
                     xpos, ypos, 4, 8, true);
#else
        lcd_putc(x, y, CURSOR_CHAR);
#endif
    }
    else {
#if defined(HAVE_LCD_BITMAP)
        /* I use xy here since it needs to disregard the margins */
        lcd_clearrect (xpos, ypos, 4, 8);
#else
        lcd_putc(x, y, ' ');
#endif
    }
}

static void menu_draw(int m)
{
    int i = 0;
#ifdef HAVE_LCD_BITMAP
    int fw, fh;
    int menu_lines;
    lcd_setfont(FONT_UI);
    lcd_getstringsize("A", &fw, &fh);
    if (global_settings.statusbar)
        menu_lines = (LCD_HEIGHT - STATUSBAR_HEIGHT) / fh;
    else
        menu_lines = LCD_HEIGHT/fh;
#else
    int menu_lines = MENU_LINES;
#endif

    lcd_scroll_pause();  /* halt scroll first... */
    lcd_clear_display(); /* ...then clean the screen */
#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(MARGIN_X,MARGIN_Y); /* leave room for cursor and icon */
#endif
    /* correct cursor pos if out of screen */
    if (menus[m].cursor - menus[m].top >= menu_lines)
        menus[m].top++;

    for (i = menus[m].top; 
         (i < menus[m].itemcount) && (i<menus[m].top+menu_lines);
         i++) {
        if((menus[m].cursor - menus[m].top)==(i-menus[m].top))
            lcd_puts_scroll(LINE_X, i-menus[m].top, menus[m].items[i].desc);
        else
            lcd_puts(LINE_X, i-menus[m].top, menus[m].items[i].desc);
    }

    /* place the cursor */
    put_cursorxy(CURSOR_X, menus[m].cursor - menus[m].top, true);
#ifdef HAVE_LCD_BITMAP
    if (global_settings.scrollbar && menus[m].itemcount > menu_lines) 
        scrollbar(SCROLLBAR_X, SCROLLBAR_Y, SCROLLBAR_WIDTH - 1,
                  LCD_HEIGHT - SCROLLBAR_Y, menus[m].itemcount, menus[m].top,
                  menus[m].top + menu_lines, VERTICAL);
#endif
    status_draw();
    lcd_update();
}

/* 
 * Move the cursor to a particular id, 
 *   target: where you want it to be 
 */
static void put_cursor(int m, int target)
{
    bool do_update = true;
#ifdef HAVE_LCD_BITMAP
    int fw, fh;
    int menu_lines;
    lcd_getstringsize("A", &fw, &fh);
    if (global_settings.statusbar)
        menu_lines = (LCD_HEIGHT - STATUSBAR_HEIGHT) / fh;
    else
        menu_lines = LCD_HEIGHT/fh;
#else
    int menu_lines = MENU_LINES;
#endif

    put_cursorxy(CURSOR_X, menus[m].cursor - menus[m].top, false);
    menus[m].cursor = target;
    menu_draw(m);

    if ( target < menus[m].top ) {
        menus[m].top--;
        menu_draw(m);
        do_update = false;
    }
    else if ( target-menus[m].top > menu_lines-1 ) {
        menus[m].top++;
        menu_draw(m);
        do_update = false;
    }

    if (do_update) {
        put_cursorxy(CURSOR_X, menus[m].cursor - menus[m].top, true); 
        lcd_update();
    }

}

int menu_init(struct menu_items* mitems, int count)
{
    int i;

    for ( i=0; i<MAX_MENUS; i++ ) {
        if ( !inuse[i] ) {
            inuse[i] = true;
            break;
        }
    }
    if ( i == MAX_MENUS ) {
        DEBUGF("Out of menus!\n");
        return -1;
    }
    menus[i].items = mitems;
    menus[i].itemcount = count;
    menus[i].top = 0;
    menus[i].cursor = 0;

    return i;
}

void menu_exit(int m)
{
    inuse[m] = false;
}

bool menu_run(int m)
{
    bool exit = false;

    menu_draw(m);

    while (!exit) {
        switch( button_get_w_tmo(HZ/2) ) {
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_UP:
            case BUTTON_UP | BUTTON_REPEAT:
#else
            case BUTTON_LEFT:
            case BUTTON_LEFT | BUTTON_REPEAT:
#endif
                if (menus[m].cursor) {
                    /* move up */
                    put_cursor(m, menus[m].cursor-1);
                }
                else {
                    /* move to bottom */
#ifdef HAVE_RECORDER_KEYPAD
                    menus[m].top = menus[m].itemcount-9;
#else
                    menus[m].top = menus[m].itemcount-3;
#endif
                    if (menus[m].top < 0)
                        menus[m].top = 0;
                    menus[m].cursor = menus[m].itemcount-1;
                    put_cursor(m, menus[m].itemcount-1);
                }
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_DOWN:
            case BUTTON_DOWN | BUTTON_REPEAT:
#else
            case BUTTON_RIGHT:
            case BUTTON_RIGHT | BUTTON_REPEAT:
#endif
                if (menus[m].cursor < menus[m].itemcount-1) {
                    /* move down */
                    put_cursor(m, menus[m].cursor+1);
                }
                else {
                    /* move to top */
                    menus[m].top = 0;
                    menus[m].cursor = 0;
                    put_cursor(m, 0);
                }
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_RIGHT:
#endif
            case BUTTON_PLAY:
                /* Erase current display state */
                lcd_scroll_pause(); /* pause is better than stop when
                                       are gonna clear the screen anyway */
                lcd_clear_display();
            
                /* if a child returns that USB was used, 
                   we return immediately */
                if (menus[m].items[menus[m].cursor].function()) {
                    lcd_scroll_pause(); /* just in case */
                    return true;
                }
            
                /* Return to previous display state */
                menu_draw(m);
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_LEFT:
            case BUTTON_F1:
#else
            case BUTTON_STOP:
            case BUTTON_MENU:
#endif
                lcd_scroll_pause();
                exit = true;
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_F3:
                if (f3_screen())
                    return true;
                menu_draw(m);
                break;
#endif

            case SYS_USB_CONNECTED:
                usb_screen();
#ifdef HAVE_LCD_CHARCELLS
                lcd_icon(ICON_PARAM, false);
#endif
                return true;
        }
        
        status_draw();
        lcd_update();
    }

    return false;
}
