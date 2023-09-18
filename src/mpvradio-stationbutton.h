/*
 * mpvradio-stationbutton.h
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


#ifndef __mpvradio_STATIONBUTTON_H__
#define __mpvradio_STATIONBUTTON_H__

G_BEGIN_DECLS

#include "mpvradio.h"

typedef struct _mpvradioStationbuttonPrivate mpvradioStationbuttonPrivate;
typedef struct _mpvradioStationbuttonClass   mpvradioStationbuttonClass;
typedef struct _mpvradioStationbutton        mpvradioStationbutton;

#define mpvradio_SCROLL_MARGIN 0.02

#define mpvradio_TYPE_STATIONBUTTON            (mpvradio_stationbutton_get_type ())
#define mpvradio_STATIONBUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), mpvradio_TYPE_STATIONBUTTON, mpvradioStationbutton))
#define mpvradio_STATIONBUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), mpvradio_TYPE_STATIONBUTTON, mpvradioStationbuttonClass))
#define mpvradio_IS_STATIONBUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), mpvradio_TYPE_STATIONBUTTON))
#define mpvradio_IS_STATIONBUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), mpvradio_TYPE_STATIONBUTTON))
#define mpvradio_STATIONBUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), mpvradio_TYPE_STATIONBUTTON, mpvradioStationbuttonClass))

struct _mpvradioStationbutton
{
  GtkButton        __parent__;

  /* private structure */
  mpvradioStationbuttonPrivate *priv;

  gchar                    *uri;
  GtkFlowBoxChild          *container;
};

GType             mpvradio_stationbutton_get_type       (void) G_GNUC_CONST;

mpvradioStationbutton *mpvradio_stationbutton_new            (void);
char              *mpvradio_stationbutton_get_uri       (mpvradioStationbutton *btn);
void               mpvradio_stationbutton_set_uri       (mpvradioStationbutton *btn,
                                                     gchar *uri);

mpvradioStationbutton *mpvradio_stationbutton_new_with_data (const gchar *label, gchar *u);

G_END_DECLS

#endif /* !__mpvradio_STATIONBUTTON_H__ */
