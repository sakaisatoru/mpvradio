/*
 * mpvradio-stationbutton.c
 *
 * Copyright 2022 phous <endeavor2wako@gmail.com>
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
#include <config.h>
#endif

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>
#include <glib.h>

#include "glib/gi18n.h"
#include "gtk/gtk.h"
#include "gdk/gdkkeysyms.h"
#include "libnotify/notification.h"
#include "libnotify/notify.h"
#include "mpvradio-stationbutton.h"


#define mpvradio_STATIONBUTTON_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), mpvradio_TYPE_STATIONBUTTON, mpvradioStationbuttonPrivate))

static void      mpvradio_stationbutton_finalize                (GObject                *object);

struct _mpvradioStationbuttonClass
{
  GtkButtonClass __parent__;
};

struct _mpvradioStationbuttonPrivate
{
  GtkButton __parent__;

};

mpvradioStationbutton *
mpvradio_stationbutton_new (void)
{
  return g_object_new (mpvradio_TYPE_STATIONBUTTON, NULL);
}

G_DEFINE_TYPE (mpvradioStationbutton, mpvradio_stationbutton, GTK_TYPE_BUTTON);

static void
mpvradio_stationbutton_class_init (mpvradioStationbuttonClass *klass)
{
  GObjectClass *gobject_class;

  g_type_class_add_private (klass, sizeof (mpvradioStationbuttonPrivate));

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = mpvradio_stationbutton_finalize;
}


static void
mpvradio_stationbutton_init (mpvradioStationbutton *button)
{
    /* private structure */
    button->priv = mpvradio_STATIONBUTTON_GET_PRIVATE (button);

    button->uri = NULL;
}

static void
mpvradio_stationbutton_finalize (GObject *object)
{
    mpvradioStationbutton *button = mpvradio_STATIONBUTTON (object);

    /* cleanup */
    //~ g_message ("stationbutton '%s' free.\n", button->uri);
    g_free (button->uri);
    (*G_OBJECT_CLASS (mpvradio_stationbutton_parent_class)->finalize) (object);
}


void
mpvradio_stationbutton_set_uri (mpvradioStationbutton *button, gchar *u)
{
    button->uri = g_strdup (u);
}

char
*mpvradio_stationbutton_get_uri (mpvradioStationbutton *button)
{
    return button->uri;
}
