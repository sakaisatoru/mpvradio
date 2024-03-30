#include "config.h"

#include "glib.h"
#include "gtk/gtk.h"

#include "mpvradio-common.h"
#include "mpvradio-banner.h"

extern GHashTable *playlist_table, *playlist_logo_table;

/*
 * キャッシュディレクトリをチェックしてlogo(ファイル名)を得る
 */
GHashTable *banner_logo_set_up (void)
{
    GHashTable *bannertable;
    const gchar *name;
    gchar **n, *cachedir, *logofile, *key;
    GDir *d;

    cachedir = g_build_filename (g_get_user_cache_dir (), PACKAGE,
                                                            "logo",
                                                            NULL);
    bannertable = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
    if ((d = g_dir_open (cachedir, 0, NULL)) != NULL) {
        while ((name = g_dir_read_name (d)) != NULL) {
            n = g_strsplit (name, ".", 0);
            key = g_strdup (n[0]); g_strfreev (n);
            logofile = g_build_filename (cachedir, name, NULL);
            g_hash_table_replace (bannertable, key, logofile);
        }
        g_dir_close (d);
    }
    g_free (cachedir);
    return bannertable;
}


/*
 * flow_box の選択された要素上で何か起きた
 */
static gboolean child_selected_change = FALSE;

/*
 * 子ウィジェットが GtkLabel だったら、再生する
 */
static void
checkchild (GtkWidget *widget, gpointer data)
{
    gpointer url;
    if (MPVRADIO_IS_BANNER (widget)) {
        mpvradio_common_mpv_play (DATADIR"/mpvradio/sounds/button57.mp3");
        usleep (0.3*1e6);
        url = mpvradio_banner_get_url (MPVRADIO_BANNER(widget));
        //~ g_idle_add (mpvradio_common_mpv_play, url);
        mpvradio_common_mpv_play (url);
    }
}

static void
child_activated_cb (GtkFlowBox      *box,
                           GtkFlowBoxChild *child,
                           gpointer         user_data)
{
    //~ system ("bash -c \"aplay button57.wav >& /dev/null\"");  // beep音の代わり
    // フラグを参照して同一局の連続呼び出しを避ける
    if (child_selected_change == TRUE ||
            mpvradio_common_check_mpv_is_play () == FALSE) {
        child_selected_change = FALSE;
        gtk_container_foreach (GTK_CONTAINER(child), checkchild, NULL);
    }
}

static void
selected_children_changed_cb (GtkFlowBox      *box,
                           gpointer         user_data)
{
    // このイベントはchild_activatedに先行する
    //~ g_print ("selected_children_changed detect. \n");
    child_selected_change = TRUE;
}


/*
 * 選局ボタンを並べたgtk_flow_boxを返す
 */
GtkWidget *
mpvradio_radiopanel_new (void)
{
    GtkWidget *btn, *grid;
    gpointer station, url, banner;
    gchar *st, **st0;

    grid = gtk_flow_box_new ();
    gtk_flow_box_set_selection_mode (GTK_FLOW_BOX(grid),GTK_SELECTION_SINGLE);
    gtk_flow_box_set_homogeneous (GTK_FLOW_BOX(grid), TRUE);
    gtk_flow_box_set_activate_on_single_click (GTK_FLOW_BOX(grid), TRUE);
    gtk_flow_box_set_row_spacing (GTK_FLOW_BOX(grid), 2);
    gtk_flow_box_set_column_spacing (GTK_FLOW_BOX(grid), 2);
    gtk_flow_box_set_max_children_per_line (GTK_FLOW_BOX(grid), 6);
    // ラベルにてボタンクリックと等価の動作を行うための準備
    g_signal_connect (G_OBJECT(grid), "child-activated",
                G_CALLBACK(child_activated_cb), NULL);

    // カーソルキーで移動する毎に生じるイベント
    g_signal_connect (G_OBJECT(grid), "selected-children-changed",
                G_CALLBACK(selected_children_changed_cb), NULL);

    /*
     * playlist_table をチェックして選局ボタンを並べる
     */
    GList *playlist_sorted, *curr;
    int i = 0;
    playlist_sorted = g_hash_table_get_keys (playlist_table);
    playlist_sorted = curr = g_list_sort (playlist_sorted, strcmp);
    for (;curr != NULL;curr = g_list_next (curr)) {
        if (curr->data != NULL) {
            url = g_hash_table_lookup (playlist_table, curr->data);

            // urlの末尾をキーにしてlogoのファイル名を得る
            st = g_path_get_basename (url);
            st0 = g_strsplit (st, ".", 0);

            banner = g_hash_table_lookup (playlist_logo_table, st0[0]);

            g_strfreev (st0);
            g_free (st);

            btn = mpvradio_banner_new_with_data (GTK_ORIENTATION_VERTICAL, 16,
                            curr->data, (gchar*)url, (gchar*)banner);
            gtk_flow_box_insert (GTK_FLOW_BOX(grid), btn, -1);
        }
    }
    g_list_free (playlist_sorted);
    return grid;
}

