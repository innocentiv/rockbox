/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Nicolas Pennequin, Dan Everton, Matthias Mohr
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "atoi.h"
#include "gwps.h"
#ifndef __PCTOOL__
#include "settings.h"
#include "debug.h"
#include "plugin.h"

#ifdef HAVE_LCD_BITMAP
#include "bmp.h"
#endif

#if (LCD_DEPTH > 1) || (defined(HAVE_LCD_REMOTE) && (LCD_REMOTE_DEPTH > 1))
#include "backdrop.h"
#endif

#endif

#define WPS_DEFAULTCFG WPS_DIR "/rockbox_default.wps"
#define RWPS_DEFAULTCFG WPS_DIR "/rockbox_default.rwps"

#define PARSE_FAIL_UNCLOSED_COND     1
#define PARSE_FAIL_INVALID_CHAR      2
#define PARSE_FAIL_COND_SYNTAX_ERROR 3

/* level of current conditional.
   -1 means we're not in a conditional. */
static int level = -1;

/* index of the last WPS_TOKEN_CONDITIONAL_OPTION
    or WPS_TOKEN_CONDITIONAL_START in current level */
static int lastcond[WPS_MAX_COND_LEVEL];

/* index of the WPS_TOKEN_CONDITIONAL in current level */
static int condindex[WPS_MAX_COND_LEVEL];

/* number of condtional options in current level */
static int numoptions[WPS_MAX_COND_LEVEL];

/* the current line in the file */
static int line;

#ifdef HAVE_LCD_BITMAP

#if LCD_DEPTH > 1
#define MAX_BITMAPS MAX_IMAGES+2 /* WPS images + pbar bitmap + backdrop */
#else
#define MAX_BITMAPS MAX_IMAGES+1 /* WPS images + pbar bitmap */
#endif

#define PROGRESSBAR_BMP MAX_IMAGES
#define BACKDROP_BMP    MAX_IMAGES+1

/* pointers to the bitmap filenames in the WPS source */
static const char *bmp_names[MAX_BITMAPS];

#endif /* HAVE_LCD_BITMAP */

#ifdef DEBUG
/* debugging function */
extern void print_debug_info(struct wps_data *data, int fail, int line);
#endif

static void wps_reset(struct wps_data *data);

/* Function for parsing of details for a token. At the moment the
   function is called, the token type has already been set. The
   function must fill in the details and possibly add more tokens
   to the token array. It should return the number of chars that
   has been consumed.

   wps_bufptr points to the char following the tag (i.e. where
   details begin).
   token is the pointer to the 'main' token being parsed
   */
typedef int (*wps_tag_parse_func)(const char *wps_bufptr,
                struct wps_token *token, struct wps_data *wps_data);

struct wps_tag {
    enum wps_token_type type;
    const char name[3];
    unsigned char refresh_type;
    const wps_tag_parse_func parse_func;
};

/* prototypes of all special parse functions : */
static int parse_subline_timeout(const char *wps_bufptr,
        struct wps_token *token, struct wps_data *wps_data);
static int parse_progressbar(const char *wps_bufptr,
        struct wps_token *token, struct wps_data *wps_data);
static int parse_dir_level(const char *wps_bufptr,
        struct wps_token *token, struct wps_data *wps_data);
#ifdef HAVE_LCD_BITMAP
static int parse_image_special(const char *wps_bufptr,
        struct wps_token *token, struct wps_data *wps_data);
static int parse_statusbar_enable(const char *wps_bufptr,
        struct wps_token *token, struct wps_data *wps_data);
static int parse_statusbar_disable(const char *wps_bufptr,
        struct wps_token *token, struct wps_data *wps_data);
static int parse_image_display(const char *wps_bufptr,
        struct wps_token *token, struct wps_data *wps_data);
static int parse_image_load(const char *wps_bufptr,
        struct wps_token *token, struct wps_data *wps_data);
#endif /*HAVE_LCD_BITMAP */

#ifdef CONFIG_RTC
#define WPS_RTC_REFRESH WPS_REFRESH_DYNAMIC
#else
#define WPS_RTC_REFRESH WPS_REFRESH_STATIC
#endif

/* array of available tags - those with more characters have to go first
   (e.g. "xl" and "xd" before "x"). It needs to end with the unknown token. */
static const struct wps_tag all_tags[] = {

    { WPS_TOKEN_ALIGN_CENTER,             "ac",  0,                   NULL },
    { WPS_TOKEN_ALIGN_LEFT,               "al",  0,                   NULL },
    { WPS_TOKEN_ALIGN_RIGHT,              "ar",  0,                   NULL },

    { WPS_TOKEN_BATTERY_PERCENT,          "bl",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_BATTERY_VOLTS,            "bv",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_BATTERY_TIME,             "bt",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_BATTERY_SLEEPTIME,        "bs",  WPS_REFRESH_DYNAMIC, NULL },
#if CONFIG_CHARGING >= CHARGING_MONITOR
    { WPS_TOKEN_BATTERY_CHARGING,         "bc",  WPS_REFRESH_DYNAMIC, NULL },
#endif
#if CONFIG_CHARGING
    { WPS_TOKEN_BATTERY_CHARGER_CONNECTED,"bp",  WPS_REFRESH_DYNAMIC, NULL },
#endif

    { WPS_TOKEN_RTC_DAY_OF_MONTH,             "cd", WPS_RTC_REFRESH, NULL },
    { WPS_TOKEN_RTC_DAY_OF_MONTH_BLANK_PADDED,"ce", WPS_RTC_REFRESH, NULL },
    { WPS_TOKEN_RTC_HOUR_24_ZERO_PADDED,      "cH", WPS_RTC_REFRESH, NULL },
    { WPS_TOKEN_RTC_HOUR_24,                  "ck", WPS_RTC_REFRESH, NULL },
    { WPS_TOKEN_RTC_HOUR_12_ZERO_PADDED,      "cI", WPS_RTC_REFRESH, NULL },
    { WPS_TOKEN_RTC_HOUR_12,                  "cl", WPS_RTC_REFRESH, NULL },
    { WPS_TOKEN_RTC_MONTH,                    "cm", WPS_RTC_REFRESH, NULL },
    { WPS_TOKEN_RTC_MINUTE,                   "cM", WPS_RTC_REFRESH, NULL },
    { WPS_TOKEN_RTC_SECOND,                   "cS", WPS_RTC_REFRESH, NULL },
    { WPS_TOKEN_RTC_YEAR_2_DIGITS,            "cy", WPS_RTC_REFRESH, NULL },
    { WPS_TOKEN_RTC_YEAR_4_DIGITS,            "cY", WPS_RTC_REFRESH, NULL },
    { WPS_TOKEN_RTC_AM_PM_UPPER,              "cP", WPS_RTC_REFRESH, NULL },
    { WPS_TOKEN_RTC_AM_PM_LOWER,              "cp", WPS_RTC_REFRESH, NULL },
    { WPS_TOKEN_RTC_WEEKDAY_NAME,             "ca", WPS_RTC_REFRESH, NULL },
    { WPS_TOKEN_RTC_MONTH_NAME,               "cb", WPS_RTC_REFRESH, NULL },
    { WPS_TOKEN_RTC_DAY_OF_WEEK_START_MON,    "cu", WPS_RTC_REFRESH, NULL },
    { WPS_TOKEN_RTC_DAY_OF_WEEK_START_SUN,    "cw", WPS_RTC_REFRESH, NULL },

    /* current file */
    { WPS_TOKEN_FILE_BITRATE,             "fb",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_FILE_CODEC,               "fc",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_FILE_FREQUENCY,           "ff",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_FILE_FREQUENCY_KHZ,       "fk",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_FILE_NAME_WITH_EXTENSION, "fm",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_FILE_NAME,                "fn",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_FILE_PATH,                "fp",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_FILE_SIZE,                "fs",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_FILE_VBR,                 "fv",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_FILE_DIRECTORY,           "d",   WPS_REFRESH_STATIC,
                                                           parse_dir_level },

    /* next file */
    { WPS_TOKEN_FILE_BITRATE,             "Fb",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_FILE_CODEC,               "Fc",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_FILE_FREQUENCY,           "Ff",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_FILE_FREQUENCY_KHZ,       "Fk",  WPS_REFRESH_DYNAMIC,  NULL },
    { WPS_TOKEN_FILE_NAME_WITH_EXTENSION, "Fm",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_FILE_NAME,                "Fn",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_FILE_PATH,                "Fp",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_FILE_SIZE,                "Fs",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_FILE_VBR,                 "Fv",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_FILE_DIRECTORY,           "D",   WPS_REFRESH_DYNAMIC,
                                                           parse_dir_level },

    /* current metadata */
    { WPS_TOKEN_METADATA_ARTIST,          "ia",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_METADATA_COMPOSER,        "ic",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_METADATA_ALBUM,           "id",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_METADATA_ALBUM_ARTIST,    "iA",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_METADATA_GENRE,           "ig",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_METADATA_DISC_NUMBER,     "ik",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_METADATA_TRACK_NUMBER,    "in",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_METADATA_TRACK_TITLE,     "it",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_METADATA_VERSION,         "iv",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_METADATA_YEAR,            "iy",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_METADATA_COMMENT,         "iC",  WPS_REFRESH_STATIC,  NULL },

    /* next metadata */
    { WPS_TOKEN_METADATA_ARTIST,          "Ia",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_METADATA_COMPOSER,        "Ic",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_METADATA_ALBUM,           "Id",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_METADATA_ALBUM_ARTIST,    "IA",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_METADATA_GENRE,           "Ig",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_METADATA_DISC_NUMBER,     "Ik",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_METADATA_TRACK_NUMBER,    "In",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_METADATA_TRACK_TITLE,     "It",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_METADATA_VERSION,         "Iv",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_METADATA_YEAR,            "Iy",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_METADATA_COMMENT,         "IC",  WPS_REFRESH_DYNAMIC, NULL },

#if (CONFIG_CODEC != MAS3507D)
    { WPS_TOKEN_SOUND_PITCH,              "Sp",  WPS_REFRESH_DYNAMIC, NULL },
#endif

#if (CONFIG_LED == LED_VIRTUAL) || defined(HAVE_REMOTE_LCD)
    { WPS_TOKEN_VLED_HDD,                 "lh",  WPS_REFRESH_DYNAMIC, NULL },
#endif

    { WPS_TOKEN_MAIN_HOLD,                "mh",  WPS_REFRESH_DYNAMIC, NULL },

#ifdef HAS_REMOTE_BUTTON_HOLD
    { WPS_TOKEN_REMOTE_HOLD,              "mr",  WPS_REFRESH_DYNAMIC, NULL },
#endif

    { WPS_TOKEN_REPEAT_MODE,              "mm",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_PLAYBACK_STATUS,          "mp",  WPS_REFRESH_DYNAMIC, NULL },

#ifdef HAVE_LCD_BITMAP
    { WPS_TOKEN_PEAKMETER,                "pm", WPS_REFRESH_PEAK_METER, NULL },
#else
    { WPS_TOKEN_PLAYER_PROGRESSBAR,       "pf",
      WPS_REFRESH_DYNAMIC | WPS_REFRESH_PLAYER_PROGRESS, parse_progressbar },
#endif
    { WPS_TOKEN_PROGRESSBAR,              "pb",  WPS_REFRESH_PLAYER_PROGRESS,
                                                         parse_progressbar },

    { WPS_TOKEN_VOLUME,                   "pv",  WPS_REFRESH_DYNAMIC, NULL },

    { WPS_TOKEN_TRACK_ELAPSED_PERCENT,    "px",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_TRACK_TIME_ELAPSED,       "pc",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_TRACK_TIME_REMAINING,     "pr",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_TRACK_LENGTH,             "pt",  WPS_REFRESH_STATIC,  NULL },

    { WPS_TOKEN_PLAYLIST_POSITION,        "pp",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_PLAYLIST_ENTRIES,         "pe",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_PLAYLIST_NAME,            "pn",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_PLAYLIST_SHUFFLE,         "ps",  WPS_REFRESH_DYNAMIC, NULL },

    { WPS_TOKEN_DATABASE_PLAYCOUNT,       "rp",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_DATABASE_RATING,          "rr",  WPS_REFRESH_DYNAMIC, NULL },
    { WPS_TOKEN_DATABASE_AUTOSCORE,       "ra",  WPS_REFRESH_DYNAMIC, NULL },
#if CONFIG_CODEC == SWCODEC
    { WPS_TOKEN_REPLAYGAIN,               "rg",  WPS_REFRESH_STATIC,  NULL },
    { WPS_TOKEN_CROSSFADE,                "xf",  WPS_REFRESH_DYNAMIC, NULL },
#endif

    { WPS_NO_TOKEN,                       "s",   WPS_REFRESH_SCROLL,  NULL },
    { WPS_TOKEN_SUBLINE_TIMEOUT,          "t",   0,  parse_subline_timeout },

#ifdef HAVE_LCD_BITMAP
    { WPS_NO_TOKEN,                       "we",  0, parse_statusbar_enable },
    { WPS_NO_TOKEN,                       "wd",  0, parse_statusbar_disable },

    { WPS_NO_TOKEN,                       "xl",  0,       parse_image_load },

    { WPS_TOKEN_IMAGE_PRELOAD_DISPLAY,    "xd",  WPS_REFRESH_STATIC,
                                                       parse_image_display },

    { WPS_TOKEN_IMAGE_DISPLAY,            "x",   0,       parse_image_load },
    { WPS_TOKEN_IMAGE_PROGRESS_BAR,       "P",   0,    parse_image_special },
#if (LCD_DEPTH > 1) || (defined(HAVE_LCD_REMOTE) && (LCD_REMOTE_DEPTH > 1))
    { WPS_TOKEN_IMAGE_BACKDROP,           "X",   0,    parse_image_special },
#endif
#endif

    { WPS_TOKEN_UNKNOWN,                  "",    0, NULL }
    /* the array MUST end with an empty string (first char is \0) */
};

/* Returns the number of chars that should be skipped to jump
   immediately after the first eol, i.e. to the start of the next line */
static int skip_end_of_line(const char *wps_bufptr)
{
    line++;
    int skip = 0;
    while(*(wps_bufptr + skip) != '\n')
        skip++;
    return ++skip;
}

/* Starts a new subline in the current line during parsing */
static void wps_start_new_subline(struct wps_data *data)
{
    data->num_sublines++;
    data->sublines[data->num_sublines].first_token_idx = data->num_tokens;
    data->lines[data->num_lines].num_sublines++;
}

#ifdef HAVE_LCD_BITMAP

static int parse_statusbar_enable(const char *wps_bufptr,
                                  struct wps_token *token,
                                  struct wps_data *wps_data)
{
    (void)token; /* Kill warnings */
    wps_data->wps_sb_tag = true;
    wps_data->show_sb_on_wps = true;
    return skip_end_of_line(wps_bufptr);
}

static int parse_statusbar_disable(const char *wps_bufptr,
                                   struct wps_token *token,
                                   struct wps_data *wps_data)
{
    (void)token; /* Kill warnings */
    wps_data->wps_sb_tag = true;
    wps_data->show_sb_on_wps = false;
    return skip_end_of_line(wps_bufptr);
}

static bool load_bitmap(struct wps_data *wps_data,
                        char* filename,
                        struct bitmap *bm)
{
    int format;
#ifdef HAVE_REMOTE_LCD
    if (wps_data->remote_wps)
        format = FORMAT_ANY|FORMAT_REMOTE;
    else
#endif
        format = FORMAT_ANY|FORMAT_TRANSPARENT;

    int ret = read_bmp_file(filename, bm,
                            wps_data->img_buf_free,
                            format);

    if (ret > 0)
    {
#if LCD_DEPTH == 16
        if (ret % 2) ret++;
        /* Always consume an even number of bytes */
#endif
        wps_data->img_buf_ptr += ret;
        wps_data->img_buf_free -= ret;

        return true;
    }
    else
        return false;
}

static int get_image_id(int c)
{
    if(c >= 'a' && c <= 'z')
        return c - 'a';
    else if(c >= 'A' && c <= 'Z')
        return c - 'A' + 26;
    else
        return -1;
}

static char *get_image_filename(const char *start, const char* bmpdir,
                                char *buf, int buf_size)
{
    const char *end = strchr(start, '|');

    if ( !end || (end - start) >= (buf_size - ROCKBOX_DIR_LEN - 2) )
    {
        buf = "\0";
        return NULL;
    }

    int bmpdirlen = strlen(bmpdir);

    strcpy(buf, bmpdir);
    buf[bmpdirlen] = '/';
    memcpy( &buf[bmpdirlen + 1], start, end - start);
    buf[bmpdirlen + 1 + end - start] = 0;

    return buf;
}

static int parse_image_display(const char *wps_bufptr,
                               struct wps_token *token,
                               struct wps_data *wps_data)
{
    int n = get_image_id(*wps_bufptr);

    if (n == -1)
    {
        /* invalid picture display tag */
        token->type = WPS_TOKEN_UNKNOWN;
        return 0;
    }

    token->value.i = n;

    /* if the image is in a conditional, remember it */
    if (level >= 0)
        wps_data->img[n].cond_index = condindex[level];

    return 1;
}

static int parse_image_load(const char *wps_bufptr,
                            struct wps_token *token,
                            struct wps_data *wps_data)
{
    int n;
    const char *ptr = wps_bufptr;
    char *pos = NULL;

    /* format: %x|n|filename.bmp|x|y|
       or %xl|n|filename.bmp|x|y|  */

    ptr = strchr(ptr, '|') + 1;
    pos = strchr(ptr, '|');

    if (!pos)
        return 0;

    /* get the image ID */
    n = get_image_id(*ptr);

    /* check the image number and load state */
    if(n < 0 || n >= MAX_IMAGES || wps_data->img[n].loaded)
    {
        /* Skip the rest of the line */
        return 0;
    }

    ptr = pos + 1;

    /* get image name */
    bmp_names[n] = ptr;

    pos = strchr(ptr, '|');
    ptr = pos + 1;

    /* get x-position */
    pos = strchr(ptr, '|');
    if (pos)
        wps_data->img[n].x = atoi(ptr);
    else
    {
        /* weird syntax, bail out */
        return 0;
    }

    /* get y-position */
    ptr = pos + 1;
    pos = strchr(ptr, '|');
    if (pos)
        wps_data->img[n].y = atoi(ptr);
    else
    {
        /* weird syntax, bail out */
        return 0;
    }

    if (token->type == WPS_TOKEN_IMAGE_DISPLAY)
        wps_data->img[n].always_display = true;

    /* Skip the rest of the line */
    return skip_end_of_line(wps_bufptr);
}

static int parse_image_special(const char *wps_bufptr,
                               struct wps_token *token,
                               struct wps_data *wps_data)
{
    if (token->type == WPS_TOKEN_IMAGE_PROGRESS_BAR)
    {
        /* format: %P|filename.bmp| */
        bmp_names[PROGRESSBAR_BMP] = wps_bufptr + 1;
    }
#if LCD_DEPTH > 1
    else if (token->type == WPS_TOKEN_IMAGE_BACKDROP)
    {
        /* format: %X|filename.bmp| */
        bmp_names[BACKDROP_BMP] = wps_bufptr + 1;
    }
#endif

    (void)wps_data; /* to avoid a warning */

    /* Skip the rest of the line */
    return skip_end_of_line(wps_bufptr);
}

#endif /* HAVE_LCD_BITMAP */

static int parse_dir_level(const char *wps_bufptr,
                           struct wps_token *token,
                           struct wps_data *wps_data)
{
    char val[] = { *wps_bufptr, '\0' };
    token->value.i = atoi(val);
    (void)wps_data; /* Kill warnings */
    return 1;
}

static int parse_subline_timeout(const char *wps_bufptr,
                                 struct wps_token *token,
                                 struct wps_data *wps_data)
{
    int skip = 0;
    int val = 0;
    bool have_point = false;
    bool have_tenth = false;

    (void)wps_data; /* Kill the warning */

    while ( isdigit(*wps_bufptr) || *wps_bufptr == '.' )
    {
        if (*wps_bufptr != '.')
        {
            val *= 10;
            val += *wps_bufptr - '0';
            if (have_point)
            {
                have_tenth = true;
                wps_bufptr++;
                skip++;
                break;
            }
        }
        else
            have_point = true;

        wps_bufptr++;
        skip++;
    }

    if (have_tenth == false)
        val *= 10;

    token->value.i = val;

    return skip;
}

static int parse_progressbar(const char *wps_bufptr,
                             struct wps_token *token,
                             struct wps_data *wps_data)
{
    (void)token; /* Kill warnings */
#ifdef HAVE_LCD_BITMAP

    short *vals[] = {
        &wps_data->progress_height,
        &wps_data->progress_start,
        &wps_data->progress_end,
        &wps_data->progress_top };

    /* default values : */
    wps_data->progress_height = 6;
    wps_data->progress_start = 0;
    wps_data->progress_end = 0;
    wps_data->progress_top = -1;

    int i = 0;
    char *newline = strchr(wps_bufptr, '\n');
    char *prev = strchr(wps_bufptr, '|');
    if (prev && prev < newline) {
        char *next = strchr(prev+1, '|');
        while (i < 4 && next && next < newline)
        {
            *(vals[i++]) = atoi(++prev);
            prev = strchr(prev, '|');
            next = strchr(++next, '|');
        }

        if (wps_data->progress_height < 3)
            wps_data->progress_height = 3;
        if (wps_data->progress_end < wps_data->progress_start + 3)
            wps_data->progress_end = 0;
    }

    return newline - wps_bufptr;

#else

    if (*(wps_bufptr-1) == 'f')
        wps_data->full_line_progressbar = true;
    else
        wps_data->full_line_progressbar = false;

    return 0;

#endif
}

/* Parse a generic token from the given string. Return the length read */
static int parse_token(const char *wps_bufptr, struct wps_data *wps_data)
{
    int skip = 0, taglen = 0;
    struct wps_token *token = wps_data->tokens + wps_data->num_tokens;
    const struct wps_tag *tag;

    switch(*wps_bufptr)
    {

        case '%':
        case '<':
        case '|':
        case '>':
        case ';':
        case '#':
            /* escaped characters */
            token->type = WPS_TOKEN_CHARACTER;
            token->value.c = *wps_bufptr;
            taglen = 1;
            wps_data->num_tokens++;
            break;

        case '?':
            /* conditional tag */
            token->type = WPS_TOKEN_CONDITIONAL;
            level++;
            condindex[level] = wps_data->num_tokens;
            numoptions[level] = 1;
            wps_data->num_tokens++;
            taglen = 1 + parse_token(wps_bufptr + 1, wps_data);
            break;

        default:
            /* find what tag we have */
            for (tag = all_tags;
                 strncmp(wps_bufptr, tag->name, strlen(tag->name)) != 0;
                 tag++) ;

            taglen = (tag->type != WPS_TOKEN_UNKNOWN) ? strlen(tag->name) : 2;
            token->type = tag->type;
            wps_data->sublines[wps_data->num_sublines].line_type |=
                                                            tag->refresh_type;

            /* if the tag has a special parsing function, we call it */
            if (tag->parse_func)
                skip += tag->parse_func(wps_bufptr + taglen, token, wps_data);

            /* Some tags we don't want to save as tokens */
            if (tag->type == WPS_NO_TOKEN)
                break;

            /* tags that start with 'F', 'I' or 'D' are for the next file */
            if ( *(tag->name) == 'I' || *(tag->name) == 'F' ||
                 *(tag->name) == 'D')
                token->next = true;

            wps_data->num_tokens++;
            break;
    }

    skip += taglen;
    return skip;
}

/* Parses the WPS.
   data is the pointer to the structure where the parsed WPS should be stored.
        It is initialised.
   wps_bufptr points to the string containing the WPS tags */
static bool wps_parse(struct wps_data *data, const char *wps_bufptr)
{
    if (!data || !wps_bufptr || !*wps_bufptr)
        return false;

    char *stringbuf = data->string_buffer;
    int stringbuf_used = 0;
    int fail = 0;
    line = 1;
    level = -1;

    while(*wps_bufptr && !fail && data->num_tokens < WPS_MAX_TOKENS - 1
          && data->num_lines < WPS_MAX_LINES)
    {
        switch(*wps_bufptr++)
        {

            /* Regular tag */
            case '%':
                wps_bufptr += parse_token(wps_bufptr, data);
                break;

            /* Alternating sublines separator */
            case ';':
                if (level >= 0) /* there are unclosed conditionals */
                {
                    fail = PARSE_FAIL_UNCLOSED_COND;
                    break;
                }

                if (data->num_sublines+1 < WPS_MAX_SUBLINES)
                    wps_start_new_subline(data);
                else
                    wps_bufptr += skip_end_of_line(wps_bufptr);

                break;

            /* Conditional list start */
            case '<':
                if (data->tokens[data->num_tokens-2].type != WPS_TOKEN_CONDITIONAL)
                {
                    fail = PARSE_FAIL_COND_SYNTAX_ERROR;
                    break;
                }

                data->tokens[data->num_tokens].type = WPS_TOKEN_CONDITIONAL_START;
                lastcond[level] = data->num_tokens++;
                break;

            /* Conditional list end */
            case '>':
                if (level < 0) /* not in a conditional, invalid char */
                {
                    fail = PARSE_FAIL_INVALID_CHAR;
                    break;
                }

                data->tokens[data->num_tokens].type = WPS_TOKEN_CONDITIONAL_END;
                if (lastcond[level])
                    data->tokens[lastcond[level]].value.i = data->num_tokens;

                lastcond[level] = 0;
                data->num_tokens++;
                data->tokens[condindex[level]].value.i = numoptions[level];
                level--;
                break;

            /* Conditional list option */
            case '|':
                if (level < 0) /* not in a conditional, invalid char */
                {
                    fail = PARSE_FAIL_INVALID_CHAR;
                    break;
                }

                data->tokens[data->num_tokens].type = WPS_TOKEN_CONDITIONAL_OPTION;
                if (lastcond[level])
                    data->tokens[lastcond[level]].value.i = data->num_tokens;

                lastcond[level] = data->num_tokens;
                numoptions[level]++;
                data->num_tokens++;
                break;

            /* Comment */
            case '#':
                if (level >= 0) /* there are unclosed conditionals */
                {
                    fail = PARSE_FAIL_UNCLOSED_COND;
                    break;
                }

                wps_bufptr += skip_end_of_line(wps_bufptr);
                break;

            /* End of this line */
            case '\n':
                if (level >= 0) /* there are unclosed conditionals */
                {
                    fail = PARSE_FAIL_UNCLOSED_COND;
                    break;
                }

                line++;
                wps_start_new_subline(data);
                data->num_lines++; /* Start a new line */

                if ((data->num_lines < WPS_MAX_LINES) &&
                    (data->num_sublines < WPS_MAX_SUBLINES))
                {
                    data->lines[data->num_lines].first_subline_idx =
                        data->num_sublines;

                    data->sublines[data->num_sublines].first_token_idx =
                        data->num_tokens;
                }

                break;

            /* String */
            default:
                {
                    unsigned int len = 1;
                    const char *string_start = wps_bufptr - 1;

                    /* find the length of the string */
                    while (*wps_bufptr && *wps_bufptr != '#' &&
                           *wps_bufptr != '%' && *wps_bufptr != ';' &&
                           *wps_bufptr != '<' && *wps_bufptr != '>' &&
                           *wps_bufptr != '|' && *wps_bufptr != '\n')
                    {
                        wps_bufptr++;
                        len++;
                    }

                    /* look if we already have that string */
                    char **str;
                    int i;
                    bool found;
                    for (i = 0, str = data->strings, found = false;
                         i < data->num_strings &&
                         !(found = (strlen(*str) == len &&
                                    strncmp(string_start, *str, len) == 0));
                         i++, str++);
                    /* If a matching string is found, found is true and i is
                       the index of the string. If not, found is false */

                    /* If it's NOT a duplicate, do nothing if we already have
                       too many unique strings */
                    if (found ||
                        (stringbuf_used < STRING_BUFFER_SIZE - 1 &&
                         data->num_strings < WPS_MAX_STRINGS))
                    {
                        if (!found)
                        {
                            /* new string */

                            /* truncate? */
                            if (stringbuf_used + len > STRING_BUFFER_SIZE - 1)
                                len = STRING_BUFFER_SIZE - stringbuf_used - 1;

                            strncpy(stringbuf, string_start, len);
                            *(stringbuf + len) = '\0';

                            data->strings[data->num_strings] = stringbuf;
                            stringbuf += len + 1;
                            stringbuf_used += len + 1;
                            data->tokens[data->num_tokens].value.i =
                                data->num_strings;
                            data->num_strings++;
                        }
                        else
                        {
                            /* another ocurrence of an existing string */
                            data->tokens[data->num_tokens].value.i = i;
                        }
                        data->tokens[data->num_tokens].type = WPS_TOKEN_STRING;
                        data->num_tokens++;
                    }
                }
                break;
        }
    }

    if (level >= 0) /* there are unclosed conditionals */
        fail = PARSE_FAIL_UNCLOSED_COND;

#ifdef DEBUG
    print_debug_info(data, fail, line);
#endif

    if (fail)
        wps_reset(data);

    return (fail == 0);
}

#ifdef HAVE_LCD_BITMAP
/* Clear the WPS image cache */
static void wps_images_clear(struct wps_data *data)
{
    int i;
    /* set images to unloaded and not displayed */
    for (i = 0; i < MAX_IMAGES; i++)
    {
       data->img[i].loaded = false;
       data->img[i].display = false;
       data->img[i].always_display = false;
    }
    data->progressbar.have_bitmap_pb = false;
}
#endif

/* initial setup of wps_data */
void wps_data_init(struct wps_data *wps_data)
{
#ifdef HAVE_LCD_BITMAP
    wps_images_clear(wps_data);
    wps_data->wps_sb_tag = false;
    wps_data->show_sb_on_wps = false;
    wps_data->img_buf_ptr = wps_data->img_buf; /* where in image buffer */
    wps_data->img_buf_free = IMG_BUFSIZE; /* free space in image buffer */
    wps_data->peak_meter_enabled = false;
#else /* HAVE_LCD_CHARCELLS */
    int i;
    for (i = 0; i < 8; i++)
    {
        wps_data->wps_progress_pat[i] = 0;
    }
    wps_data->full_line_progressbar = false;
#endif
    wps_data->wps_loaded = false;
}

static void wps_reset(struct wps_data *data)
{
#ifdef HAVE_REMOTE_LCD
    bool rwps = data->remote_wps; /* remember whether the data is for a RWPS */
#endif
    memset(data, 0, sizeof(*data));
    wps_data_init(data);
#ifdef HAVE_REMOTE_LCD
    data->remote_wps = rwps;
#endif
}

#ifdef HAVE_LCD_BITMAP


static void clear_bmp_names(void)
{
    int n;
    for (n = 0; n < MAX_BITMAPS; n++)
    {
        bmp_names[n] = NULL;
    }
}

static void load_wps_bitmaps(struct wps_data *wps_data, char *bmpdir)
{
    char img_path[MAX_PATH];
    struct bitmap *bitmap;
    bool *loaded;

    int n;
    for (n = 0; n < BACKDROP_BMP; n++)
    {
        if (bmp_names[n])
        {
            get_image_filename(bmp_names[n], bmpdir,
                               img_path, sizeof(img_path));

            if (n == PROGRESSBAR_BMP) {
                /* progressbar bitmap */
                bitmap = &wps_data->progressbar.bm;
                loaded = &wps_data->progressbar.have_bitmap_pb;
            } else {
                /* regular bitmap */
                bitmap = &wps_data->img[n].bm;
                loaded = &wps_data->img[n].loaded;
            }

            /* load the image */
            bitmap->data = wps_data->img_buf_ptr;
            if (load_bitmap(wps_data, img_path, bitmap))
            {
                *loaded = true;
            }
        }
    }

#if (LCD_DEPTH > 1) || (defined(HAVE_LCD_REMOTE) && (LCD_REMOTE_DEPTH > 1))
    if (bmp_names[BACKDROP_BMP])
    {
        get_image_filename(bmp_names[BACKDROP_BMP], bmpdir,
                            img_path, sizeof(img_path));
#ifdef HAVE_REMOTE_LCD
        if (wps_data->remote_wps)
#if LCD_REMOTE_DEPTH > 1
            load_remote_wps_backdrop(img_path)
#endif
            ;
        else
#endif /* HAVE_REMOTE_LCD */
            load_wps_backdrop(img_path);
    }
#endif /* has backdrop support */
}

#endif /* HAVE_LCD_BITMAP */

/* Skip leading UTF-8 BOM, if present. */
static char *skip_utf8_bom(char *buf)
{
    unsigned char *s = (unsigned char *)buf;

    if(s[0] == 0xef && s[1] == 0xbb && s[2] == 0xbf)
    {
        buf += 3;
    }

    return buf;
}

/* to setup up the wps-data from a format-buffer (isfile = false)
   from a (wps-)file (isfile = true)*/
bool wps_data_load(struct wps_data *wps_data,
                   const char *buf,
                   bool isfile)
{
    if (!wps_data || !buf)
        return false;

    wps_reset(wps_data);

    if (!isfile)
    {
        return wps_parse(wps_data, buf);
    }
    else
    {
        /*
         * Hardcode loading WPS_DEFAULTCFG to cause a reset ideally this
         * wants to be a virtual file.  Feel free to modify dirbrowse()
         * if you're feeling brave.
         */
#ifndef __PCTOOL__
        if (! strcmp(buf, WPS_DEFAULTCFG) )
        {
            global_settings.wps_file[0] = 0;
            return false;
        }

#ifdef HAVE_REMOTE_LCD
        if (! strcmp(buf, RWPS_DEFAULTCFG) )
        {
            global_settings.rwps_file[0] = 0;
            return false;
        }
#endif
#endif /* __PCTOOL__ */

        int fd = open(buf, O_RDONLY);

        if (fd < 0)
            return false;

        /* get buffer space from the plugin buffer */
        size_t buffersize = 0;
        char *wps_buffer = (char *)plugin_get_buffer(&buffersize);

        if (!wps_buffer)
            return false;

        /* copy the file's content to the buffer for parsing,
           ensuring that every line ends with a newline char. */
        unsigned int start = 0;
        while(read_line(fd, wps_buffer + start, buffersize - start) > 0)
        {
            start += strlen(wps_buffer + start);
            if (start < buffersize - 1)
            {
                wps_buffer[start++] = '\n';
                wps_buffer[start] = 0;
            }
        }

        close(fd);

        if (start <= 0)
            return false;

#ifdef HAVE_LCD_BITMAP
        clear_bmp_names();
#endif

        /* Skip leading UTF-8 BOM, if present. */
        wps_buffer = skip_utf8_bom(wps_buffer);

        /* parse the WPS source */
        if (!wps_parse(wps_data, wps_buffer))
            return false;

        wps_data->wps_loaded = true;

#ifdef HAVE_LCD_BITMAP
        /* get the bitmap dir */
        char bmpdir[MAX_PATH];
        size_t bmpdirlen;
        char *dot = strrchr(buf, '.');
        bmpdirlen = dot - buf;
        strncpy(bmpdir, buf, dot - buf);
        bmpdir[bmpdirlen] = 0;

        /* load the bitmaps that were found by the parsing */
        load_wps_bitmaps(wps_data, bmpdir);
#endif
        return true;
    }
}

int wps_subline_index(struct wps_data *data, int line, int subline)
{
    return data->lines[line].first_subline_idx + subline;
}

int wps_first_token_index(struct wps_data *data, int line, int subline)
{
    int first_subline_idx = data->lines[line].first_subline_idx;
    return data->sublines[first_subline_idx + subline].first_token_idx;
}

int wps_last_token_index(struct wps_data *data, int line, int subline)
{
    int first_subline_idx = data->lines[line].first_subline_idx;
    int idx = first_subline_idx + subline;
    if (idx < data->num_sublines - 1)
    {
        /* This subline ends where the next begins */
        return data->sublines[idx+1].first_token_idx - 1;
    }
    else
    {
        /* The last subline goes to the end */
        return data->num_tokens - 1;
    }
}
