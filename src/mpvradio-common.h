/*
 * mpvradio-common.h
 *
 * Copyright 2022 sakai satoru <endeavor2wako@gmail.com>
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

#include "gtk/gtk.h"

/* mpvradio-common.c */
extern void mpvradio_common_cb (GtkWidget *widget, gpointer user_data);
extern void mpvradio_common_play (void);
extern void mpvradio_common_toggle_pause (void);
extern void mpvradio_common_stop (void);
extern void mpvradio_common_next (void);
extern void mpvradio_common_prev (void);
extern void mpvradio_common_volume_value_change (double);
extern gboolean mpvradio_common_mpv_play (gpointer url);


