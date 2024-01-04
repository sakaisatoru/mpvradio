#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <libintl.h>
#include <locale.h>

#include <errno.h>
#include <glib/gstdio.h>
#include "glib.h"
#include "gtk/gtk.h"
#include "gdk/gdk.h"
#include "mpvradio-banner.h"

struct _MpvradioBannerClass
{
  GtkBoxClass __parent__;
};

struct _MpvradioBanner
{
  GtkBox __parent__;

  GtkWidget *image;
  GtkWidget *label;

  gchar     *name;  // station name
  gchar     *url;   // mpv に渡すurl
};


static void mpvradio_banner_class_init    (MpvradioBannerClass *klass);
static void mpvradio_banner_init          (MpvradioBanner *item);
static void mpvradio_banner_dispose       (GObject *object);
static void mpvradio_banner_finalize      (GObject *object);
static gboolean mpvradio_banner_delete_event (MpvradioBanner *item, GdkEvent *event);

G_DEFINE_TYPE (MpvradioBanner, mpvradio_banner, GTK_TYPE_BOX);


GtkWidget *
mpvradio_banner_new (GtkOrientation orientation, gint spacing)
{
    GtkWidget *self = g_object_new (MPVRADIO_TYPE_BANNER, NULL);
    gtk_orientable_set_orientation (GTK_ORIENTABLE(self), orientation);
    gtk_box_set_spacing (GTK_BOX(self), spacing);
    return self;
}

GtkWidget *
mpvradio_banner_new_with_data (GtkOrientation orientation, gint spacing, gchar *name, gchar *url)
{
    MpvradioBanner *self = MPVRADIO_BANNER (mpvradio_banner_new (orientation, spacing));

    mpvradio_banner_set_name (self, name);
    mpvradio_banner_set_url (self, url);

    GdkPixbuf *pixbuf, *buf2;
    gint banner_width = 128, banner_height = 64;

    // url の末尾を元にキャッシュリストからイメージファイルを漁ってくる処理を入れる
    //~ pixbuf = gdk_pixbuf_from_file (/*banner image cache file*/, NULL);
    pixbuf = gdk_pixbuf_new_from_file ("/home/sakai/.cache/mpvradio/logo/JOAK-FM.png", NULL);// test用
    //~ if (pixbuf != NULL) {
        //~ buf2 = gdk_pixbuf_scale_simple (pixbuf, banner_width, banner_height, GDK_INTERP_BILINEAR);
    //~ }
    //~ self->image = gtk_image_new_from_pixbuf (buf2);
    self->image = gtk_image_new_from_pixbuf (pixbuf);
    g_object_unref (pixbuf);
    g_object_unref (buf2);

    self->label = gtk_label_new (name);

    gtk_box_pack_start (GTK_BOX(self), self->image, FALSE, TRUE, 10);
    gtk_box_pack_start (GTK_BOX(self), self->label, FALSE, TRUE, 5);

    return GTK_WIDGET(self);
}

gchar *
mpvradio_banner_get_name (MpvradioBanner *self)
{
    return self->name;
}

gchar *
mpvradio_banner_get_url (MpvradioBanner *self)
{
    return self->url;
}

void
mpvradio_banner_set_name (MpvradioBanner *self, gchar *name)
{
    g_free (self->name);
    self->name = g_strdup (name);
}

void
mpvradio_banner_set_url (MpvradioBanner *self, gchar *url)
{
    g_free (self->url);
    self->url = g_strdup (url);
}


static void
mpvradio_banner_class_init (MpvradioBannerClass *klass)
{
    GObjectClass   *gobject_class;
    GtkWidgetClass *gtkwidget_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->dispose = mpvradio_banner_dispose;
    gobject_class->finalize = mpvradio_banner_finalize;

    gtkwidget_class = GTK_WIDGET_CLASS (klass);
    //~ gtkwidget_class->configure_event = mpvradio_banner_configure_event;
}

static void
mpvradio_banner_init (MpvradioBanner *self)
{
    self->name  = NULL;
    self->url   = NULL;
}


static void
mpvradio_banner_dispose (GObject *object)
{
    (*G_OBJECT_CLASS (mpvradio_banner_parent_class)->dispose) (object);
}


static void
mpvradio_banner_finalize (GObject *object)
{
    MpvradioBanner *self = MPVRADIO_BANNER (object);

    g_free (self->name);
    g_free (self->url);

    (*G_OBJECT_CLASS (mpvradio_banner_parent_class)->finalize) (object);
}
