/*
 * mpvradio.h
 *
 * Copyright 2019 sakai <endeavor2wako@gmail.com>
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

#include "glib/gi18n.h"
#include "gtk/gtk.h"
#include "gdk/gdkkeysyms.h"
#include "libnotify/notification.h"
#include "libnotify/notify.h"
#include "mpd/client.h"

#ifndef mpvradio_H
#define mpvradio_H

#define mpvradioICON   "media-playback-start-symbolic"

struct serverinfo {
            gchar *id;
            gchar *name;
            gchar *ip4;
            unsigned port;
            gboolean ua;
            gchar *password;
            gchar *music_directory;
};
#endif
