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
#include <locale.h>
#include <errno.h>
#include <string.h>

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

XAppStatusIcon *appindicator;       // Depends on LinuxMint's libxapp
GtkEntryBuffer *infomessage;        // IPC受け取り後の格納用

GtkWidget *volume_up_button, *volume_down_button;

static GKeyFile *kconf = NULL;

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

typedef struct {
    gchar *playlist;
    gboolean version;
} AppOptions;


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

    g_key_file_set_double (kconf, "startup", "volume", value);
}


/*
 * playlist の内容をハッシュテーブルに格納する。同時にlogo画像ファイル名も得る。
 * key : 局名
 * value : URL(playlist_table), logo(playlist_logo_table)
 */
GHashTable *playlist_table, *playlist_logo_table;

void
mpvradio_read_playlist (void)
{
    gchar buf[256], *pos, *station, **playlist, **pl, *fn;
	gchar *cachedir;
    int i;
    gboolean flag = FALSE;

    cachedir = g_build_filename (g_get_user_cache_dir (), PACKAGE,
                                                            "logo",
                                                            NULL);

    playlist = g_key_file_get_keys (kconf, "playlist", NULL, NULL);
    playlist_table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
	playlist_logo_table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

    pl = playlist;
    station = NULL;
    while (*pl != NULL) {
        fn = g_key_file_get_value (kconf, "playlist", *pl, NULL);
        FILE *fp = fopen (fn, "rt");
        if (fp != NULL) {
            while (fgets (buf, sizeof(buf)-1, fp)) {
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

                if (flag == TRUE) {
                    // 直前に #EXTINFがあれば URLとして読み込む
                    if (*pos != '#') {  // コメント避け
                        char *tail = strrchr (pos, '\n');
                        if (tail != NULL) *tail = '\0';
                        if (station != NULL) {
							// URL登録
                            if (g_hash_table_contains (playlist_table, station)) {
                                if (!g_hash_table_replace (playlist_table, station, g_strdup (pos))) {
                                    g_warning ("duplicate station and URL : %s", station);
                                }
                            }
                            else {
                                g_hash_table_insert (playlist_table, station, g_strdup (pos));
                            }

							// 局名からlogoファイル名を作成
							gchar *n = g_strdup_printf ("%s.png", station);
							gchar *logofile = g_build_filename (cachedir, n, NULL);
							g_free (n);
							if (g_file_test (logofile, G_FILE_TEST_EXISTS) == FALSE) {
								// 上記logoファイルが存在しない場合はURLからlogoファイル名を作成
								gchar *bname = g_path_get_basename (pos);
								gchar *n = g_strdup_printf("%s.png", bname);
								g_free (bname);
								g_free (logofile);
								logofile = g_build_filename (cachedir, n, NULL);
								g_free (n);
								if (g_file_test (logofile, G_FILE_TEST_EXISTS) == FALSE) {
									g_message ("%s : undetect logo(banner)file.", station);
									g_free (logofile);
									// logoファイルが定義されていない場合は、プログラムアイコンで代替する
									logofile = g_build_filename (cachedir, PACKAGE".png", NULL);
								}
							}
							// logofile名 (banner) 登録
							if (g_hash_table_contains (playlist_logo_table, station)) {
								if (!g_hash_table_replace (playlist_logo_table, station, g_strdup (logofile))) {
									g_warning ("duplicate station and logo(banner)file : %s", station);
								}
							}
							else {
								g_hash_table_insert (playlist_logo_table, station, g_strdup (logofile));
							}
							g_free (logofile);
                        }
                        flag = FALSE;
                    }
                }
            }
            fclose (fp);
        }
        else {
            g_message ("mpvradio_read_playlist() : %s [%s]", strerror(errno), fn);
        }
        g_free (fn);
        pl++;
    }
    g_strfreev (playlist);
    g_free (cachedir);
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

static void
mpvradio_save_window_size (GtkWidget *widget)
{
    int width, height;
    gtk_window_get_size (GTK_WINDOW(widget), &width, &height);
    g_key_file_set_integer (kconf, "window", "width", width);
    g_key_file_set_integer (kconf, "window", "height", height);
}

static gboolean
mainwindow_delete_event_cb (GtkWidget *widget, GdkEvent *event,
                                                gpointer app)
{
    if (XAPP_IS_STATUS_ICON(appindicator)) {
        if (xapp_status_icon_get_visible (appindicator)) {
            // StatusIconが有効であれば、パネルを隠して終わる
            gtk_widget_hide (widget);
            return TRUE;
        }
    }
    mpvradio_save_window_size (widget);
    return FALSE;
}

static gboolean
mainwindow_key_press_event_cb (GtkWidget *widget,
                               GdkEvent  *event,
                               gpointer   user_data)
{
    guint k = ((GdkEventKey*)event)->keyval;

    if (k == GDK_KEY_Up) {
        g_signal_emit_by_name (volume_up_button, "clicked", NULL);
        return TRUE;
    }
    else if (k == GDK_KEY_Down) {
        g_signal_emit_by_name (volume_down_button, "clicked", NULL);
        return TRUE;
    }
    return FALSE;
}

/*
 * メインウィンドウの作成
 */
GtkWindow *
mpvradio_window_new (GtkApplication *application)
{
    GtkWidget *window, *btn, *header, *scroll, *box, *selectergrid;
    GtkWidget *infobar, *infotext, *infocontainer;
    GtkWidget *volbtn, *stopbtn, *forwardbtn, *backwardbtn;

    int width  = g_key_file_get_integer (kconf, "window", "width", NULL);
    int height = g_key_file_get_integer (kconf, "window", "height", NULL);
    double vol = g_key_file_get_double (kconf, "startup", "volume", NULL);

    window = gtk_application_window_new (application);
    gtk_window_set_default_size (GTK_WINDOW(window), width, height);
    gtk_window_set_position (GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    g_signal_connect (G_OBJECT(window), "delete-event",
                        G_CALLBACK(mainwindow_delete_event_cb), application);

    // ボリュームボタン
    volbtn = gtk_volume_button_new ();
    g_signal_connect (G_OBJECT(volbtn), "value-changed",
                        G_CALLBACK(volume_value_change_cb), NULL);
    volume_up_button = gtk_scale_button_get_plus_button (GTK_SCALE_BUTTON (volbtn));
    volume_down_button = gtk_scale_button_get_minus_button (GTK_SCALE_BUTTON (volbtn));
    gtk_scale_button_set_value (GTK_SCALE_BUTTON (volbtn), vol);

    // ストップボタン
    stopbtn = gtk_button_new_from_icon_name ("media-playback-stop-symbolic",
                                                GTK_ICON_SIZE_BUTTON);
    g_signal_connect (G_OBJECT(stopbtn), "clicked",
                                G_CALLBACK(mpvradio_common_cb), mpvradio_common_stop);
    forwardbtn = gtk_button_new_from_icon_name ("media-skip-forward-symbolic",
                                                GTK_ICON_SIZE_BUTTON);
    g_signal_connect (G_OBJECT(forwardbtn), "clicked",
                                G_CALLBACK(mpvradio_common_cb), mpvradio_common_next);
    backwardbtn = gtk_button_new_from_icon_name ("media-skip-backward-symbolic",
                                                GTK_ICON_SIZE_BUTTON);
    g_signal_connect (G_OBJECT(backwardbtn), "clicked",
                                G_CALLBACK(mpvradio_common_cb), mpvradio_common_prev);
    header = gtk_header_bar_new ();
    gtk_header_bar_set_decoration_layout (GTK_HEADER_BAR (header), "menu:close");
    gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (header), TRUE);
    gtk_header_bar_set_title (GTK_HEADER_BAR (header), PACKAGE);
    gtk_header_bar_set_has_subtitle (GTK_HEADER_BAR (header), TRUE);

    gtk_window_set_titlebar (GTK_WINDOW (window), header);
    gtk_header_bar_pack_end (GTK_HEADER_BAR (header), volbtn);
    gtk_header_bar_pack_end (GTK_HEADER_BAR (header), forwardbtn);
    gtk_header_bar_pack_end (GTK_HEADER_BAR (header), stopbtn);
    gtk_header_bar_pack_end (GTK_HEADER_BAR (header), backwardbtn);

    /* 情報表示用 */
    infotext = gtk_entry_new_with_buffer (infomessage);
    gtk_widget_set_can_focus (infotext, FALSE);

    /* 選局パネル */
    selectergrid = mpvradio_radiopanel_new ();
    scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_kinetic_scrolling (GTK_SCROLLED_WINDOW (scroll), TRUE);
    gtk_scrolled_window_set_capture_button_press (GTK_SCROLLED_WINDOW (scroll), TRUE);
    gtk_scrolled_window_set_overlay_scrolling (GTK_SCROLLED_WINDOW (scroll), TRUE);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER (scroll), selectergrid);

    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
    gtk_box_pack_start (GTK_BOX(box), infotext, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX(box), scroll, TRUE, TRUE, 0);

    gtk_container_add (GTK_CONTAINER(window), box);

    // オプション
    // 上下カーソルキーに音量調整を割り当てる
    if (g_key_file_get_boolean (kconf, "mode", "allowkey_volume", NULL)) {
        g_signal_connect (G_OBJECT(window), "key-press-event",
                    G_CALLBACK(mainwindow_key_press_event_cb), NULL);
    }

    return GTK_WINDOW(window);
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
            mpvradio_common_mpv_play (uri);
            g_free (uri);
        }
    }
}

static void
statusicon_activated (GSimpleAction *action,
                       GVariant      *parameter,
                       gpointer       app)
{
    if (XAPP_IS_STATUS_ICON(appindicator)) {
        xapp_status_icon_set_visible (appindicator,
            xapp_status_icon_get_visible (appindicator)? FALSE:TRUE);
        g_key_file_set_boolean (kconf, "window", "statusicon",
            xapp_status_icon_get_visible (appindicator));
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

    GdkPixbuf *buf = gdk_pixbuf_new_from_file (DATADIR"/pixmaps/mpvradio.png", NULL);

    gtk_show_about_dialog (windows->data,
        "copyright", "endeavor wako 2021",
        "authors", authors,
        "translator-credits", "endeavor wako (japanese)",
        "license-type", GTK_LICENSE_LGPL_2_1,
        //~ "logo-icon-name", "media-playback-start-symbolic",
        "logo", buf,
        "program-name", PACKAGE,
        "version", PACKAGE_VERSION,
        NULL);

    g_object_unref (buf);
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
  { "statusicon", statusicon_activated, NULL, NULL, NULL },
  { "about", about_activated, NULL, NULL, NULL },
  { "quit", quit_activated, NULL, NULL, NULL }
};


static void
mpvradio_startup_cb (GtkApplication *app, gpointer user_data)
{
    GtkBuilder *builder;
    GMenuModel *app_menu, *popup_menu;
    GtkWidget *menu, *menu2;

g_message ("startup.");
    /* 設定ファイル */
    kconf = load_config_file ();

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
      "<item>"
        "<attribute name=\"label\" translatable=\"yes\">StatusIcon</attribute>"
        "<attribute name=\"action\">app.statusicon</attribute>"
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

    /* デスクトップ用ステータスアイコン (Depends on LinuxMint's libxapp) */
    //~ appindicator = g_key_file_get_boolean (kconf, "window", "statusicon", NULL)?
            //~ mpvradio_statusicon_new (app):NULL;
    appindicator = mpvradio_statusicon_new (app);
    xapp_status_icon_set_visible (appindicator,
        g_key_file_get_boolean (kconf, "window", "statusicon", NULL));

    g_object_unref (builder);

    /* IPC 受け取り後の情報格納用 */
    infomessage = gtk_entry_buffer_new ("mpvradio",-1);
    g_signal_connect (G_OBJECT(infomessage), "inserted-text",
                G_CALLBACK(infotext_inserted_text_cb), NULL);

    /* フラグの初期化 */
    mpvradio_recv_dead = TRUE;
    mpvradio_recv_stop = FALSE;
    mpvradio_connection_in = FALSE;


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

    mpvradio_common_stop ();    // appindicatorが存在するうちに呼ぶ事

    if (XAPP_IS_STATUS_ICON(appindicator)) {
        g_object_unref (appindicator);
    }

    windows = gtk_application_get_windows (app);
    while (windows != NULL) {
        if (GTK_IS_WINDOW(windows->data)) {
            if (gtk_widget_in_destruction (GTK_WIDGET(windows->data)) == FALSE) {
                // メインウィンドウを閉じた場合は、２重に破壊してセグるので
                // 破壊中かどうかチェックする
                // g_application_quit で destroyしても delete-event が
                // 発生しないのでここにも処理を書く。
                mpvradio_save_window_size (GTK_WIDGET(windows->data));
                gtk_widget_destroy (GTK_WIDGET(windows->data));
            }
        }
        windows = g_list_next(windows);
    }

    save_config_file (kconf);
    detach_config_file (kconf);

    mpvradio_ipc_kill_mpv ();
    mpvradio_ipc_remove_socket ();
    g_object_unref (infomessage);

    g_hash_table_destroy (playlist_table);
    g_hash_table_destroy (playlist_logo_table);

    g_message ("shutdown now.");
}


static void
mpvradio_activate_cb (GtkApplication *app, gpointer data)
{
g_message ("activate.");
    GList *windows = gtk_application_get_windows (app);
    GtkWindow *radikopanel = (windows == NULL)? mpvradio_window_new (app):
                                            GTK_WINDOW(windows->data);
    gtk_widget_show_all (GTK_WIDGET(radikopanel));
    gtk_window_present (radikopanel);
}


static void
mpvradio_init_cmd_parameters(GOptionContext *ctx, AppOptions *options)
{
    const GOptionEntry cmd_params[] = {
        {
            .long_name = "load",
            .short_name = 'l',
            .flags = G_OPTION_FLAG_NONE,     // see `GOptionFlags`
            .arg = G_OPTION_ARG_STRING,        // type of option (see `GOptionArg`)
            .arg_data = &(options->playlist),// store data here
            .description = "プレイリストを再生します",
            .arg_description = NULL,
        },
        {
            .long_name = "version",
            .short_name = 'v',
            .flags = G_OPTION_FLAG_NONE,     // see `GOptionFlags`
            .arg = G_OPTION_ARG_NONE,        // type of option (see `GOptionArg`)
            .arg_data = &(options->version),// store data here
            .description = "バージョン番号",
            .arg_description = NULL,
        },
        {NULL}
    };

    g_option_context_add_main_entries (ctx, cmd_params, NULL);
    g_option_context_add_group (ctx, gtk_get_option_group (TRUE));
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

    GError *error = NULL;
    static AppOptions mpvradio_options = { .playlist = NULL, .version = FALSE};

    GOptionContext *options = g_option_context_new ("");
    mpvradio_init_cmd_parameters (options, &mpvradio_options);
    if (!g_option_context_parse (options, &argc, &argv, &error)) {
        g_print ("option parsing failed: %s\n", error->message);
        exit (1);
    }
    if (mpvradio_options.version) {
        g_print ("%s\n", VERSION);
        return 0;
    }
    if (mpvradio_options.playlist != NULL) {
        mpvradio_common_mpv_play (mpvradio_options.playlist);
        g_free (mpvradio_options.playlist);
    }

#if GLIB_CHECK_VERSION(2,74,0)
    app = gtk_application_new ("com.google.endeavor2wako.mpvradio",
                                            G_APPLICATION_DEFAULT_FLAGS);
#else
    app = gtk_application_new ("com.google.endeavor2wako.mpvradio",
                                            G_APPLICATION_FLAGS_NONE);
#endif

    g_signal_connect (app, "startup",  G_CALLBACK (mpvradio_startup_cb),  NULL);
    g_signal_connect (app, "shutdown", G_CALLBACK (mpvradio_shutdown_cb), NULL);
    g_signal_connect (app, "activate", G_CALLBACK (mpvradio_activate_cb), NULL);

    status = g_application_run (G_APPLICATION(app), 0, NULL);
    g_object_unref (app);
    return status;
}
