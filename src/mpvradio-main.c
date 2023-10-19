/*
 * mpvradio-main.c
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
#   include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>
#include <glib.h>
#include <locale.h>

#include "glib/gi18n.h"
#include "gtk/gtk.h"
#include "glib.h"
#include "gdk/gdkkeysyms.h"

#include "libxapp/xapp-status-icon.h"
#include "libxapp/xapp-preferences-window.h"

#include "mpvradio.h"
#include "mpvradio-common.h"
#include "mpvradio-statusicon.h"
#include "mpvradio-ipc.h"
//~ #include "mpvradio-playlist.h"
#include "mpvradio-radiopanel.h"

GtkWindow *radikopanel;
GtkWindow *selectergrid;
XAppStatusIcon *appindicator;       // LinuxMint 専用
GtkEntryBuffer *infomessage;             // IPC受け取り後の格納用

static GKeyFile *kconf;


GtkWidget *volume_up_button, *volume_down_button;

static gpointer mpvradio_eventmonitor_gt (gpointer n);
static void mpvradio_cb (GtkWidget *menuitem, gpointer user_data);

static gboolean mpvradio_recv_dead;
static gboolean mpvradio_recv_stop;
static GtkActionGroup *actions;
static gboolean mpvradio_connection_in;

static const gint button_width = 100;
static const gint button_height = 48;


/* mpvradio_adduri.c */
extern gchar *mpvradio_adduri_quicktune (GtkWindow *oya);
//~ extern void mpvradio_adduridialog_destroy (void);


/* mpvradio-config.c */
extern void detach_config_file (GKeyFile *);
extern void save_config_file (GKeyFile *);
extern GKeyFile *load_config_file (void);
extern XAppPreferencesWindow *mpvradio_config_prefernces_ui (void);


/*
 * 音量調整
 */
static void
volume_value_change_cb (GtkScaleButton *button,
                                           double          value,
                                           gpointer        user_data)
{
    char *message;

    message =
        g_strdup_printf ("{\"command\": [\"set_property\",\"volume\",%d]}\x0a", (uint32_t)(value*100.));
    mpvradio_ipc_send (message);
    g_free (message);
}


static void
radiopanel_destroy_cb (GtkWidget *widget, gpointer data)
{
    if (GTK_IS_WINDOW (widget)) {
        g_message ("destroy_cb : %s.", gtk_window_get_title (widget));
    }
}


/*
 * playlist の内容をハッシュテーブルに格納する
 */
GHashTable *playlist_table;
GList *playlist_sorted = NULL;
void
mpvradio_read_playlist (void)
{
    gchar buf[256], *pos, *station, **playlist, **pl, *fn;
    int i, flag = FALSE;

    playlist = g_key_file_get_keys (kconf, "playlist", NULL, NULL);
    playlist_table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

    pl = playlist;
    while (*pl != NULL) {
        fn = g_key_file_get_value (kconf, "playlist", *pl, NULL);
        //~ g_message (fn);
        FILE *fp = fopen (fn, "r");
        if (fp != NULL) {
            while (!feof(fp)) {
                if (fgets (buf, sizeof(buf)-1, fp) == NULL) {
                    if (ferror(fp)) {
                        break;
                    }
                }
                pos = g_strchug (buf);

                // Extended M3U
                if (sscanf (pos, "#EXTINF:%d,%*[A-Za-z0-9()_. ] / %*s\n", &i)) {
                    char *st = strrchr (pos, '/');
                    if (st != NULL) {
                        st++;
                        while (*st == ' ' || *st == '\t') st++;
                        char *tail = strrchr (st, '\n');
                        if (tail != NULL) *tail = '\0';
                        station = g_strdup (st);
                        flag = TRUE;
                    }
                    else {
                        station = NULL;
                        flag = FALSE;
                    }
                    continue;
                }

                if (flag) {
                    // 直前に #EXTINFがあれば URLとして読み込む
                    if (*pos != '#') {  // コメント避け
                        char *tail = strrchr (pos, '\n');
                        if (tail != NULL) *tail = '\0';
                        if (station != NULL) {
                            g_hash_table_insert (playlist_table, station, g_strdup (pos));
                            // ソート用のリストに追加する
                            // hashテーブルに格納した局名は、hashテーブル開放時に破壊されるので
                            // リスト用に新規に確保する。
                            playlist_sorted = g_list_append (playlist_sorted, g_strdup(station));
                        }
                        flag = FALSE;
                    }
                }

            }
            fclose (fp);
        }
        else {
            g_message ("file not found. %s", *pl);
        }
        g_free (fn);
        pl++;
    }
    g_strfreev (playlist);
    playlist_sorted = g_list_sort (playlist_sorted, strcmp);
}


static GtkWidget *box2; // playlist 挿入用

void infotext_inserted_text_cb (GtkEntryBuffer *buffer,
                                   guint           position,
                                   char           *chars,
                                   guint           n_chars,
                                   gpointer        user_data)
{
    g_print ("insert infotext:%s", chars);
}


/*
 * 選局パネル(ウィンドウ)の作成
 */
GtkWindow *
mpvradio_window_new (GtkApplication *application)
{
    struct mpd_connection *cn;
    struct mpd_playlist *pl;
    struct mpd_entry *en;
    struct mpd_song *sn;
    struct mpd_status *st;

    int i, width, height;
    double vol = 0.5;

    GtkWidget *window, *btn, *header, *scroll, *box;
    GtkWidget *tapescroll, *tapelist;
    GtkWidget *infobar, *infotext, *infocontainer;
    GtkWidget *volbtn, *stopbtn;
    GMenuModel *menumodel;

    GtkWidget *stack, *stackswitcher;

    gint x,y;

    window = gtk_application_window_new (application);
    g_signal_connect (G_OBJECT(window), "destroy",
                        G_CALLBACK(radiopanel_destroy_cb), NULL);
    g_signal_connect (G_OBJECT(window), "delete-event",
                        G_CALLBACK(gtk_widget_hide_on_delete), NULL);

    // ボリュームボタン
    volbtn = gtk_volume_button_new ();
    //~ gtk_scale_button_set_value (volbtn, 1.0);
    g_signal_connect (G_OBJECT(volbtn), "value-changed",
                        G_CALLBACK(volume_value_change_cb), NULL);

    volume_up_button = gtk_scale_button_get_plus_button (GTK_SCALE_BUTTON (volbtn));
    volume_down_button = gtk_scale_button_get_minus_button (GTK_SCALE_BUTTON (volbtn));


    // ストップボタン
    stopbtn = gtk_button_new_from_icon_name ("media-playback-stop-symbolic",
                                                GTK_ICON_SIZE_BUTTON);
    g_signal_connect (G_OBJECT(stopbtn), "clicked",
                                G_CALLBACK(mpvradio_common_cb), mpvradio_common_stop);

    header = gtk_header_bar_new ();
    gtk_header_bar_set_decoration_layout (GTK_HEADER_BAR (header), "menu:close");
    gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (header), TRUE);
    gtk_header_bar_set_title (GTK_HEADER_BAR (header), PACKAGE);
    gtk_header_bar_set_has_subtitle (GTK_HEADER_BAR (header), TRUE);

    gtk_window_set_titlebar (GTK_WINDOW (window), header);
    gtk_header_bar_pack_end (GTK_HEADER_BAR (header), volbtn);
    gtk_header_bar_pack_end (GTK_HEADER_BAR (header), stopbtn);


    /* 情報表示用 */
    infotext = gtk_entry_new_with_buffer (infomessage);
    gtk_widget_set_can_focus (infotext, FALSE);

    selectergrid = mpvradio_radiopanel_new ();

    gtk_scale_button_set_value (GTK_SCALE_BUTTON (volbtn), vol);

    scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_kinetic_scrolling (GTK_SCROLLED_WINDOW (scroll), TRUE);
    gtk_scrolled_window_set_capture_button_press (GTK_SCROLLED_WINDOW (scroll), TRUE);
    gtk_scrolled_window_set_overlay_scrolling (GTK_SCROLLED_WINDOW (scroll), TRUE);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER (scroll), selectergrid);
    //~ gtk_widget_set_size_request (scroll, 400, 400);

    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
    gtk_box_pack_start (box, infotext, FALSE, TRUE, 0);
    gtk_box_pack_start (box, scroll, TRUE, TRUE, 0);

    gtk_container_add (GTK_CONTAINER(window), box);

    return window;
}


static void
quicktune_activated (GSimpleAction *action,
                       GVariant      *parameter,
                       gpointer       app)
{
    GList *windows;
    windows = gtk_application_get_windows (app);
    if (windows != NULL) {
        gchar *uri = mpvradio_adduri_quicktune (windows->data);
        if (uri != NULL) {
            gchar *tmp = g_strdup_printf ("{\"command\": [\"loadfile\",\"%s\"]}\x0a", uri);
            g_free (uri);
            mpvradio_ipc_send (tmp);
            g_free (tmp);
        }
    }
}


static void
about_activated (GSimpleAction *action,
                       GVariant      *parameter,
                       gpointer       app)
{
    //~ GtkWindow *oya;
    gchar *authors[] = { "endeavor wako", "sakai satoru", NULL };
    GList *windows = gtk_application_get_windows (app);

    gtk_show_about_dialog (windows->data,
        "copyright", "endeavor wako 2021",
        "authors", authors,
        "translator-credits", "endeavor wako (japanese)",
        "license-type", GTK_LICENSE_LGPL_2_1,
        "logo-icon-name", "media-playback-start-symbolic",
        "program-name", PACKAGE,
        "version", PACKAGE_VERSION,
        NULL);
}


static void
quit_activated (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       app)
{
  g_application_quit (G_APPLICATION (app));
}


static GActionEntry app_entries[] =
{
  { "quicktune", quicktune_activated, NULL, NULL, NULL },
  { "about", about_activated, NULL, NULL, NULL },
  { "quit", quit_activated, NULL, NULL, NULL }
};


static void
mpvradio_startup_cb (GtkApplication *app, gpointer user_data)
{
    GtkBuilder *builder;
    GMenuModel *app_menu, *popup_menu;
    GtkWidget *menu, *menu2;

    /* アプリケーションメニューの構築 */
    builder = gtk_builder_new_from_string (
    "<interface>"
    "<!-- interface-requires gtk+ 3.0 -->"
    "<menu id=\"appmenu\">"
    "<section>"
      "<item>"
        "<attribute name=\"label\" translatable=\"yes\">QuickTune</attribute>"
        "<attribute name=\"action\">app.quicktune</attribute>"
      "</item>"
    "</section>"
    "<section>"
      "<item>"
        "<attribute name=\"label\" translatable=\"yes\">_about</attribute>"
        "<attribute name=\"action\">app.about</attribute>"
      "</item>"
      "<item>"
        "<attribute name=\"label\" translatable=\"yes\">_Quit</attribute>"
        "<attribute name=\"action\">app.quit</attribute>"
      "</item>"
    "</section>"
    "</menu>"
    "</interface>",
                                            -1);
    const gchar *quit_accels[2] = { "<Ctrl>Q", NULL };
    const gchar *url_accels[2]  = { "<Ctrl>L", NULL };
    g_action_map_add_action_entries (G_ACTION_MAP (app),
                                   app_entries, G_N_ELEMENTS (app_entries),
                                   app);
    gtk_application_set_accels_for_action (app,
                                         "app.quicktune",
                                         url_accels);
    gtk_application_set_accels_for_action (app,
                                         "app.quit",
                                         quit_accels);

    app_menu = G_MENU_MODEL (gtk_builder_get_object (builder, "appmenu"));
    gtk_application_set_app_menu (app, app_menu);

    /* デスクトップ用ステータスアイコン (Linux Mint 専用) */
    appindicator = mpvradio_statusicon_new (app);

    g_object_unref (builder);

    /* IPC 受け取り後の情報格納用 */
    infomessage = gtk_entry_buffer_new ("mpvradio",-1);
    g_signal_connect (G_OBJECT(infomessage), "inserted-text",
                G_CALLBACK(infotext_inserted_text_cb), NULL);

    /* フラグの初期化 */
    mpvradio_recv_dead = TRUE;
    mpvradio_recv_stop = FALSE;
    mpvradio_connection_in = FALSE;

    /* 設定ファイル */
    kconf = load_config_file ();

    /* ラジオ局一覧 (playlist)の読み込み */
    mpvradio_read_playlist ();

    /* ipc サーバを動かす */
    g_thread_new ("mpvradio", mpvradio_ipc_recv, NULL);

    /* mpv スタート */
    mpvradio_ipc_fork_mpv ();
}


static void
mpvradio_shutdown_cb (GtkApplication *app, gpointer data)
{
    GList *windows;

    //~ mpvradio_stop_mpv ();
    mpvradio_common_stop ();    // appindicatorが存在するうちに呼ぶ事

    g_object_unref (appindicator);

    windows = gtk_application_get_windows (app);
    while (windows != NULL && GTK_IS_WINDOW(windows->data)) {
        gtk_widget_destroy (windows->data);
        windows = g_list_next(windows);
    }

    save_config_file (kconf);

    detach_config_file (kconf);


    mpvradio_ipc_kill_mpv ();
    mpvradio_ipc_remove_socket ();
    g_object_unref (infomessage);

    g_hash_table_destroy (playlist_table);
    g_list_free_full (playlist_sorted, g_free);

    g_message ("shutdown now.");
}


static void
mpvradio_activate_cb (GtkApplication *app, gpointer data)
{
    GList *windows;
    windows = gtk_application_get_windows (app);
    if (windows == NULL) {
        radikopanel = mpvradio_window_new (app);
        //~ mpvradio_notify_currentsong ();
    }
    gtk_widget_show_all (radikopanel);
    gtk_window_present (radikopanel);
}


/*
 * エントリー
 */
int main (int argc, char **argv)
{
    GtkApplication  *app;
    int             status;

    setlocale (LC_ALL, "");
    bindtextdomain (PACKAGE, LOCALEDIR);
    textdomain (PACKAGE);

    app = gtk_application_new ("com.google.endeavor2wako.mpvradio",
#if GLIB_CHECK_VERSION(2,74,0)
                                            G_APPLICATION_DEFAULT_FLAGS);
#else
                                            G_APPLICATION_FLAGS_NONE);
#endif
    g_signal_connect (app, "startup",  G_CALLBACK (mpvradio_startup_cb),  NULL);
    g_signal_connect (app, "shutdown", G_CALLBACK (mpvradio_shutdown_cb), NULL);
    g_signal_connect (app, "activate", G_CALLBACK (mpvradio_activate_cb), NULL);
    status = g_application_run (G_APPLICATION(app), argc, argv);
    g_object_unref (app);
    return status;
}
