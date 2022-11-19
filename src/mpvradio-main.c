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
#include "libnotify/notification.h"
#include "libnotify/notify.h"

#include "libxapp/xapp-status-icon.h"
#include "libxapp/xapp-preferences-window.h"

#include "mpvradio.h"
#include "mpvradio-stationbutton.h"
#include "mpvradio-common.h"
#include "mpvradio-statusicon.h"

NotifyNotification *notifi;         // デスクトップ通知
GtkWindow *radikopanel;
GtkWindow *selectergrid;
XAppStatusIcon *appindicator;       // LinuxMint 専用

static GKeyFile *kconf;


GtkWidget *volume_up_button, *volume_down_button;

static gpointer mpvradio_eventmonitor_gt (gpointer n);
static void mpvradio_cb (GtkWidget *menuitem, gpointer user_data);

static gboolean mpvradio_recv_dead;
static gboolean mpvradio_recv_stop;
static GtkActionGroup *actions;
static gboolean mpvradio_connection_in;

static const gint button_width = 200;
static const gint button_height = 70;


/* mpvradio_adduri.c */
extern void mpvradio_adduri_quicktune (GtkWindow *oya);
//~ extern void mpvradio_adduridialog_destroy (void);


/* mpvradio-config.c */
extern void detach_config_file (GKeyFile *);
extern void save_config_file (GKeyFile *);
extern GKeyFile *load_config_file (void);
extern XAppPreferencesWindow *mpvradio_config_prefernces_ui (void);

/* mpvradio-notify.c */
extern void mpvradio_notify_currentsong (void);

/*
 *  実行中の mpv を止める
 */

void mpvradio_stop_mpv (void)
{
    //~ if (current_mpv) {
        //~ g_message ("kill %d", current_mpv);
        //~ kill (current_mpv, SIGTERM);
    //~ }

    mpvradio_ipc_send ("{\"command\": [\"stop\"]}\x0a");
}

/*
 * 選局ボタンのコールバック
 */
static void _mpvradio_radiopanel_clicked_cb (mpvradioStationbutton *btn,
                                                gpointer data)
{
    char *url = mpvradio_stationbutton_get_uri (btn);
    char *message;

    message = g_strdup_printf ("{\"command\": [\"loadfile\",\"%s\"]}\x0a", url);
    mpvradio_ipc_send (message);
    g_free (message);
}


/*
 * 音量調整
 */
static void volume_value_change_cb (GtkScaleButton *button,
                                           double          value,
                                           gpointer        user_data)
{
    char *message;

    message =
        g_strdup_printf ("{\"command\": [\"set_property\",\"volume\",%d]}\x0a", (uint32_t)(value*100.));
    mpvradio_ipc_send (message);
    g_free (message);
}

static void radiopanel_destroy_cb (GtkWidget *widget, gpointer data)
{
    if (GTK_IS_WINDOW (widget)) {
        g_message ("destroy_cb : %s.", gtk_window_get_title (widget));
    }
}

/*
 * playlist の内容をハッシュテーブルに格納する
 */
static GHashTable *playlist_table;
void mpvradio_read_playlist (void)
{
    gchar buf[256], *pos, *station, *url, **playlist, **pl, *fn;
    int i;

    playlist = g_key_file_get_keys (kconf, "playlist", NULL, NULL);
    playlist_table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

    pl = playlist;
    while (*pl != NULL) {
        fn = g_key_file_get_value (kconf, "playlist", *pl, NULL);
        g_message (fn);
        FILE *fp = fopen (fn, "r");
        if (fp != NULL) {
            while (!feof(fp)) {
                if (fgets (buf, sizeof(buf)-1, fp) == NULL) {
                    if (ferror(fp)) {
                        break;
                    }
                }
                pos = g_strchug (buf);
                if (pos[0] != '#') {
                    // コメント行は飛ばす
                    i = strlen(pos) - 1;
                    // 空行は飛ばす
                    if (i > 0) {
                        if (pos[i] == '\n') pos[i] = '\0';
                        station = g_path_get_basename (pos);
                        url = g_strdup (pos);
                        g_message ("path : %s   station : %s", url, station);
                        g_hash_table_insert (playlist_table, station, url);
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
}



/*
 * 選局ボタンを並べたgtk_flow_boxを返す
 */
static GtkWidget *selectergrid_new (void)
{
    GtkWidget *btn, *grid = NULL;
    GHashTableIter iter;
    gpointer station, url;

    gint x_margin = 1, y_margin = 1;
    char *playlist[] = {"00_radiko", "radio"};

    /*
     * playlist_table をチェックして選局ボタンを並べる
     */
    grid = gtk_flow_box_new ();
    gtk_flow_box_set_column_spacing (GTK_FLOW_BOX(grid), x_margin);
    gtk_flow_box_set_row_spacing (GTK_FLOW_BOX(grid), y_margin);

    g_hash_table_iter_init (&iter, playlist_table);

    while (g_hash_table_iter_next (&iter, &station, &url)) {
        btn = mpvradio_stationbutton_new ();
        gtk_button_set_label (btn, station);
        mpvradio_stationbutton_set_uri (btn, url);
        gtk_widget_set_size_request (btn, button_width, button_height);
        g_signal_connect (G_OBJECT(btn), "clicked",
            G_CALLBACK(_mpvradio_radiopanel_clicked_cb), NULL);

        gtk_container_add (GTK_CONTAINER(grid), btn);
    }

    return grid;
}

/*
 * 選局パネル(ウィンドウ)の作成
 */
GtkWindow *mpvradio_radiopanel (GtkApplication *application)
{
    struct mpd_connection *cn;
    struct mpd_playlist *pl;
    struct mpd_entry *en;
    struct mpd_song *sn;
    struct mpd_status *st;

    char *playlist[] = {"00_radiko", "radio"};
    int i, width, height;
    double vol = 0.5;

    GtkWidget *window, *btn, *header, *scroll, *box;
    GtkWidget *infobar, *infotext, *infocontainer;
    GtkWidget *volbtn, *stopbtn, *playbtn;
    GtkWidget *menubtn;
    GMenuModel *menumodel;

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

    volume_up_button = gtk_scale_button_get_plus_button (volbtn);
    volume_down_button = gtk_scale_button_get_minus_button (volbtn);


    // プレイ・ポーズボタン
    playbtn = gtk_button_new_from_icon_name ("media-playback-start-symbolic",
                                                GTK_ICON_SIZE_BUTTON);
    g_signal_connect (G_OBJECT(playbtn), "clicked",
                                G_CALLBACK(mpvradio_common_cb), mpvradio_common_play);
    // ストップボタン
    stopbtn = gtk_button_new_from_icon_name ("media-playback-stop-symbolic",
                                                GTK_ICON_SIZE_BUTTON);
    g_signal_connect (G_OBJECT(stopbtn), "clicked",
                                G_CALLBACK(mpvradio_common_cb), mpvradio_common_stop);

    header = gtk_header_bar_new ();
    gtk_header_bar_set_decoration_layout (header, "menu:close");
    gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (header), TRUE);
    gtk_header_bar_set_title (GTK_HEADER_BAR (header), PACKAGE);
    gtk_header_bar_set_has_subtitle (GTK_HEADER_BAR (header), TRUE);

    gtk_window_set_titlebar (GTK_WINDOW (window), header);
    //~ gtk_header_bar_pack_start (GTK_HEADER_BAR (header), menubtn);
    gtk_header_bar_pack_end (GTK_HEADER_BAR (header), volbtn);
    //~ gtk_header_bar_pack_end (GTK_HEADER_BAR (header), playbtn);
    gtk_header_bar_pack_end (GTK_HEADER_BAR (header), stopbtn);


    /* エラー表示用 */
    infobar = gtk_info_bar_new ();
    gtk_widget_set_no_show_all (infobar, TRUE);
    infotext = gtk_label_new ("");
    infocontainer = gtk_info_bar_get_content_area (infobar);
    gtk_container_add (GTK_CONTAINER(infocontainer), infotext);

    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 3);
    gtk_box_pack_start (box, infobar, FALSE, TRUE, 0);

    selectergrid = selectergrid_new ();
    gtk_scale_button_set_value (volbtn, vol);

    scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (scroll, selectergrid);

    gtk_box_pack_start (box, scroll, TRUE, TRUE, 0);

    gtk_container_add (window, box);

    gtk_window_set_default_size (GTK_WINDOW (window), button_width * 4, button_height * 4);
    //~ gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);
    return window;
}

static void
fileopen_activated (GSimpleAction *action,
                       GVariant      *parameter,
                       gpointer       app)
{
    GList *windows;
    GtkWidget *dialog;
    gint res;

    GtkFileChooserAction act = GTK_FILE_CHOOSER_ACTION_OPEN;

    windows = gtk_application_get_windows (app);
    if (windows != NULL) {
        dialog = gtk_file_chooser_dialog_new ("Open File",
                                                windows->data,
                                                act,
                                                _("_Cancel"),
                                                GTK_RESPONSE_CANCEL,
                                                _("_Open"),
                                                GTK_RESPONSE_ACCEPT,
                                                NULL);
        res = gtk_dialog_run (GTK_DIALOG(dialog));
        if (res == GTK_RESPONSE_ACCEPT) {
            GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
            char *filename = gtk_file_chooser_get_filename (chooser);
            // 以下の処理が躓くとダイアログが閉じないので先に閉じる
            gtk_widget_destroy (dialog);

            // 処理
            char *message = g_strdup_printf ("{\"command\": [\"loadfile\",\"%s\"]}\x0a", filename);
            g_message ("file name : %s", filename);
            g_free (filename);
            mpvradio_ipc_send (message);
            g_free (message);
        }
        else {
            gtk_widget_destroy (dialog);
        }
    }
}

static void
quicktune_activated (GSimpleAction *action,
                       GVariant      *parameter,
                       gpointer       app)
{
    GList *windows;
    windows = gtk_application_get_windows (app);
    if (windows != NULL) {
        mpvradio_adduri_quicktune (windows->data);
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

static void
disconnect_activated (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       app)
{
    /* mpd応答待ち処理側のスレッドを止める。
     * 実際に止まるのは、mpdからの応答を得た後
     */
    mpvradio_recv_stop = TRUE;
    /*
     * ここでUIの破壊ルーチンを呼ぶ
     */
    gtk_widget_hide (selectergrid);
    //~ gtk_widget_destroy (selectergrid);
    g_message ("exit disconnect_activate.");
}

static GActionEntry app_entries[] =
{
  //~ { "disconnect", disconnect_activated, NULL, NULL, NULL },
  //~ { "quicktune", quicktune_activated, NULL, NULL, NULL },
  { "fileopen", fileopen_activated, NULL, NULL, NULL },
  { "about", about_activated, NULL, NULL, NULL },
  { "quit", quit_activated, NULL, NULL, NULL }
};



static void mpvradio_startup_cb (GApplication *app, gpointer user_data)
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
        "<attribute name=\"label\" translatable=\"yes\">Disconnect</attribute>"
        "<attribute name=\"action\">app.disconnect</attribute>"
      "</item>"
      "<item>"
        "<attribute name=\"label\" translatable=\"yes\">QuickTune</attribute>"
        "<attribute name=\"action\">app.quicktune</attribute>"
      "</item>"
      "<item>"
        "<attribute name=\"label\" translatable=\"yes\">FileOpen</attribute>"
        "<attribute name=\"action\">app.fileopen</attribute>"
      "</item>"
    "</section>"
    "<section>"
      "<item>"
        "<attribute name=\"label\" translatable=\"yes\">_Preferences</attribute>"
        "<attribute name=\"action\">app.preferences</attribute>"
      "</item>"
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
    const gchar *open_accels[2] = { "<Ctrl>O", NULL };
    g_action_map_add_action_entries (G_ACTION_MAP (app),
                                   app_entries, G_N_ELEMENTS (app_entries),
                                   app);
    gtk_application_set_accels_for_action (GTK_APPLICATION (app),
                                         "app.quit",
                                         quit_accels);
    gtk_application_set_accels_for_action (GTK_APPLICATION (app),
                                         "app.fileopen",
                                         open_accels);

    app_menu = G_MENU_MODEL (gtk_builder_get_object (builder, "appmenu"));
    gtk_application_set_app_menu (GTK_APPLICATION (app), app_menu);

    /* デスクトップ用ステータスアイコン (Linux Mint 専用) */
    appindicator = mpvradio_statusicon_new (app);

    g_object_unref (builder);

    /* デスクトップ通知の準備 */
    notify_init (PACKAGE);
    notifi = notify_notification_new (PACKAGE, "a mpd client for radio", mpvradioICON);

    /* フラグの初期化 */
    mpvradio_recv_dead = TRUE;
    mpvradio_recv_stop = FALSE;
    mpvradio_connection_in = FALSE;

    /* 設定ファイル */
    kconf = load_config_file ();

    /* ラジオ局一覧 (playlist)の読み込み */
    mpvradio_read_playlist ();

    /* mpv スタート */
    mpvradio_ipc_fork_mpv ();
}

static void mpvradio_shutdown_cb (GtkApplication *app, gpointer data)
{
    GList *windows;

    mpvradio_stop_mpv ();

    g_object_unref (appindicator);

    windows = gtk_application_get_windows (app);
    while (windows != NULL && GTK_IS_WINDOW(windows->data)) {
        gtk_widget_destroy (windows->data);
        windows = g_list_next(windows);
    }

    save_config_file (kconf);

    g_object_unref (notifi);
    notify_uninit ();
    detach_config_file (kconf);

    g_hash_table_destroy (playlist_table);

    mpvradio_ipc_kill_mpv ();
    g_message ("shutdown now.");
}

static void mpvradio_activate_cb (GtkApplication *app, gpointer data)
{
    GList *windows;
    windows = gtk_application_get_windows (app);
    if (windows == NULL) {
        radikopanel = mpvradio_radiopanel (app);
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
                                            G_APPLICATION_FLAGS_NONE);
    g_signal_connect (app, "startup",  G_CALLBACK (mpvradio_startup_cb),  NULL);
    g_signal_connect (app, "shutdown", G_CALLBACK (mpvradio_shutdown_cb), NULL);
    g_signal_connect (app, "activate", G_CALLBACK (mpvradio_activate_cb), NULL);
    status = g_application_run (G_APPLICATION(app), argc, argv);
    g_object_unref (app);
    return status;
}
