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
//~ #include "libnotify/notification.h"
//~ #include "libnotify/notify.h"

#include "libxapp/xapp-status-icon.h"
#include "libxapp/xapp-preferences-window.h"

#include "mpvradio.h"
#include "mpvradio-stationbutton.h"
#include "mpvradio-common.h"
#include "mpvradio-statusicon.h"
#include "mpvradio-ipc.h"

//~ NotifyNotification *notifi;         // デスクトップ通知
GtkWindow *radikopanel;
GtkWindow *selectergrid;
XAppStatusIcon *appindicator;       // LinuxMint 専用
GtkWidget *infomessage;             // IPC受け取り後の格納用

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
extern gchar *mpvradio_adduri_quicktune (GtkWindow *oya);
//~ extern void mpvradio_adduridialog_destroy (void);


/* mpvradio-config.c */
extern void detach_config_file (GKeyFile *);
extern void save_config_file (GKeyFile *);
extern GKeyFile *load_config_file (void);
extern XAppPreferencesWindow *mpvradio_config_prefernces_ui (void);

/* mpvradio-notify.c */
//~ extern void mpvradio_notify_currentsong (void);


/*
 * 選局ボタンのコールバック
 * url を見て必要であればプラグインを呼び出す
 */
static void _mpvradio_radiopanel_clicked_cb (mpvradioStationbutton *btn,
                                                gpointer data)
{
    char *url = mpvradio_stationbutton_get_uri (btn);
    char *message;

    gchar *scheme = g_uri_parse_scheme (url);
    //~ g_print ("scheme:%s\n", scheme);

    if (!strcmp (scheme, "plugin")) {
        // url の先頭がpluginであれば呼び出しにかかる
        gchar *station = g_path_get_basename (url); // basename をplugin の引数にする
        //~ g_print ("station:%s\n", station);

        gchar *p_path = g_path_get_dirname (url);
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
        message = g_strdup_printf ("{\"command\": [\"loadfile\",\"%s\"]}\x0a", url);
        mpvradio_ipc_send (message);
        g_free (message);
    }

    g_free (scheme);
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
GHashTable *playlist_table;
GList *playlist_sorted = NULL;
void mpvradio_read_playlist (void)
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




static void _test_cb (gchar *url)
{
    char *message;

    gchar *scheme = g_uri_parse_scheme (url);
    //~ g_print ("scheme:%s\n", scheme);

    if (!strcmp (scheme, "plugin")) {
        // url の先頭がpluginであれば呼び出しにかかる
        gchar *station = g_path_get_basename (url); // basename をplugin の引数にする
        //~ g_print ("station:%s\n", station);

        gchar *p_path = g_path_get_dirname (url);
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
        message = g_strdup_printf ("{\"command\": [\"loadfile\",\"%s\"]}\x0a", url);
        mpvradio_ipc_send (message);
        g_free (message);
    }

    g_free (scheme);
}

/*
 * 子ウィジェットがstationbuttonだったら、clickedイベントを起こす
 */
static
void checkchild (GtkWidget *widget, gpointer data)
{
    //~ if (mpvradio_IS_STATIONBUTTON (widget)) {
        //~ g_print ("station button.");
        //~ g_signal_emit_by_name (widget, "clicked");
    //~ }

    gpointer url;
    //~ if (!strcmp (gtk_widget_get_name (widget), "GtkLabel")) {
    if (GTK_IS_LABEL (widget)) {
        url = g_hash_table_lookup (playlist_table, gtk_label_get_text (widget));
        g_print ("label : %s   url : %s\n",
                    gtk_label_get_text (widget), (gchar*)url);
        _test_cb ((gchar*)url);
    }
}

/*
 * flow_box の選択された要素上で何か起きた
 */
static
void child_activated_cb (GtkFlowBox      *box,
                           GtkFlowBoxChild *child,
                           gpointer         user_data)
{
    //~ g_print ("child-activated. %d\n", gtk_flow_box_child_get_index (child));
    gtk_container_foreach (GTK_CONTAINER(child), checkchild, NULL);
}

static
void selected_children_changed_cb (GtkFlowBox      *box,
                           gpointer         user_data)
{
    g_print ("selected_children_changed detect. \n");
}

/*
 * 選局ボタンを並べたgtk_flow_boxを返す
 */
static GtkWidget *selectergrid_new (void)
{
    GtkWidget *btn, *grid;
    GHashTableIter iter;
    gpointer station, url;

    /*
     * playlist_table をチェックして選局ボタンを並べる
     */
    grid = gtk_flow_box_new ();
    //~ gtk_flow_box_set_selection_mode (GTK_FLOW_BOX(grid),GTK_SELECTION_NONE);
    gtk_flow_box_set_selection_mode (GTK_FLOW_BOX(grid),GTK_SELECTION_SINGLE);
    gtk_flow_box_set_homogeneous (GTK_FLOW_BOX(grid), TRUE);
    gtk_flow_box_set_activate_on_single_click (GTK_FLOW_BOX(grid), TRUE);
    // キー押下で選局ボタンにイベントを送るための準備
    g_signal_connect (G_OBJECT(grid), "child-activated",
                G_CALLBACK(child_activated_cb), NULL);
    // test
    g_signal_connect (G_OBJECT(grid), "selected-children-changed",
                G_CALLBACK(selected_children_changed_cb), NULL);

    GList *curr;
    int i = 0;
    for (curr = g_list_first (playlist_sorted);
                curr != NULL;curr = g_list_next (curr)) {
        if (curr->data != NULL) {
            url = g_hash_table_lookup (playlist_table, curr->data);
            //~ btn = mpvradio_stationbutton_new_with_data (curr->data, url);
            //~ gtk_widget_set_size_request (btn, button_width, button_height);
            //~ g_signal_connect (G_OBJECT(btn), "clicked",
                //~ G_CALLBACK(_mpvradio_radiopanel_clicked_cb), grid);
            btn = gtk_label_new (curr->data);
            gtk_widget_set_size_request (btn, -1, 48);
            gtk_flow_box_insert (GTK_FLOW_BOX(grid), btn, -1);
        }
    }

    return grid;
}

/*
 * ドラッグアンドドロップでファイル名を受け取ってmpvへ送る
 */
static void radiopanel_dd_received (GtkWidget *widget,
                                    GdkDragContext *context,
                                    gint        x,
                                    gint        y,
                                    GtkSelectionData *data,
                                    guint       info,
                                    guint       time,
                                    gpointer    user_data)
{
    gchar **filenames = NULL, *filename, *message;

    filenames = g_uri_list_extract_uris((const gchar *)gtk_selection_data_get_data (data));
    if (filenames == NULL) {
        //~ g_print ("error");
        gtk_drag_finish (context, FALSE, FALSE, time);
        return;
    }

    for (int iter = 0; filenames[iter] != NULL; iter++) {
        filename = g_filename_from_uri (filenames[iter], NULL, NULL);
        //~ g_print("detect : %s\n",filename);

        // 処理
        message = g_strdup_printf (
                    "{\"command\": [\"loadfile\",\"%s\",\"append-play\"]}\x0a",
                                                            filename);
        mpvradio_ipc_send (message);
        g_free (message);

        g_free (filename);
    }
    g_strfreev (filenames);
    gtk_drag_finish (context, TRUE, FALSE, time);
}


GtkEventController *event_c;
gboolean event_c_cb (GtkEventControllerKey *controller,
                            guint                  keyval,
                            guint                  keycode,
                            GdkModifierType        state,
                            gpointer               user_data)
{
    g_message ("val:%d code:%d",keyval, keycode);
    return FALSE;
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

    int i, width, height;
    double vol = 0.5;

    GtkWidget *window, *btn, *header, *scroll, *box;
    GtkWidget *infobar, *infotext, *infocontainer;
    GtkWidget *volbtn, *stopbtn, *playbtn, *nextbtn, *prevbtn;
    GtkWidget *menubtn;
    GMenuModel *menumodel;

    gint x,y;

    window = gtk_application_window_new (application);
    g_signal_connect (G_OBJECT(window), "destroy",
                        G_CALLBACK(radiopanel_destroy_cb), NULL);
    g_signal_connect (G_OBJECT(window), "delete-event",
                        G_CALLBACK(gtk_widget_hide_on_delete), NULL);

    // プレイリストをDnDで受け取るための準備
    static GtkTargetEntry te[] = {
        {"text/uri-list", 0, 0}
    };
    gtk_drag_dest_set (window, GTK_DEST_DEFAULT_ALL, te, 1, GDK_ACTION_COPY);
    g_signal_connect (G_OBJECT(window), "drag-data-received",
                        G_CALLBACK(radiopanel_dd_received), NULL);

    // ボリュームボタン
    volbtn = gtk_volume_button_new ();
    //~ gtk_scale_button_set_value (volbtn, 1.0);
    g_signal_connect (G_OBJECT(volbtn), "value-changed",
                        G_CALLBACK(volume_value_change_cb), NULL);

    volume_up_button = gtk_scale_button_get_plus_button (volbtn);
    volume_down_button = gtk_scale_button_get_minus_button (volbtn);


    // 戻るボタン
    prevbtn = gtk_button_new_from_icon_name ("media-skip-backward",
                                                GTK_ICON_SIZE_BUTTON);
    g_signal_connect (G_OBJECT(prevbtn), "clicked",
                                G_CALLBACK(mpvradio_common_cb), mpvradio_common_prev);
    // プレイ・ポーズボタン
    playbtn = gtk_button_new_from_icon_name ("media-playback-pause-symbolic",
                                                GTK_ICON_SIZE_BUTTON);
    g_signal_connect (G_OBJECT(playbtn), "clicked",
                                G_CALLBACK(mpvradio_common_cb), mpvradio_common_toggle_pause);
    // 進むボタン
    nextbtn = gtk_button_new_from_icon_name ("media-skip-forward",
                                                GTK_ICON_SIZE_BUTTON);
    g_signal_connect (G_OBJECT(nextbtn), "clicked",
                                G_CALLBACK(mpvradio_common_cb), mpvradio_common_next);
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
    gtk_header_bar_pack_end (GTK_HEADER_BAR (header), stopbtn);
    gtk_header_bar_pack_end (GTK_HEADER_BAR (header), nextbtn);
    gtk_header_bar_pack_end (GTK_HEADER_BAR (header), playbtn);
    gtk_header_bar_pack_end (GTK_HEADER_BAR (header), prevbtn);


    /* 情報表示用 */
    infobar = gtk_box_new (GTK_ORIENTATION_VERTICAL, 3);
    infotext = gtk_entry_new_with_buffer (infomessage);
    gtk_widget_set_can_focus (infotext, FALSE);
    gtk_box_pack_start (infobar, infotext, FALSE, TRUE, 0);

    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 3);
    gtk_box_pack_start (box, infobar, FALSE, TRUE, 0);

    selectergrid = selectergrid_new ();

//
    //~ event_c = gtk_event_controller_key_new (selectergrid);
    //~ g_signal_connect (G_OBJECT(event_c), "key-pressed",
                        //~ G_CALLBACK(event_c_cb), NULL);
//

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
    gchar *filename, *message;

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
            filename = gtk_file_chooser_get_filename (chooser);
            // 以下の処理が躓くとダイアログが閉じないので先に閉じる
            gtk_widget_destroy (dialog);

            // 処理
            message = g_strdup_printf (
                    "{\"command\": [\"loadfile\",\"%s\",\"append-play\"]}\x0a",
                                                            filename);
            //~ g_message ("file name : %s", filename);
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
  { "quicktune", quicktune_activated, NULL, NULL, NULL },
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
    const gchar *url_accels[2]  = { "<Ctrl>L", NULL };
    g_action_map_add_action_entries (G_ACTION_MAP (app),
                                   app_entries, G_N_ELEMENTS (app_entries),
                                   app);
    gtk_application_set_accels_for_action (GTK_APPLICATION (app),
                                         "app.quicktune",
                                         url_accels);
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

    /* IPC 受け取り後の情報格納用 */
    infomessage = gtk_entry_buffer_new ("mpvradio",-1);

    /* デスクトップ通知の準備 */
    //~ notify_init (PACKAGE);
    //~ notifi = notify_notification_new (PACKAGE, "a mpd client for radio", mpvradioICON);

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

static void mpvradio_shutdown_cb (GtkApplication *app, gpointer data)
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

    //~ g_object_unref (notifi);
    //~ notify_uninit ();
    detach_config_file (kconf);


    mpvradio_ipc_kill_mpv ();
    mpvradio_ipc_remove_socket ();
    g_object_unref (infomessage);

    g_hash_table_destroy (playlist_table);
    g_list_free_full (playlist_sorted, g_free);

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
