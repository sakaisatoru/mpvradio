#ifdef HAVE_CONFIG_H
#   include "config.h"
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
#include "libxapp/xapp-status-icon.h"
#include "libxapp/xapp-preferences-window.h"

#include "mpvradio.h"
#include "mpvradio-stationbutton.h"

extern XAppStatusIcon *appindicator;

extern void mpvradio_stop_mpv (void);

/*
 * シグナルコールバック用の汎用ラッパ
 */
void mpvradio_common_cb (GtkWidget *menuitem, gpointer user_data)
{
    ((void (*)(void))user_data)();
}

/*
 * 音量調整
 */
void mpvradio_common_volume_value_change (double value)
{
}

/*
 * 再生（受信）開始
 */
void mpvradio_common_play (void)
{
}

/*
 * 再生（受信）停止
 */
void mpvradio_common_stop (void)
{
    mpvradio_stop_mpv ();
    xapp_status_icon_set_label (appindicator, "");
}

/*
 * 次の曲
 */
void mpvradio_common_next (void)
{
}

/*
 * 前の曲
 */
void mpvradio_common_prev (void)
{
}
