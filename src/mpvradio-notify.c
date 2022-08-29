/*
 * mpvradio_notify.c
 *
 * Copyright 2022 sakai <endeavor2wako@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glib.h>

#include "glib/gi18n.h"
#include "gtk/gtk.h"
#include "gdk/gdkkeysyms.h"
#include "libnotify/notification.h"
#include "libnotify/notify.h"

#include "libxapp/xapp-status-icon.h"

#include "mpvradio.h"

/* mpvradio-main.c */
extern XAppStatusIcon *appindicator;
extern NotifyNotification *notifi;      // デスクトップ通知

/*
 * notify_notification が & を受け付けないのでエスケープする
 * g_markup_escape_text　でも良いが、今度は ' 等もエスケープ
 * して、それは &apos;とか表示されてしまうのでうまくない。
 */
static char *escape_amp (char *s)
{
    gchar **v, **pos, *r, *t;

    if (s == NULL) return NULL;
#if 0
    v = pos = g_strsplit (s, "&", 0);
    r = g_strdup (*pos);
    while (*++pos != NULL) {
        t = g_strdup_printf ("%s&amp;%s", r, *pos);
        g_free (r);
        r = t;
    }
    g_strfreev (v);
#else
    r = g_markup_printf_escaped ("%s", s);
#endif
    return r;
}

/*
 * 現在再生中の曲をデスクトップに通知する
 */
void mpvradio_notify_currentsong (void)
{

}


