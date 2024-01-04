#ifndef __MPVRADIO_BANNER_H__
#define __MPVRADIO_BANNER_H__

#include <glib-object.h>
#include <gobject/gtype.h>

G_BEGIN_DECLS

// 継承できないクラスの宣言。継承できるクラスを定義する場合は、
// ヘッダファイルに構造体を書く。今回はCファイルに構造体宣言を書いている。
G_DECLARE_FINAL_TYPE(MpvradioBanner, mpvradio_banner, MPVRADIO, BANNER, GtkBox)


#define MPVRADIO_TYPE_BANNER            (mpvradio_banner_get_type ())
#define MPVRADIO_BANNER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPVRADIO_TYPE_BANNER, MpvradioBanner))
#define MPVRADIO_BANNER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MPVRADIO_TYPE_BANNER, MpvradioBannerClass))
#define MPVRADIO_IS_BANNER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPVRADIO_TYPE_BANNER))
#define MPVRADIO_IS_BANNER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), MPVRADIO_TYPE_BANNER))
#define MPVRADIO_BANNER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPVRADIO_TYPE_BANNER, MpvradioBannerClass))

//~ typedef struct _MpvradioBannerClass MpvradioBannerClass;
typedef struct _MpvradioBanner      MpvradioBanner;

GType           mpvradio_banner_get_type          (void) G_GNUC_CONST;

GtkWidget      *mpvradio_banner_new               (GtkOrientation orientation, gint spacing);
GtkWidget      *mpvradio_banner_new_with_data   (GtkOrientation orientation, gint spacing, gchar *name, gchar *url);
gchar          *mpvradio_banner_get_name        (MpvradioBanner *self);
gchar          *mpvradio_banner_get_url         (MpvradioBanner *self);
void            mpvradio_banner_set_name        (MpvradioBanner *self, gchar *name);
void            mpvradio_banner_set_url         (MpvradioBanner *self, gchar *url);

gboolean        mpvradio_banner_do_execute        (MpvradioBanner *self);
void            mpvradio_banner_popup             (MpvradioBanner *self, GdkEvent *event);
G_END_DECLS

#endif /* !__MPVRADIO_BANNER_H__ */

