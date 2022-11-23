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

#include <json-glib/json-glib.h>
#include <json-glib/json-gobject.h>

#include "mpvradio.h"
#include "mpvradio-stationbutton.h"
#include "mpvradio-ipc.h"

extern XAppStatusIcon *appindicator;
extern GtkWidget *infomessage;             // IPC受け取り後の格納用

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
 * 一時停止・再開
 */
void mpvradio_common_toggle_pause (void)
{
    char *s;
    int retval = 0;
    GError *er;

    s = mpvradio_ipc_send_and_response (
        "{\"command\": [\"get_property\", \"pause\"]}\x0a");
    //~ g_message ("pause -->");g_message ("[=[%s]=]\n",s);
    JsonParser *parser = json_parser_new ();
    json_parser_load_from_data (parser, s, -1, NULL);

    JsonReader *reader = json_reader_new (json_parser_get_root (parser));

    json_reader_read_member (reader, "data");
    //~ er = json_reader_get_error (reader);
    //~ if (er) {g_message ("read member --->\n");g_error (er->message);}

    gboolean mpv_data = json_reader_get_boolean_value (reader);
    //~ er = json_reader_get_error (reader);
    //~ if (er) {g_message ("get value --->\n");g_error (er->message);}

    //~ g_print ("data : %s(%d)\n", mpv_data?"true":"false",mpv_data);
    json_reader_end_member (reader);

    g_object_unref (reader);
    g_object_unref (parser);

    g_free (s);

    if (!mpv_data) {
        mpvradio_ipc_send ("{\"command\": [\"set_property\", \"pause\", true]}\x0a");
    }
    else {
        mpvradio_ipc_send ("{\"command\": [\"set_property\", \"pause\", false]}\x0a");
    }
}

/*
 * 再生（受信）停止
 */
void mpvradio_common_stop (void)
{
    //~ if (current_mpv) {
        //~ g_message ("kill %d", current_mpv);
        //~ kill (current_mpv, SIGTERM);
    //~ }
    mpvradio_ipc_send ("{\"command\": [\"stop\"]}\x0a");
    gtk_entry_buffer_set_text (infomessage, "",-1);
    xapp_status_icon_set_label (appindicator,
                            gtk_entry_buffer_get_text (infomessage));
}

/*
 * 次の曲
 */
void mpvradio_common_next (void)
{
    mpvradio_ipc_send ("{\"command\": [\"playlist-next\"]}\x0a");
}

/*
 * 前の曲
 */
void mpvradio_common_prev (void)
{
    mpvradio_ipc_send ("{\"command\": [\"playlist-prev\"]}\x0a");
}
