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

  gchar     *name;      // station name
  gchar     *url;       // mpv に渡すurl
  gchar     *banner;    // banner file name
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
mpvradio_banner_new_with_data (GtkOrientation orientation, gint spacing,
                                    gchar *name, gchar *url, gchar *banner)
{
    MpvradioBanner *self = MPVRADIO_BANNER (mpvradio_banner_new (orientation, spacing));

    mpvradio_banner_set_name (self, name);
    mpvradio_banner_set_url (self, url);
    mpvradio_banner_set_banner (self, banner);

	gint width = -1, height = -1;
	GdkPixbufFormat *bf = gdk_pixbuf_get_file_info (banner, &width, &height);
	if (bf != NULL) {
		if (width > 200) width = 200;
		if (height > 64) height = 64;
	}

	
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_size (banner, width, height, NULL);
    if (pixbuf != NULL) {
		self->image = gtk_image_new_from_pixbuf (pixbuf);
		g_object_unref (pixbuf);
	}
	else {
		self->image = gtk_image_new_from_icon_name ("image-missing", GTK_ICON_SIZE_DIALOG);
	}

    self->label = gtk_label_new (name);
    gtk_label_set_width_chars (GTK_LABEL(self->label), 10);
    gtk_label_set_max_width_chars (GTK_LABEL(self->label), 20);
    gtk_label_set_line_wrap (GTK_LABEL(self->label), TRUE);

    gtk_box_pack_start (GTK_BOX(self), self->image, TRUE, TRUE, 0);
    gtk_box_pack_end (GTK_BOX(self), self->label, TRUE, TRUE, 0);

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

gchar *
mpvradio_banner_get_banner (MpvradioBanner *self)
{
    return self->banner;
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

void
mpvradio_banner_set_banner (MpvradioBanner *self, gchar *banner)
{
    g_free (self->banner);
    self->banner = g_strdup (banner);
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
    self->name   = NULL;
    self->url    = NULL;
    self->banner = NULL;
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
    g_free (self->banner);

    (*G_OBJECT_CLASS (mpvradio_banner_parent_class)->finalize) (object);
}
