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
//~ #include "mpvradio-stationbutton.h"
#include "mpvradio-ipc.h"

extern XAppStatusIcon *appindicator;
extern GtkEntryBuffer *infomessage;             // IPC受け取り後の格納用

/*
 * シグナルコールバック用の汎用ラッパ
 */
void mpvradio_common_cb (GtkWidget *menuitem, gpointer user_data)
{
    ((void (*)(void))user_data)();
}

/*
 * mpv が演奏中なら TRUE を返す
 */
gboolean mpvradio_common_check_mpv_is_play (void)
{
    char *s;
    GError *er = NULL;

    s = mpvradio_ipc_send_and_response (
        "{\"command\": [\"get_property\", \"stream-path\"]}\x0a");
    //~ g_message ("path -->");g_message ("[=[%s]=]\n",s);
    JsonParser *parser = json_parser_new ();
    json_parser_load_from_data (parser, s, -1, NULL);

    JsonReader *reader = json_reader_new (json_parser_get_root (parser));

    json_reader_read_member (reader, "data");
    //~ er = json_reader_get_error (reader);
    //~ if (er) {g_message ("read member --->\n");g_error (er->message);}

    const gchar *mpv_data = json_reader_get_string_value (reader);
    //~ g_print ("result : %s\n", mpv_data);
    //~ er = json_reader_get_error (reader);
    //~ if (er) {g_message ("get value --->\n");g_error (er->message);}

    //~ g_print ("data : %s(%d)\n", mpv_data?"true":"false",mpv_data);
    json_reader_end_member (reader);

    g_object_unref (reader);
    g_object_unref (parser);

    g_free (s);

    return (mpv_data == NULL)?FALSE:TRUE;
}


/*
 * 再生（受信）停止
 */
void mpvradio_common_stop (void)
{
    mpvradio_ipc_send ("{\"command\": [\"stop\"]}\x0a");
    gtk_entry_buffer_set_text (infomessage, "",-1);
    if (XAPP_IS_STATUS_ICON(appindicator)) {
        xapp_status_icon_set_label (appindicator,
                                gtk_entry_buffer_get_text (infomessage));
    }
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


/*
 * 引数のURLをみてmpvに渡す
 */
gboolean mpvradio_common_mpv_play (gpointer url)
{
    gchar *scheme;
    if (url != NULL) {
        // パスをそのまま渡すと g_uri_parse_schemeでセグるので事前に判定する
        // 2.66 and later
#if GLIB_CHECK_VERSION (2,66,0)
            scheme = (g_uri_is_valid (url, G_URI_FLAGS_NONE, NULL) == TRUE)?
                g_uri_parse_scheme ((gchar*)url) : g_strdup ("");
#else
            scheme = g_uri_parse_scheme ((gchar*)url);
            if (scheme == NULL) scheme = g_strdup ("");
#endif
        if (!strcmp (scheme, "plugin")) {
            // url の先頭がpluginであれば呼び出しにかかる
            gchar *station = g_path_get_basename ((gchar*)url); // basename をplugin の引数にする
            //~ g_print ("station:%s\n", station);

            gchar *p_path = g_path_get_dirname ((gchar*)url);
            gchar **plugin = g_strsplit (p_path, ":", 2);
            if (plugin != NULL) {
                gchar *command = g_strdup_printf ("%s/mpvradio/plugins%s %s",
                                            DATADIR, plugin[1], station);
                system (command);
                //~ g_print ("plugin :%s\n", command);
                g_free (command);
                g_strfreev (plugin);
            }
            g_free (p_path);
            g_free (station);
        }
        else {
            // url や playlist であればそのまま mpv に送る
            mpvradio_ipc_send ("{\"command\": [\"set_property\", \"pause\", false]}\x0a");
            char *message = g_strdup_printf ("{\"command\": [\"loadfile\",\"%s\"]}\x0a", (gchar*)url);
            //~ printf ("%s\n", message);
            mpvradio_ipc_send (message);
            g_free (message);
        }
        g_free (scheme);
    }
    return G_SOURCE_REMOVE;
}
